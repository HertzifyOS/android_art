/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "control_flow_simplifier.h"

#include "base/arena_allocator.h"
#include "base/macros.h"
#include "builder.h"
#include "com_android_art_rw_flags.h"
#include "nodes.h"
#include "optimizing_unit_test.h"
#include "side_effects_analysis.h"

namespace art HIDDEN {

class ControlFlowSimplifierTest : public OptimizingUnitTest {
 protected:
  HPhi* ConstructBasicGraphForSelect(HBasicBlock* return_block, HInstruction* instr) {
    HParameterValue* bool_param = MakeParam(DataType::Type::kBool);
    HIntConstant* const1 =  graph_->GetIntConstant(1);

    auto [if_block, then_block, else_block] = CreateDiamondPattern(return_block, bool_param);

    AddOrInsertInstruction(then_block, instr);
    HPhi* phi = MakePhi(return_block, {instr, const1});
    return phi;
  }

  bool CheckGraphAndTryControlFlowSimplifier() {
    graph_->BuildDominatorTree();
    EXPECT_TRUE(CheckGraph());
    return HControlFlowSimplifier(graph_, /*handles*/ nullptr, /*stats*/ nullptr).Run();
  }

  template <typename T>
  bool EntriesMatch(HLoadConstantTableEntry* lcte, ArrayRef<const T> entries) {
    if (entries.size() != lcte->GetNumEntries()) {
      return false;
    }
    for (size_t i : Range(entries.size())) {
      int64_t expected;
      if constexpr (std::is_same_v<T, float>) {
        expected = bit_cast<uint32_t, float>(entries[i]);
      } else if constexpr (std::is_same_v<T, double>) {
        expected = bit_cast<uint64_t, double>(entries[i]);
      } else {
        expected = entries[i];
      }
      if (lcte->GetEntry(i) != expected) {
        return false;
      }
    }
    return true;
  }

  template <typename T>
  void TestSwitchToTable(DataType::Type type, ArrayRef<const T> entries, int32_t start_value);

  template <typename T, size_t num_entries>
  void TestSwitchToTable(DataType::Type type,
                         const T (&entries)[num_entries],
                         int32_t start_value) {
    TestSwitchToTable(type, ArrayRef<const T>(entries, num_entries), start_value);
  }

  template <typename T>
  void TestSwitchToTableWithDefault(
      DataType::Type type, ArrayRef<const T> entries, int32_t start_value, T dflt);

