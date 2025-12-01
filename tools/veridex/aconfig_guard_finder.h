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

#ifndef ART_TOOLS_VERIDEX_ACONFIG_GUARD_FINDER_H_
#define ART_TOOLS_VERIDEX_ACONFIG_GUARD_FINDER_H_

#include "class_filter.h"
#include "dependency_graph.h"
#include "flow_analysis.h"

namespace art {

class AconfigGuardFinder : public VeriFlowAnalysis {
 public:
  AconfigGuardFinder(VeridexResolver* resolver,
                     const ClassAccessor::Method& method,
                     DependencyGraph* dependency_graph);

  bool IsPcGuarded(uint32_t dex_pc) const;

  RegisterValue AnalyzeInvoke(uint32_t dex_pc,
                              const Instruction& instruction,
                              bool is_range) override;
  void AnalyzeFieldSet(const Instruction& instruction) override;
  void ProcessBranch(uint32_t dex_pc, const Instruction& instruction) override;

 private:
  bool IsAconfigRegister(uint32_t reg) const;

  DependencyGraph* dependency_graph_;
  std::vector<std::pair<uint32_t, uint32_t>> guarded_blocks_;
};

}  // namespace art

#endif  // ART_TOOLS_VERIDEX_ACONFIG_GUARD_FINDER_H_
