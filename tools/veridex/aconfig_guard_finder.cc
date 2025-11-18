/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "aconfig_guard_finder.h"

#include <android-base/strings.h>

#include "dependency_graph.h"
#include "dex/class_accessor-inl.h"
#include "dex/dex_instruction-inl.h"

namespace art {

AconfigGuardFinder::AconfigGuardFinder(VeridexResolver* resolver,
                                       const ClassAccessor::Method& method,
                                       DependencyGraph* dependency_graph)
    : VeriFlowAnalysis(resolver, method), dependency_graph_(dependency_graph) {}

bool AconfigGuardFinder::IsPcGuarded(uint32_t dex_pc) const {
  for (const auto& block : guarded_blocks_) {
    if (dex_pc >= block.first && dex_pc < block.second) {
      // This dex_pc is guarded
      return true;
    }
  }
  return false;
}

RegisterValue AconfigGuardFinder::AnalyzeInvoke(uint32_t dex_pc ATTRIBUTE_UNUSED,
                                                const Instruction& instruction,
                                                bool is_range) {
  uint32_t id = is_range ? instruction.VRegB_3rc() : instruction.VRegB_35c();
  DependencyNode* node = dependency_graph_->GetNode(NodeType::kMethod, id);
  if (node == nullptr) {
    // This is not expected. Node must be found in the dependency_graph
    return GetReturnType(id);
  }
  if (node->IsAconfigSource()) {
    return RegisterValue(
        RegisterSource::kAconfigFlag, DexFileReference(nullptr, 0), VeriClass::boolean_);
  }

  const DexFile& dex_file = resolver_->GetDexFile();
  const dex::MethodId& method_id = dex_file.GetMethodId(id);
  std::string signature = dex_file.GetMethodSignature(method_id).ToString();

  if (signature == "(Ljava/lang/String;)Landroid/os/flagging/AconfigPackage;" ||
      signature == "(Ljava/lang/String;J)Landroid/os/flagging/PlatformAconfigPackageInternal;") {
    // Found a method with an aconfig flag signature.
    node->PropagateAconfigFlag();
    return RegisterValue(RegisterSource::kAconfigPackage, DexFileReference(nullptr, 0), nullptr);
  } else if (android::base::EndsWith(signature, ")Z")) {
    uint32_t args[5];
    instruction.GetVarArgs(args);
    uint32_t package_reg = is_range ? instruction.VRegC() : args[0];
    if (aconfig_package_registers_.count(package_reg)) {
      node->PropagateAconfigFlag();
      return RegisterValue(
          RegisterSource::kAconfigFlag, DexFileReference(nullptr, 0), VeriClass::boolean_);
    }
  }

  return GetReturnType(id);
}

void AconfigGuardFinder::AnalyzeFieldSet(const Instruction& instruction) {
  if (instruction.Opcode() == Instruction::SPUT_OBJECT) {
    uint32_t field_id = instruction.VRegB_21c();
    const DexFile& dex_file = resolver_->GetDexFile();
    if (android::base::EndsWith(dex_file.PrettyField(field_id), "Flags.FEATURE_FLAGS")) {
      DependencyNode* field_node = dependency_graph_->GetNode(NodeType::kField, field_id);
      if (field_node != nullptr) {
        // FEATURE_FLAGS found. Set this node as a aconfig flag.
        // This is a deprecated identification. It is not required with the ExportdFlags.
        field_node->PropagateAconfigFlag();
      }
    }
  }
}

bool AconfigGuardFinder::IsAconfigRegister(uint32_t reg) const {
  const RegisterValue& reg_value = GetRegister(reg);
  RegisterSource source = reg_value.GetSource();

  if (source == RegisterSource::kAconfigFlag || source == RegisterSource::kAconfigPackage) {
    return true;
  } else if (source == RegisterSource::kMethod) {
    uint32_t method_id = reg_value.GetDexFileReference().index;
    return dependency_graph_->IsNodeAconfigSource(NodeType::kMethod, method_id);
  } else if (source == RegisterSource::kField) {
    uint32_t field_id = reg_value.GetDexFileReference().index;
    return dependency_graph_->IsNodeAconfigSource(NodeType::kField, field_id);
  }
  return false;
}

void AconfigGuardFinder::ProcessBranch(uint32_t dex_pc, const Instruction& instruction) {
  if (instruction.Opcode() == Instruction::IF_EQZ || instruction.Opcode() == Instruction::IF_NEZ) {
    uint32_t reg = instruction.VRegA();
    if (IsAconfigRegister(reg)) {
      // This is a branch controlled by a feature flag.
      uint32_t branch_target = dex_pc + instruction.GetTargetOffset();
      uint32_t next_pc = dex_pc + instruction.SizeInCodeUnits();
      if (instruction.Opcode() == Instruction::IF_EQZ) {
        // The guarded block is from the next instruction to the branch target.
        guarded_blocks_.emplace_back(next_pc, branch_target);
      } else {  // IF_NEZ
        // The guarded block is from the branch target to the next branch target.
        uint32_t guarded_block_start = branch_target;
        uint32_t guarded_block_end = guarded_block_start;
        const auto& code_item = GetCodeItemAccessor();
        const uint32_t max_pc = code_item.InsnsSizeInCodeUnits();
        while (guarded_block_end < max_pc) {
          if (guarded_block_end > guarded_block_start && IsBranchTarget(guarded_block_end)) {
            break;
          }
          const uint16_t* end_insns = code_item.Insns() + guarded_block_end;
          const Instruction& current_end_inst = *Instruction::At(end_insns);
          int flags = Instruction::FlagsOf(current_end_inst.Opcode());
          if (((flags & Instruction::kBranch) != 0 && (flags & Instruction::kContinue) == 0) ||
              (flags & Instruction::kReturn) != 0) {
            guarded_block_end += current_end_inst.SizeInCodeUnits();
            break;
          }
          guarded_block_end += current_end_inst.SizeInCodeUnits();
        }
        guarded_blocks_.emplace_back(guarded_block_start, guarded_block_end);
      }
    }
  }
}

}  // namespace art
