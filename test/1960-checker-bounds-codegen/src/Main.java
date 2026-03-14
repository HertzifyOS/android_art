/*
 * Copyright (C) 2019 The Android Open Source Project
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

/**
 * Test code generation for BoundsCheck.
 */
public class Main {
  // Constant index, variable length.
  /// CHECK-START-ARM64: int Main.constantIndex(int[]) disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK:                     cmp {{w\d+}}, #0x0
  /// CHECK:                     b.ls #+0x{{[0-9a-f]+}} (addr 0x<<SLOW:[0-9a-f]+>>)
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                0x<<SLOW>>:
  /// CHECK-START-ARM: int Main.constantIndex(int[]) disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK:                     cmp {{r\d+}}, #0
  /// CHECK:                     bls <<SLOW:0x[0-9a-f]+>>
  /// CHECK:                     BoundsCheckSlowPathARMVIXL
  /// CHECK-NEXT:                <<SLOW>>:
  public static int constantIndex(int[] a) {
    try {
      a[0] = 42;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals(String.format("length=%d; index=0", a.length), expected.getMessage());
      }
      return -1;
    }
    return a.length;
  }

  // Constant length, variable index.
  /// CHECK-START-ARM64: int Main.constantLength(int) disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK:                     cmp {{w\d+}}, #0xa
  /// CHECK:                     b.hs #+0x{{[0-9a-f]+}} (addr 0x<<SLOW:[0-9a-f]+>>)
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                0x<<SLOW>>:
  /// CHECK-START-ARM: int Main.constantLength(int) disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK:                     cmp {{r\d+}}, #10
  /// CHECK:                     bcs <<SLOW:0x[0-9a-f]+>>
  /// CHECK:                     BoundsCheckSlowPathARMVIXL
  /// CHECK-NEXT:                <<SLOW>>:
  public static int constantLength(int index) {
    int[] a = new int[10];
    try {
      a[index] = 1;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals(String.format("length=10; index=%d", index), expected.getMessage());
      }
      return -1;
    }
    return index;
  }

  // Constant index and length, out of bounds access. Check that we only have
  // the slow path.
  /// CHECK-START-ARM64: int Main.constantIndexAndLength() disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK-NOT:                 cmp
  /// CHECK:                     b #+0x{{[0-9a-f]+}} (addr 0x<<SLOW:[0-9a-f]+>>)
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                0x<<SLOW>>:
  /// CHECK-START-ARM: int Main.constantIndexAndLength() disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK-NOT:                 cmp
  /// CHECK:                     b <<SLOW:0x[0-9a-f]+>>
  /// CHECK:                     BoundsCheckSlowPathARMVIXL
  /// CHECK-NEXT:                <<SLOW>>:
  public static int constantIndexAndLength() {
    try {
      int[] a = new int[5];
      a[10] = 42;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals("length=5; index=10", expected.getMessage());
      }
      return -1;
    }
    return 0;
  }

  // Test that we encode arguments correctly for faulting bounds check slow paths.
  // Currently, the argument payload is 6 bits and can be either a register number
  // or a signed constant. So we cover the following cases:
  // - Array length/index are max/min encodable signed constants (fit in 6 bits)
  // - Array length/index are non-encodable signed constants (larger than 6 bits):
  //   additional moves are generated in this case
  // - Array length/index are non-constant values

  /// CHECK-START-ARM64: int Main.encodableConstantMinIndexAndMaxLength() disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK-NEXT:                b
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                udf
  public static int encodableConstantMinIndexAndMaxLength() {
    try {
      int[] a = new int[31];
      a[-32] = 42;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals("length=31; index=-32", expected.getMessage());
      }
      return -1;
    }
    return 0;
  }

  /// CHECK-START-ARM64: int Main.nonEncodableConstantIndexAndLength() disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK-NEXT:                b
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                mov x16, #0xffffffffffffffdf
  /// CHECK-NEXT:                mov x17, #0x20
  /// CHECK-NEXT:                udf
  public static int nonEncodableConstantIndexAndLength() {
    try {
      int[] a = new int[32];
      a[-33] = 42;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals("length=32; index=-33", expected.getMessage());
      }
      return -1;
    }
    return 0;
  }

  /// CHECK-START-ARM64: int Main.variableIndexAndLength(int[], int) disassembly (after)
  /// CHECK:                     BoundsCheck
  /// CHECK-NEXT:                cmp {{w\d+}}, {{w\d+}}
  /// CHECK:                     BoundsCheckSlowPathARM64
  /// CHECK-NEXT:                udf
  public static int variableIndexAndLength(int[] a, int index) {
    try {
      a[index] = 42;
    } catch (ArrayIndexOutOfBoundsException expected) {
      if (!usingRI) {
        expectEquals(String.format("length=%d; index=%d", a.length, index), expected.getMessage());
      }
      return -1;
    }
    return 0;
  }

  public static void main(String[] args) {
    try {
      Class.forName("dalvik.system.PathClassLoader");
    } catch (ClassNotFoundException e) {
      usingRI = true;
    }

    int[] a = new int[10];
    int[] b = new int[0];
    expectEquals(a.length, constantIndex(a));
    expectEquals(-1, constantIndex(b));
    expectEquals(0, constantLength(0));
    expectEquals(9, constantLength(9));
    expectEquals(-1, constantLength(10));
    expectEquals(-1, constantLength(-2));
    expectEquals(-1, constantIndexAndLength());
    expectEquals(-1, encodableConstantMinIndexAndMaxLength());
    expectEquals(-1, nonEncodableConstantIndexAndLength());
    expectEquals(-1, variableIndexAndLength(a, 100));
    System.out.println("passed");
  }

  private static void expectEquals(int expected, int result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  private static void expectEquals(String expected, String result) {
    if (!expected.equals(result)) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  static boolean usingRI = false;
}
