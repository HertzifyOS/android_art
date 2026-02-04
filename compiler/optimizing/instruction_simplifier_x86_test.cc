/*
 * Copyright (C) 2026 The Android Open Source Project
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

#include "instruction_simplifier_x86.h"

#include "base/globals.h"
#include "com_android_art_flags.h"
#include "instruction_simplifier_x86_shared_test.h"

namespace art HIDDEN {
namespace x86 {

class InstructionSimplifierX86Test : public InstructionSimplifierX86SharedTest {
 public:
  InstructionSet GetInstructionSet() const override { return InstructionSet::kX86; }

  void RunSimplifier(CodeGenerator* codegen) override {
    InstructionSimplifierX86(graph_, codegen, /*stats=*/ nullptr).Run();
  }
};

TEST_F(InstructionSimplifierX86Test, AddToLea32WithDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea32TestShifts) {
    bool expect_lea = (0u < shift && shift < 4u);
    LeaExpect expect = expect_lea ? LeaExpect::kLeaWithDisp : LeaExpect::kNoLea;
    for (bool lhs_shift : {false, true}) {
      for (int32_t disp : kLea32TestDisplacements) {
        TestAddToLeaSimplification(
            expect, LeaBaseOption::kNoBase, DataType::Type::kInt32, shift, lhs_shift, disp);
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, AddToLea64WithDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea64TestShifts) {
    for (bool lhs_shift : {false, true}) {
      for (int64_t disp : kLea64TestDisplacements) {
        LeaExpect expect = LeaExpect::kNoLea;  // No 64-bit LEA on 32-bit x86.
        TestAddToLeaSimplification(
            expect, LeaBaseOption::kNoBase, DataType::Type::kInt64, shift, lhs_shift, disp);
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, AddToLea32WithBaseAndDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea32TestShifts) {
    bool expect_lea = (0u < shift && shift < 4u);
    for (LeaBaseOption base_opt : kLeaBaseOptionsWithBase) {
      LeaExpect expect = expect_lea
          ? (base_opt == LeaBaseOption::kDispMinusBase ? LeaExpect::kLeaWithBase
                                                       : LeaExpect::kLeaWithBaseAndDisp)
          : LeaExpect::kNoLea;
      for (bool lhs_shift : {false, true}) {
        for (int32_t disp : kLea32TestDisplacements) {
          TestAddToLeaSimplification(
              expect, base_opt, DataType::Type::kInt32, shift, lhs_shift, disp);
        }
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, AddToLea64WithBaseAndDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea64TestShifts) {
    for (LeaBaseOption base_opt : kLeaBaseOptionsWithBase) {
      for (bool lhs_shift : {false, true}) {
        for (int64_t disp : kLea64TestDisplacements) {
          LeaExpect expect = LeaExpect::kNoLea;  // No 64-bit LEA on 32-bit x86.
          TestAddToLeaSimplification(
              expect, base_opt, DataType::Type::kInt64, shift, lhs_shift, disp);
        }
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, SubToLea32WithDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea32TestShifts) {
    for (bool lhs_shift : {false, true}) {
      bool expect_lea = lhs_shift && (0u < shift && shift < 4u);
      LeaExpect expect = expect_lea ? LeaExpect::kLeaWithDisp : LeaExpect::kNoLea;
      for (int32_t disp : kLea32TestDisplacements) {
        TestSubToLeaSimplification(
            expect, LeaBaseOption::kNoBase, DataType::Type::kInt32, shift, lhs_shift, disp);
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, SubToLea64WithDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea64TestShifts) {
    for (bool lhs_shift : {false, true}) {
      for (int64_t disp : kLea64TestDisplacements) {
        LeaExpect expect = LeaExpect::kNoLea;  // No 64-bit LEA on 32-bit x86.
        TestSubToLeaSimplification(
            expect, LeaBaseOption::kNoBase, DataType::Type::kInt64, shift, lhs_shift, disp);
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, SubToLea32WithBaseAndDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea32TestShifts) {
    for (LeaBaseOption base_opt : kLeaBaseOptionsWithBase) {
      for (bool lhs_shift : {false, true}) {
        for (int32_t disp : kLea32TestDisplacements) {
          TestSubToLeaSimplification(
              LeaExpect::kNoLea, base_opt, DataType::Type::kInt32, shift, lhs_shift, disp);
        }
      }
    }
  }
}

TEST_F(InstructionSimplifierX86Test, SubToLea64WithBaseAndDisp) {
  if (!com::android::art::flags::x86_lea_optimizations()) {
    GTEST_SKIP() << "x86 LEA optimizations disabled.";
  }
  for (uint32_t shift : kLea64TestShifts) {
    for (LeaBaseOption base_opt : kLeaBaseOptionsWithBase) {
      for (bool lhs_shift : {false, true}) {
        for (int64_t disp : kLea64TestDisplacements) {
          TestSubToLeaSimplification(
              LeaExpect::kNoLea, base_opt, DataType::Type::kInt64, shift, lhs_shift, disp);
        }
      }
    }
  }
}

}  // namespace x86
}  // namespace art
