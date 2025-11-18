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

#include "dependency_graph.h"

#include <iostream>

#include "dex/class_accessor-inl.h"
#include "dex/code_item_accessors-inl.h"
#include "dex/dex_file-inl.h"
#include "dex/dex_instruction-inl.h"
#include "resolver.h"
#include "veridex.h"

namespace art {

void DependencyGraph::Build(const std::vector<std::unique_ptr<VeridexResolver>>& resolvers) {
  for (const std::unique_ptr<VeridexResolver>& resolver : resolvers) {
    const DexFile& dex_file = resolver->GetDexFile();
    for (ClassAccessor accessor : dex_file.GetClasses()) {
      for (const ClassAccessor::Method& method : accessor.GetMethods()) {
        if (method.GetCodeItem() == nullptr) {
          continue;
        }

        DependencyNode* current_method_node = GetOrCreateNode(NodeType::kMethod, method.GetIndex());
        CodeItemDataAccessor codes = method.GetInstructionsAndData();
        if (codes.InsnsSizeInCodeUnits() == 0) {
          continue;
        }
        const uint32_t max_pc = codes.InsnsSizeInCodeUnits();

        DependencyNode* last_invoked_method_node = nullptr;
        std::vector<DependencyNode*> register_sources(codes.RegistersSize(), nullptr);

        for (const DexInstructionPcPair& inst_pair : codes) {
          const Instruction& inst = inst_pair.Inst();
          if (inst_pair.DexPc() >= max_pc) {
            break;
          }

          switch (inst.Opcode()) {
            case Instruction::INVOKE_DIRECT:
            case Instruction::INVOKE_INTERFACE:
            case Instruction::INVOKE_STATIC:
            case Instruction::INVOKE_SUPER:
            case Instruction::INVOKE_VIRTUAL: {
              uint32_t callee_method_id = inst.VRegB_35c();
              DependencyNode* callee_node = GetOrCreateNode(NodeType::kMethod, callee_method_id);
              callee_node->AddChild(current_method_node);  // Callee affects current method
              last_invoked_method_node = callee_node;
              break;
            }
            case Instruction::INVOKE_DIRECT_RANGE:
            case Instruction::INVOKE_INTERFACE_RANGE:
            case Instruction::INVOKE_STATIC_RANGE:
            case Instruction::INVOKE_SUPER_RANGE:
            case Instruction::INVOKE_VIRTUAL_RANGE: {
              uint32_t callee_method_id = inst.VRegB_3rc();
              DependencyNode* callee_node = GetOrCreateNode(NodeType::kMethod, callee_method_id);
              callee_node->AddChild(current_method_node);  // Callee affects current method
              last_invoked_method_node = callee_node;
              break;
            }
            case Instruction::MOVE_RESULT:
            case Instruction::MOVE_RESULT_WIDE:
            case Instruction::MOVE_RESULT_OBJECT: {
              register_sources[inst.VRegA_11x()] = last_invoked_method_node;
              break;
            }
            case Instruction::MOVE:
            case Instruction::MOVE_FROM16:
            case Instruction::MOVE_16:
            case Instruction::MOVE_WIDE:
            case Instruction::MOVE_WIDE_FROM16:
            case Instruction::MOVE_WIDE_16:
            case Instruction::MOVE_OBJECT:
            case Instruction::MOVE_OBJECT_16:
            case Instruction::MOVE_OBJECT_FROM16: {
              register_sources[inst.VRegA()] = register_sources[inst.VRegB()];
              break;
            }
            case Instruction::SPUT_BOOLEAN:
            case Instruction::SPUT_BYTE:
            case Instruction::SPUT_CHAR:
            case Instruction::SPUT_SHORT:
            case Instruction::SPUT:
            case Instruction::SPUT_WIDE:
            case Instruction::SPUT_OBJECT: {
              uint32_t field_id = inst.VRegB_21c();
              DependencyNode* field_node = GetOrCreateNode(NodeType::kField, field_id);
              uint32_t source_reg = inst.VRegA_21c();
              DependencyNode* value_source_node = register_sources[source_reg];
              if (value_source_node != nullptr) {
                value_source_node->AddChild(field_node);
              } else {
                current_method_node->AddChild(field_node);
              }
              break;
            }
            case Instruction::SGET_BOOLEAN:
            case Instruction::SGET_BYTE:
            case Instruction::SGET_CHAR:
            case Instruction::SGET_SHORT:
            case Instruction::SGET:
            case Instruction::SGET_WIDE:
            case Instruction::SGET_OBJECT: {
              uint32_t field_id = inst.VRegB_21c();
              DependencyNode* field_node = GetOrCreateNode(NodeType::kField, field_id);
              field_node->AddChild(current_method_node);  // Field affects current method
              break;
            }
            default:
              break;
          }
        }
      }
    }
  }
}

void DependencyGraph::Dump(const std::vector<std::unique_ptr<VeridexResolver>>& resolvers) const {
  auto get_description = [&](NodeType type, uint32_t id) -> std::string {
    for (const auto& resolver : resolvers) {
      const DexFile& dex_file = resolver->GetDexFile();
      if (type == NodeType::kMethod && id < dex_file.NumMethodIds()) {
        return dex_file.PrettyMethod(id);
      } else if (type == NodeType::kField && id < dex_file.NumFieldIds()) {
        return dex_file.PrettyField(id);
      }
    }
    return std::to_string(id);
  };

  std::cout << "---- Dependency Graph Dump ----" << std::endl;
  for (const auto& pair : nodes_) {
    const DependencyNode* node = pair.second.get();
    std::cout << (node->GetType() == NodeType::kMethod ? "Method" : "Field") << "@" << node->GetId()
              << " " << get_description(node->GetType(), node->GetId())
              << " (is_aconfig_source: " << (node->IsAconfigSource() ? "true" : "false") << ")"
              << std::endl;
    const auto& children = node->GetChildren();
    if (!children.empty()) {
      std::cout << "  Children:" << std::endl;
      for (const DependencyNode* child : children) {
        std::cout << "    -> " << (child->GetType() == NodeType::kMethod ? "Method" : "Field")
                  << " " << get_description(child->GetType(), child->GetId()) << std::endl;
      }
    }
  }
  std::cout << "-----------------------------" << std::endl;
}

}  // namespace art