  template <typename T, size_t num_entries>
  void TestSwitchToTableWithDefault(DataType::Type type,
                                    const T (&entries)[num_entries],
                                    int32_t start_value,
                                    T dflt) {
    TestSwitchToTableWithDefault(type, ArrayRef<const T>(entries, num_entries), start_value, dflt);
  }
};

// HDivZeroCheck might throw and should not be hoisted from the conditional to an unconditional.
TEST_F(ControlFlowSimplifierTest, testZeroCheckPreventsSelect) {
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  HParameterValue* param = MakeParam(DataType::Type::kInt32);
  HDivZeroCheck* instr = new (GetAllocator()) HDivZeroCheck(param, 0);
  HPhi* phi = ConstructBasicGraphForSelect(return_block, instr);

  ManuallyBuildEnvFor(instr, {param, graph_->GetIntConstant(1)});

  EXPECT_FALSE(CheckGraphAndTryControlFlowSimplifier());
  EXPECT_INS_RETAINED(phi);
}

// Test that ControlFlowSimplifier succeeds with HAdd.
TEST_F(ControlFlowSimplifierTest, testSelectWithAdd) {
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  HParameterValue* param = MakeParam(DataType::Type::kInt32);
  HAdd* instr = new (GetAllocator()) HAdd(DataType::Type::kInt32, param, param, /*dex_pc=*/ 0);
  HPhi* phi = ConstructBasicGraphForSelect(return_block, instr);
  EXPECT_TRUE(CheckGraphAndTryControlFlowSimplifier());
  EXPECT_INS_REMOVED(phi);
}

// Test that ControlFlowSimplifier succeeds if there is an additional `HPhi` with identical inputs.
TEST_F(ControlFlowSimplifierTest, testSelectWithAddAndExtraPhi) {
  // Create a graph with three blocks merging to the `return_block`.
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  HParameterValue* bool_param1 = MakeParam(DataType::Type::kBool);
  HParameterValue* bool_param2 = MakeParam(DataType::Type::kBool);
  HParameterValue* param = MakeParam(DataType::Type::kInt32);
  HInstruction* const0 = graph_->GetIntConstant(0);
  auto [if_block1, left, mid] = CreateDiamondPattern(return_block, bool_param1);
  HBasicBlock* if_block2 = AddNewBlock();
  if_block1->ReplaceSuccessor(mid, if_block2);
  HBasicBlock* right = AddNewBlock();
  if_block2->AddSuccessor(mid);
  if_block2->AddSuccessor(right);
  HIf* if2 = MakeIf(if_block2, bool_param2);
  right->AddSuccessor(return_block);
  MakeGoto(right);
  ASSERT_TRUE(PredecessorsEqual(return_block, {left, mid, right}));
  HAdd* add = MakeBinOp<HAdd>(right, DataType::Type::kInt32, param, param);
  HPhi* phi1 = MakePhi(return_block, {param, param, add});
  HPhi* phi2 = MakePhi(return_block, {param, const0, const0});

  // Prevent second `HSelect` match. Do not rely on the "instructions per branch" limit.
  MakeInvokeStatic(left, DataType::Type::kVoid, {}, {});

  EXPECT_TRUE(CheckGraphAndTryControlFlowSimplifier());

  ASSERT_BLOCK_RETAINED(left);
  ASSERT_BLOCK_REMOVED(mid);
  ASSERT_BLOCK_REMOVED(right);
  HInstruction* select = if2->GetPrevious();  // `HSelect` is inserted before `HIf`.
  ASSERT_TRUE(select->IsSelect());
  ASSERT_INS_RETAINED(phi1);
  ASSERT_TRUE(InputsEqual(phi1, {param, select}));
  ASSERT_INS_RETAINED(phi2);
  ASSERT_TRUE(InputsEqual(phi2, {param, const0}));
}

// Test `HSelect` optimization in an irreducible loop.
TEST_F(ControlFlowSimplifierTest, testSelectInIrreducibleLoop) {
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  auto [split, left_header, right_header, body] = CreateIrreducibleLoop(return_block);

  HParameterValue* split_param = MakeParam(DataType::Type::kBool);
  HParameterValue* bool_param = MakeParam(DataType::Type::kBool);
  HParameterValue* n_param = MakeParam(DataType::Type::kInt32);

  MakeIf(split, split_param);

  HInstruction* const0 = graph_->GetIntConstant(0);
  HInstruction* const1 = graph_->GetIntConstant(1);
  auto [left_phi, right_phi, add] =
      MakeLinearIrreducibleLoopVar(left_header, right_header, body, const1, const0, const1);
  HCondition* condition = MakeCondition(left_header, kCondGE, left_phi, n_param);
  MakeIf(left_header, condition);

  auto [if_block, then_block, else_block] = CreateDiamondPattern(body, bool_param);
  HPhi* phi = MakePhi(body, {const1, const0});

  EXPECT_TRUE(CheckGraphAndTryControlFlowSimplifier());
  HLoopInformation* loop_info = left_header->GetLoopInformation();
  ASSERT_TRUE(loop_info != nullptr);
  ASSERT_TRUE(loop_info->IsIrreducible());

  EXPECT_INS_REMOVED(phi);
  ASSERT_TRUE(if_block->GetFirstInstruction()->IsSelect());

  ASSERT_EQ(if_block, add->GetBlock());  // Moved when merging blocks.

  for (HBasicBlock* removed_block : {then_block, else_block, body}) {
    ASSERT_BLOCK_REMOVED(removed_block);
    uint32_t removed_block_id = removed_block->GetBlockId();
    ASSERT_FALSE(loop_info->GetBlockMask().IsBitSet(removed_block_id)) << removed_block_id;
  }
}

template <typename T>
void ControlFlowSimplifierTest::TestSwitchToTable(DataType::Type type,
                                                  ArrayRef<const T> entries,
                                                  int32_t start_value) {
  HBasicBlock* return_block = InitEntryMainExitGraph();
  HInstruction* switch_input = MakeParam(DataType::Type::kInt32);
  HInstruction* default_value = MakeParam(type);
  HBasicBlock* switch_block =
      CreateSwitchPattern(return_block, entries.size(), switch_input, start_value);
  HPackedSwitch* packed_switch = switch_block->GetLastInstruction()->AsPackedSwitch();
  std::vector<HInstruction*> phi_inputs;
  for (T entry : entries) {
    phi_inputs.push_back(GetConstant(type, entry));
  }
  phi_inputs.push_back(default_value);
  HPhi* phi = MakePhi(return_block, phi_inputs);
  HReturn* ret = MakeReturn(return_block, phi);

  bool success = CheckGraphAndTryControlFlowSimplifier();
  ASSERT_TRUE(success);

  // Check `HPackedSwitch` replacement.
  ASSERT_INS_REMOVED(packed_switch);
  ASSERT_TRUE(switch_block->GetLastInstruction()->IsIf());
  HInstruction* cond = switch_block->GetLastInstruction()->AsIf()->InputAt(0);
  ASSERT_TRUE(cond->IsBelow());
  HInstruction* lhs = cond->AsBelow()->GetLeft();
  HInstruction* rhs = cond->AsBelow()->GetRight();
  if (start_value != 0) {
    ASSERT_TRUE(lhs->IsAdd());
    ASSERT_INS_EQ(switch_input, lhs->AsAdd()->GetLeft());
    ASSERT_TRUE(lhs->AsAdd()->GetRight()->IsIntConstant());
    ASSERT_EQ(-start_value, lhs->AsAdd()->GetRight()->AsIntConstant()->GetValue());
  } else {
    ASSERT_INS_EQ(switch_input, lhs);
  }
  ASSERT_TRUE(rhs->IsIntConstant());
  EXPECT_EQ(dchecked_integral_cast<int32_t>(entries.size()), rhs->AsIntConstant()->GetValue());
  // Check `HLoadConstantTableEntry`.
  ASSERT_INS_EQ(phi, ret->InputAt(0));
  ASSERT_EQ(2u, phi->InputCount());
  ASSERT_TRUE(phi->InputAt(0)->IsLoadConstantTableEntry());
  HLoadConstantTableEntry* lcte = phi->InputAt(0)->AsLoadConstantTableEntry();
  EXPECT_EQ(type, lcte->GetType());
  ASSERT_TRUE(EntriesMatch(lcte, entries));
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableBoolean) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {1, 0, 1, 1, 0, 0, 0, 1, 0, 1};
  TestSwitchToTable(DataType::Type::kBool, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kBool, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableByte) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {42, 88, -7, 11, 123};
  TestSwitchToTable(DataType::Type::kInt8, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kInt8, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableUint8) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {42, 88, 7, 11, 255};
  TestSwitchToTable(DataType::Type::kUint8, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kUint8, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableShort) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {42, 88, -7, 11, 12345};
  TestSwitchToTable(DataType::Type::kInt16, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kInt16, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableChar) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {42, 88, 7, 11, 54321};
  TestSwitchToTable(DataType::Type::kUint16, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kUint16, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableInt) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kEntries[] = {42, 88, -7, 11, 123456789};
  TestSwitchToTable(DataType::Type::kInt32, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kInt32, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableLong) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int64_t kEntries[] = {42, 88, -7, 11, INT64_C(123456789987654321)};
  TestSwitchToTable(DataType::Type::kInt64, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kInt64, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableFloat) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static constexpr float nan = std::numeric_limits<float>::quiet_NaN();
  static const float kEntries[] = {42.0f, 88.0f, -7.0f, 11.0f, 123456789.0f, nan};
  TestSwitchToTable(DataType::Type::kFloat32, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kFloat32, kEntries, /*start_value=*/ 1);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableDouble) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static constexpr double nan = std::numeric_limits<double>::quiet_NaN();
  static const double kEntries[] = {42.0, 88.0, -7.0, 11.0, 123456789.0, nan};
  TestSwitchToTable(DataType::Type::kFloat64, kEntries, /*start_value=*/ 0);
  TestSwitchToTable(DataType::Type::kFloat64, kEntries, /*start_value=*/ 1);
}

