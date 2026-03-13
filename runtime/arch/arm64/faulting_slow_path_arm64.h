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

#ifndef ART_RUNTIME_ARCH_ARM64_FAULTING_SLOW_PATH_ARM64_H_
#define ART_RUNTIME_ARCH_ARM64_FAULTING_SLOW_PATH_ARM64_H_

#include "base/bit_field.h"
#include "base/bit_utils.h"

namespace art {

// Slow paths for which the compiler can emit a faulting instruction (e.g., UDF on Arm64) instead
// of a call to a quick entrypoint in order to reduce code size.
enum class FaultingSlowPath {
  // To ensure the high bits of the faulting-instruction argument field are always set, the
  // deoptimization type tag is encoded as 0. Because the deoptimization slow path takes only one
  // parameter, the second argument can be set to all ones in this case. For other slow path types
  // the arguments may be 0, so they must use a non-zero type tag. See the comment on
  // Arm64FaultingSlowPathArguments for more information.
  kDeoptimize = 0,
  kArrayBoundsCheck,
  kStringBoundsCheck,
  kTypeCheck,
  kLast = kTypeCheck,
};
std::ostream& operator<<(std::ostream& os, FaultingSlowPath rhs);

inline size_t GetNumberOfSlowPathArguments(FaultingSlowPath slow_path) {
  switch (slow_path) {
    case FaultingSlowPath::kDeoptimize:
      return 1;
    case FaultingSlowPath::kArrayBoundsCheck:
    case FaultingSlowPath::kStringBoundsCheck:
    case FaultingSlowPath::kTypeCheck:
      return 2;
  }
}

// This class is used to encode the context needed to call a quick entrypoint from a signal handler
// into the immediate operand of a UDF instruction. In includes a slow path type and up to two
// agruments, which can be either constants or registers.
//
// The encoded context has the following layout:
//
//  15   14 13  12                   7   6   5                   0
// +-------+---+-----------------------+---+-----------------------+
// |   ST  |T_1|        Arg_1          |T_0|        Arg_0          |
// +-------+---+-----------------------+---+-----------------------+
//
// where:
// * ST: slow path type - the ordinal value of FaultingSlowPath (2 bits)
// * Arg_0 / 1: arguments payload (6 bits). For registers, only 5 bits are used, the MSB is always
//   set to 1.
// * T_0 / 1: argument types, indicating how to interpret the payload (1 bit):
//     0 - register number
//     1 - constant
//
// The encoding is chosen to ensure that at least one of the top bits (15:12) of the immediate is
// non-zero. This helps distinguish between UDF instructions from native code and those generated
// by ART. Although the runtime relies on the source of the signal (i.e., it calls ART handlers
// only for signals from generated code), using a different encoding can still be helpful during
// debugging.
class Arm64FaultingSlowPathArguments {
 public:
  using DataType = uint16_t;

  class Arg {
   public:
    enum class Type {
      // To ensure the top bits are always set, the register type is encoded as 0. Since a register
      // number requires fewer bits than a constant, the top payload bit can be set to 1 in this
      // case. See the comment for Arm64FaultingSlowPathArguments for the more information.
      kRegister = 0,
      kConstant,
      kLast = kConstant,
    };
    friend std::ostream& operator<<(std::ostream& os, Type rhs);

    static constexpr uintptr_t kBitSize = 7;

    explicit Arg(uintptr_t data) : data_(data & kMask) { DCHECK(IsValid(data)); }

    Type GetType() const { return TypeField::Decode(data_); }

    int64_t GetConstant() const {
      DCHECK_EQ(GetType(), Type::kConstant);
      uintptr_t value = PayloadField::Decode(data_);
      using R = decltype(GetConstant());
      constexpr size_t kShift = BitSizeOf<R>() - PayloadField::BitSize();
      return static_cast<R>(value) << kShift >> kShift;
    }

    uint32_t GetRegister() const {
      DCHECK_EQ(GetType(), Type::kRegister);
      return PayloadField::Decode(data_) & kRegNumMask;
    }

    static bool IsEncodableConstant(int64_t value) { return IsInt<PayloadField::BitSize()>(value); }

    static bool IsValidRegister(uint32_t regnum) { return IsUint<kRegNumSize>(regnum); }

    static Arg Constant(int64_t value) {
      DCHECK(IsEncodableConstant(value)) << value;
      uintptr_t data =
          PayloadField::Update(static_cast<uintptr_t>(value) & PayloadField::Mask(), 0);
      return Arg(TypeField::Update(Type::kConstant, data));
    }

    static Arg Register(uint32_t regnum) {
      DCHECK(IsValidRegister(regnum)) << regnum;
      // Set top unused bits to 1s.
      regnum |= BitFieldClear(PayloadField::Mask(), 0, kRegNumSize);
      uintptr_t data = PayloadField::Update(regnum, 0);
      return Arg(TypeField::Update(Type::kRegister, data));
    }