template <typename T>
void ControlFlowSimplifierTest::TestSwitchToTableWithDefault(DataType::Type type,
                                                             ArrayRef<const T> entries,
                                                             int32_t start_value,
                                                             T dflt) {
  HBasicBlock* return_block = InitEntryMainExitGraph();
  HInstruction* switch_input = MakeParam(DataType::Type::kInt32);
  HInstruction* default_value = GetConstant(type, dflt);
  HBasicBlock* switch_block =
      CreateSwitchPattern(return_block, entries.size(), switch_input, start_value);
  HPackedSwitch* packed_switch = switch_block->GetLastInstruction()->AsPackedSwitch();
  std::vector<HInstruction*> phi_inputs;
  for (T entry : entries) {
    phi_inputs.push_back(GetConstant(type, entry));
  }
  phi_inputs.push_back(default_value);
  HPhi* phi = MakePhi(return_block, phi_inputs);
  HReturn* ret = MakeReturn(return_block, phi);

  bool success = CheckGraphAndTryControlFlowSimplifier();
  ASSERT_TRUE(success);

  // The packed switch and phi have been replaced by the load from constant table.
  ASSERT_INS_REMOVED(packed_switch);
  ASSERT_INS_REMOVED(phi);
  ASSERT_TRUE(ret->InputAt(0)->IsLoadConstantTableEntry());
  HLoadConstantTableEntry* lcte = ret->InputAt(0)->AsLoadConstantTableEntry();
  EXPECT_EQ(type, lcte->GetType());
  std::vector<T> expected_entries(entries.begin(), entries.end());
  expected_entries.insert(
      start_value == 1 ? expected_entries.begin() : expected_entries.end(), dflt);
  ASSERT_TRUE(EntriesMatch(lcte, ArrayRef<const T>(expected_entries)));
  // The `lcte->GetIndex()` must be a `HSelect` between a raw index and default index.
  ASSERT_TRUE(lcte->GetIndex()->IsSelect());
  HSelect* select = lcte->GetIndex()->AsSelect();
  HInstruction* raw_index = select->GetTrueValue();
  HInstruction* default_index = select->GetFalseValue();
  int32_t expected_default_index = start_value == 1 ? 0 : static_cast<int32_t>(entries.size());
  ASSERT_TRUE(default_index->IsIntConstant());
  ASSERT_EQ(expected_default_index, default_index->AsIntConstant()->GetValue());
  // The select condition must be `Below (raw_index, entries.size() + <0 or 1>)`.
  ASSERT_TRUE(select->GetCondition()->IsBelow());
  HBelow* below = select->GetCondition()->AsBelow();
  ASSERT_TRUE(below->GetRight()->IsIntConstant());
  ASSERT_EQ(static_cast<int32_t>(entries.size()) + (start_value == 1 ? 1 : 0),
            below->GetRight()->AsIntConstant()->GetValue());
  ASSERT_INS_EQ(raw_index, below->GetLeft());
  // Check raw index.
  if (start_value != 0 && start_value != 1) {
    ASSERT_TRUE(raw_index->IsAdd());
    ASSERT_INS_EQ(switch_input, raw_index->AsAdd()->GetLeft());
    ASSERT_TRUE(raw_index->AsAdd()->GetRight()->IsIntConstant());
    ASSERT_EQ(-start_value, raw_index->AsAdd()->GetRight()->AsIntConstant()->GetValue());
  } else {
    ASSERT_INS_EQ(switch_input, raw_index);
  }
  // The `return_block` has been merged into `switch_block`.
  ASSERT_BLOCK_REMOVED(return_block);
  ASSERT_TRUE(switch_block->EndsWithReturn());
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableBooleanWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = 0;
  static const int32_t kEntries[] = {1, 0, 1, 1, 0, 0, 0, 1, 0, 1};
  TestSwitchToTableWithDefault(DataType::Type::kBool, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kBool, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kBool, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableByteWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = -42;
  static const int32_t kEntries[] = {42, 88, -7, 11, 123};
  TestSwitchToTableWithDefault(DataType::Type::kInt8, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt8, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt8, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableUint8WithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = 43;
  static const int32_t kEntries[] = {42, 88, 7, 11, 255};
  TestSwitchToTableWithDefault(DataType::Type::kUint8, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kUint8, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kUint8, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableShortWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = -42;
  static const int32_t kEntries[] = {42, 88, -7, 11, 12345};
  TestSwitchToTableWithDefault(DataType::Type::kInt16, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt16, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt16, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableCharWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = 43;
  static const int32_t kEntries[] = {42, 88, 7, 11, 54321};
  TestSwitchToTableWithDefault(DataType::Type::kUint16, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kUint16, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kUint16, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableIntWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int32_t kDefault = -42;
  static const int32_t kEntries[] = {42, 88, -7, 11, 123456789};
  TestSwitchToTableWithDefault(DataType::Type::kInt32, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt32, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt32, kEntries, /*start_value=*/ 3, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableLongWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const int64_t kDefault = -42;
  static const int64_t kEntries[] = {42, 88, -7, 11, INT64_C(123456789987654321)};
  TestSwitchToTableWithDefault(DataType::Type::kInt64, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt64, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kInt64, kEntries, /*start_value=*/ 2, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableFloatWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const float kDefault = -42.0f;
  static constexpr float nan = std::numeric_limits<float>::quiet_NaN();
  static const float kEntries[] = {42.0f, 88.0f, -7.0f, 11.0f, 123456789.0f, nan};
  TestSwitchToTableWithDefault(DataType::Type::kFloat32, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kFloat32, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kFloat32, kEntries, /*start_value=*/ 2, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchToTableDoubleWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static const double kDefault = -42.0;
  static constexpr double nan = std::numeric_limits<double>::quiet_NaN();
  static const double kEntries[] = {42.0, 88.0, -7.0, 11.0, 123456789.0, nan};
  TestSwitchToTableWithDefault(DataType::Type::kFloat64, kEntries, /*start_value=*/ 0, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kFloat64, kEntries, /*start_value=*/ 1, kDefault);
  TestSwitchToTableWithDefault(DataType::Type::kFloat64, kEntries, /*start_value=*/ 2, kDefault);
}

TEST_F(ControlFlowSimplifierTest, SwitchReplacedByIf) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static constexpr size_t kNumEntries = 5;
  static constexpr int32_t kStartValue = 1;
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  HInstruction* switch_input = MakeParam(DataType::Type::kInt32);
  HBasicBlock* switch_block =
      CreateSwitchPattern(return_block, kNumEntries, switch_input, kStartValue);
  HPackedSwitch* packed_switch = switch_block->GetLastInstruction()->AsPackedSwitch();
  MakeInvokeStatic(packed_switch->GetDefaultBlock(), DataType::Type::kVoid, {}, {});

  bool success = CheckGraphAndTryControlFlowSimplifier();
  ASSERT_TRUE(success);

  ASSERT_INS_REMOVED(packed_switch);
  ASSERT_TRUE(switch_block->GetLastInstruction()->IsIf());
  ASSERT_TRUE(switch_block->GetLastInstruction()->AsIf()->InputAt(0)->IsBelow());
  HBelow* below = switch_block->GetLastInstruction()->AsIf()->InputAt(0)->AsBelow();
  ASSERT_TRUE(below->GetRight()->IsIntConstant());
  ASSERT_EQ(static_cast<int32_t>(kNumEntries), below->GetRight()->AsIntConstant()->GetValue());
  ASSERT_TRUE(below->GetLeft()->IsAdd());
  HAdd* add = below->GetLeft()->AsAdd();
  ASSERT_TRUE(add->GetRight()->IsIntConstant());
  ASSERT_EQ(-kStartValue, add->GetRight()->AsIntConstant()->GetValue());
  ASSERT_INS_EQ(switch_input, add->GetLeft());
  ASSERT_BLOCK_RETAINED(return_block);
}

TEST_F(ControlFlowSimplifierTest, SwitchEliminatedWithDefault) {
  if (!com::android::art::rw::flags::packed_switch_simplification()) {
    GTEST_SKIP() << "packed switch simplification disabled.";
  }
  static constexpr size_t kNumEntries = 5;
  static constexpr int32_t kStartValue = 1;
  HBasicBlock* return_block = InitEntryMainExitGraphWithReturnVoid();
  HInstruction* switch_input = MakeParam(DataType::Type::kInt32);
  HBasicBlock* switch_block =
      CreateSwitchPattern(return_block, kNumEntries, switch_input, kStartValue);
  HPackedSwitch* packed_switch = switch_block->GetLastInstruction()->AsPackedSwitch();

  bool success = CheckGraphAndTryControlFlowSimplifier();
  ASSERT_TRUE(success);

  ASSERT_INS_REMOVED(packed_switch);
  ASSERT_TRUE(switch_block->GetLastInstruction()->IsReturnVoid());
  ASSERT_BLOCK_REMOVED(return_block);
}

}  // namespace art