    // Necessary to be able to use BitField with the Arg type as BitField::Update uses static cast
    // to uintptr_t.
    operator uintptr_t() const { return data_; }

    static bool IsValid(uintptr_t data) {
      Type type = TypeField::Decode(data);
      if (type == Type::kRegister) {
        // Top bits of the payload should be set to 1s.
        return (PayloadField::Decode(data) | kRegNumMask) == PayloadField::Mask();
      } else {
        DCHECK_EQ(type, Type::kConstant);
        // For constants, the entire payload is the value, so any bit pattern is valid.
        return true;
      }
    }

   private:
    static constexpr uintptr_t kMask = MaskLeastSignificant(kBitSize);

    static constexpr uintptr_t kTypeSize = MinimumBitsToStore(static_cast<uint32_t>(Type::kLast));
    static constexpr uintptr_t kPayloadSize = kBitSize - kTypeSize;

    // There are 32 core registers, so we need 5 bits to encode them.
    static constexpr uintptr_t kRegNumSize = 5;
    static constexpr uintptr_t kRegNumMask = MaskLeastSignificant(kRegNumSize);
    // We need at least one unused bit that can be set to 1.
    static_assert(kPayloadSize > kRegNumSize);

    using PayloadField = BitField<uintptr_t, 0, kPayloadSize>;
    using TypeField = BitField<Type, kPayloadSize, kTypeSize>;

    uintptr_t data_;
  };

  // We use the maximum DataType value as the default so that unused arguments are set to all 1s.
  explicit Arm64FaultingSlowPathArguments(DataType data = std::numeric_limits<DataType>::max())
      : data_(data) {
    DCHECK(IsValid(data));
  }

  FaultingSlowPath SlowPath() const { return SlowPathField::Decode(data_); }

  Arg Arg0() const { return Arg0Field::Decode(data_); }

  Arg Arg1() const { return Arg1Field::Decode(data_); }

  void SetSlowPath(FaultingSlowPath slow_path) { data_ = SlowPathField::Update(slow_path, data_); }

  Arg GetArg(size_t i) const {
    DCHECK_LT(i, kNumArgs);
    if (i == 0) {
      return Arg0Field::Decode(data_);
    } else {
      DCHECK_EQ(i, kNumArgs - 1);
      return Arg1Field::Decode(data_);
    }
  }

  void SetArg(size_t i, Arg arg) {
    DCHECK_LT(i, kNumArgs);
    if (i == 0) {
      data_ = Arg0Field::Update(arg, data_);
    } else {
      DCHECK_EQ(i, kNumArgs - 1);
      data_ = Arg1Field::Update(arg, data_);
    }
  }

  DataType Data() const { return data_; }

  static bool IsValid(DataType data) {
    FaultingSlowPath slow_path = SlowPathField::Decode(data);
    size_t num_args = GetNumberOfSlowPathArguments(slow_path);
    DCHECK_LE(num_args, kNumArgs);
    return IsValidArgs<Arg0Field, Arg1Field>(data, num_args);
  }

 private:
  template <typename ...ArgFields>
  static bool IsValidArgs(DataType data, size_t num_args) {
    static_assert(sizeof...(ArgFields) == kNumArgs);
    return (IsValidArg<ArgFields>(data, num_args) && ...);
  }

  template <typename ArgField>
  static bool IsValidArg(DataType data, size_t num_args) {
    // Arguments are placed one by one starting at bit 0 (see the comment on
    // Arm64FaultingSlowPathArguments), so we can calculate an argument's index
    // based on its bit position and size.
    size_t index = ArgField::position / ArgField::size;
    // We don't use ArgField to avoid constructing Arg from unvalidated data
    // (so we don't trigger asserts in its constructor), so we use a separate
    // BitField type to extract the argument data.
    DataType arg = BitField<DataType, ArgField::position, ArgField::size>::Decode(data);
    if (index < num_args) {
      return Arg::IsValid(arg);
    } else {
      // Unused arguments are set to 1s.
      return arg == ArgField::Mask();
    }
  }

  static constexpr size_t kNumArgs = 2;
  static constexpr size_t kSlowPathBitSize =
      MinimumBitsToStore(static_cast<uint32_t>(FaultingSlowPath::kLast));
  static_assert(Arg::kBitSize * kNumArgs + kSlowPathBitSize <= BitSizeOf<DataType>());
  using Arg0Field = BitField<Arg, 0, Arg::kBitSize>;
  using Arg1Field = BitField<Arg, Arg::kBitSize, Arg::kBitSize>;
  using SlowPathField = BitField<FaultingSlowPath, 2 * Arg::kBitSize, kSlowPathBitSize>;
  uintptr_t data_;
};

}  // namespace art

#endif  // ART_RUNTIME_ARCH_ARM64_FAULTING_SLOW_PATH_ARM64_H_
