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

// TODO: Due to missing flagging support in checker, checker tests for
// `com::android::art::rw::flags::packed_switch_simplification()` are disabled.
// Enable them when we implement flagging support or during the flag cleanup.
public class PackedSwitch {
    public static void main() {
        // Test creating constant tables of different types with the default case throwing `Error`.
        // Note: For narrow types, we're using `int` as the return type
        // but still expect the table to use a narrow type to save space.
        $noinline$testSwitchToTableBoolean();
        $noinline$testSwitchToTableByte();
        $noinline$testSwitchToTableUint8();
        $noinline$testSwitchToTableShort();
        $noinline$testSwitchToTableChar();
        $noinline$testSwitchToTableInt();
        $noinline$testSwitchToTableLong();
        $noinline$testSwitchToTableFloat();
        $noinline$testSwitchToTableDouble();

        // Test creating constant tables (`int` only) with the default case included.
        $noinline$testSwitchToTableIntWithDefault0();
        $noinline$testSwitchToTableIntWithDefault1();
        $noinline$testSwitchToTableIntWithDefault2();
    }

    public static void $noinline$testSwitchToTableBoolean() {
        try {
            $noinline$switchToTableBoolean(0);
        } catch (Error expected) {}
        assertEquals(1, $noinline$switchToTableBoolean(1));
        assertEquals(0, $noinline$switchToTableBoolean(2));
        assertEquals(0, $noinline$switchToTableBoolean(3));
        assertEquals(1, $noinline$switchToTableBoolean(4));
        assertEquals(0, $noinline$switchToTableBoolean(5));
        try {
            $noinline$switchToTableBoolean(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableBoolean(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableBoolean(int) control_flow_simplifier (after)
    // CHECK: {{z\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableBoolean(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableBoolean(int value) {
        final int result;
        switch (value) {
            case 1: result = 1; break;
            case 2: result = 0; break;
            case 3: result = 0; break;
            case 4: result = 1; break;
            case 5: result = 0; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableByte() {
        try {
            $noinline$switchToTableByte(0);
        } catch (Error expected) {}
        assertEquals(42, $noinline$switchToTableByte(1));
        assertEquals(88, $noinline$switchToTableByte(2));
        assertEquals(-7, $noinline$switchToTableByte(3));
        assertEquals(11, $noinline$switchToTableByte(4));
        assertEquals(123, $noinline$switchToTableByte(5));
        try {
            $noinline$switchToTableByte(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableByte(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableByte(int) control_flow_simplifier (after)
    // CHECK: {{b\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableByte(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableByte(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = -7; break;
            case 4: result = 11; break;
            case 5: result = 123; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableUint8() {
        try {
            $noinline$switchToTableUint8(0);
        } catch (Error expected) {}
        assertEquals(42, $noinline$switchToTableUint8(1));
        assertEquals(88, $noinline$switchToTableUint8(2));
        assertEquals(7, $noinline$switchToTableUint8(3));
        assertEquals(11, $noinline$switchToTableUint8(4));
        assertEquals(255, $noinline$switchToTableUint8(5));
        try {
            $noinline$switchToTableUint8(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableUint8(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableUint8(int) control_flow_simplifier (after)
    // CHECK: {{a\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableUint8(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableUint8(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = 7; break;
            case 4: result = 11; break;
            case 5: result = 255; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableShort() {
        try {
            $noinline$switchToTableShort(0);
        } catch (Error expected) {}
        assertEquals(42, $noinline$switchToTableShort(1));
        assertEquals(88, $noinline$switchToTableShort(2));
        assertEquals(-7, $noinline$switchToTableShort(3));
        assertEquals(11, $noinline$switchToTableShort(4));
        assertEquals(12345, $noinline$switchToTableShort(5));
        try {
            $noinline$switchToTableShort(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableShort(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableShort(int) control_flow_simplifier (after)
    // CHECK: {{s\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableShort(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableShort(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = -7; break;
            case 4: result = 11; break;
            case 5: result = 12345; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableChar() {
        try {
            $noinline$switchToTableChar(0);
        } catch (Error expected) {}
        assertEquals(42, $noinline$switchToTableChar(1));
        assertEquals(88, $noinline$switchToTableChar(2));
        assertEquals(7, $noinline$switchToTableChar(3));
        assertEquals(11, $noinline$switchToTableChar(4));
        assertEquals(54321, $noinline$switchToTableChar(5));
        try {
            $noinline$switchToTableChar(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableChar(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableChar(int) control_flow_simplifier (after)
    // CHECK: {{c\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableChar(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableChar(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = 7; break;
            case 4: result = 11; break;
            case 5: result = 54321; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableInt() {
        try {
            $noinline$switchToTableInt(0);
        } catch (Error expected) {}
        assertEquals(42, $noinline$switchToTableInt(1));
        assertEquals(88, $noinline$switchToTableInt(2));
        assertEquals(-7, $noinline$switchToTableInt(3));
        assertEquals(11, $noinline$switchToTableInt(4));
        assertEquals(123456789, $noinline$switchToTableInt(5));
        try {
            $noinline$switchToTableInt(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableInt(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableInt(int) control_flow_simplifier (after)
    // CHECK: {{i\d+}}     LoadConstantTableEntry

    // CHECK-START: int PackedSwitch.$noinline$switchToTableInt(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableInt(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = -7; break;
            case 4: result = 11; break;
            case 5: result = 123456789; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableLong() {
        try {
            $noinline$switchToTableLong(0);
        } catch (Error expected) {}
        assertEquals(42L, $noinline$switchToTableLong(1));
        assertEquals(88L, $noinline$switchToTableLong(2));
        assertEquals(-7L, $noinline$switchToTableLong(3));
        assertEquals(11L, $noinline$switchToTableLong(4));
        assertEquals(123456789987654321L, $noinline$switchToTableLong(5));
        try {
            $noinline$switchToTableLong(6);
        } catch (Error expected) {}
    }

    /// CHECK-START: long PackedSwitch.$noinline$switchToTableLong(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: long PackedSwitch.$noinline$switchToTableLong(int) control_flow_simplifier (after)
    // CHECK: {{j\d+}}     LoadConstantTableEntry

    // CHECK-START: long PackedSwitch.$noinline$switchToTableLong(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static long $noinline$switchToTableLong(int value) {
        final long result;
        switch (value) {
            case 1: result = 42L; break;
            case 2: result = 88L; break;
            case 3: result = -7L; break;
            case 4: result = 11L; break;
            case 5: result = 123456789987654321L; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableFloat() {
        try {
            $noinline$switchToTableFloat(0);
        } catch (Error expected) {}
        assertEquals(42.0f, $noinline$switchToTableFloat(1));
        assertEquals(88.0f, $noinline$switchToTableFloat(2));
        assertEquals(-7.0f, $noinline$switchToTableFloat(3));
        assertEquals(11.0f, $noinline$switchToTableFloat(4));
        assertEquals(123456789.0f, $noinline$switchToTableFloat(5));
        assertTrue(Float.isNaN($noinline$switchToTableFloat(6)));
        try {
            $noinline$switchToTableFloat(7);
        } catch (Error expected) {}
    }

    /// CHECK-START: float PackedSwitch.$noinline$switchToTableFloat(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: float PackedSwitch.$noinline$switchToTableFloat(int) control_flow_simplifier (after)
    // CHECK: {{f\d+}}     LoadConstantTableEntry

    // CHECK-START: float PackedSwitch.$noinline$switchToTableFloat(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static float $noinline$switchToTableFloat(int value) {
        final float result;
        switch (value) {
            case 1: result = 42.0f; break;
            case 2: result = 88.0f; break;
            case 3: result = -7.0f; break;
            case 4: result = 11.0f; break;
            case 5: result = 123456789.0f; break;
            case 6: result = Float.NaN; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableDouble() {
        try {
            $noinline$switchToTableDouble(0);
        } catch (Error expected) {}
        assertEquals(42.0, $noinline$switchToTableDouble(1));
        assertEquals(88.0, $noinline$switchToTableDouble(2));
        assertEquals(-7.0, $noinline$switchToTableDouble(3));
        assertEquals(11.0, $noinline$switchToTableDouble(4));
        assertEquals(123456789.0, $noinline$switchToTableDouble(5));
        assertTrue(Double.isNaN($noinline$switchToTableDouble(6)));
        try {
            $noinline$switchToTableDouble(7);
        } catch (Error expected) {}
    }

    /// CHECK-START: double PackedSwitch.$noinline$switchToTableDouble(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: double PackedSwitch.$noinline$switchToTableDouble(int) control_flow_simplifier (after)
    // CHECK: {{d\d+}}     LoadConstantTableEntry

    // CHECK-START: double PackedSwitch.$noinline$switchToTableDouble(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static double $noinline$switchToTableDouble(int value) {
        final double result;
        switch (value) {
            case 1: result = 42.0; break;
            case 2: result = 88.0; break;
            case 3: result = -7.0; break;
            case 4: result = 11.0; break;
            case 5: result = 123456789.0; break;
            case 6: result = Double.NaN; break;
            default: throw new Error();
        }
        return result;
    }

    public static void $noinline$testSwitchToTableIntWithDefault0() {
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault0(-1));
        assertEquals(42, $noinline$switchToTableIntWithDefault0(0));
        assertEquals(88, $noinline$switchToTableIntWithDefault0(1));
        assertEquals(-7, $noinline$switchToTableIntWithDefault0(2));
        assertEquals(11, $noinline$switchToTableIntWithDefault0(3));
        assertEquals(123456789, $noinline$switchToTableIntWithDefault0(4));
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault0(5));
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault0(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault0(int) control_flow_simplifier (after)
    // CHECK: <<Sel:i\d+>> Select
    // CHECK: {{i\d+}}     LoadConstantTableEntry [<<Sel>>]

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault0(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableIntWithDefault0(int value) {
        final int result;
        switch (value) {
            case 0: result = 42; break;
            case 1: result = 88; break;
            case 2: result = -7; break;
            case 3: result = 11; break;
            case 4: result = 123456789; break;
            default: result = -987654321; break;
        }
        return result;
    }

    public static void $noinline$testSwitchToTableIntWithDefault1() {
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault1(0));
        assertEquals(42, $noinline$switchToTableIntWithDefault1(1));
        assertEquals(88, $noinline$switchToTableIntWithDefault1(2));
        assertEquals(-7, $noinline$switchToTableIntWithDefault1(3));
        assertEquals(11, $noinline$switchToTableIntWithDefault1(4));
        assertEquals(123456789, $noinline$switchToTableIntWithDefault1(5));
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault1(6));
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault1(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault1(int) control_flow_simplifier (after)
    // CHECK: <<Sel:i\d+>> Select
    // CHECK: {{i\d+}}     LoadConstantTableEntry [<<Sel>>]

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault1(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableIntWithDefault1(int value) {
        final int result;
        switch (value) {
            case 1: result = 42; break;
            case 2: result = 88; break;
            case 3: result = -7; break;
            case 4: result = 11; break;
            case 5: result = 123456789; break;
            default: result = -987654321; break;
        }
        return result;
    }

    public static void $noinline$testSwitchToTableIntWithDefault2() {
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault2(1));
        assertEquals(42, $noinline$switchToTableIntWithDefault2(2));
        assertEquals(88, $noinline$switchToTableIntWithDefault2(3));
        assertEquals(-7, $noinline$switchToTableIntWithDefault2(4));
        assertEquals(11, $noinline$switchToTableIntWithDefault2(5));
        assertEquals(123456789, $noinline$switchToTableIntWithDefault2(6));
        assertEquals(-987654321, $noinline$switchToTableIntWithDefault2(7));
    }

    /// CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault2(int) control_flow_simplifier (before)
    /// CHECK:              PackedSwitch

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault2(int) control_flow_simplifier (after)
    // CHECK: <<Sel:i\d+>> Select
    // CHECK: {{i\d+}}     LoadConstantTableEntry [<<Sel>>]

    // CHECK-START: int PackedSwitch.$noinline$switchToTableIntWithDefault2(int) control_flow_simplifier (after)
    // CHECK-NOT:          PackedSwitch
    // CHECK-NOT:          Phi
    public static int $noinline$switchToTableIntWithDefault2(int value) {
        final int result;
        switch (value) {
            case 2: result = 42; break;
            case 3: result = 88; break;
            case 4: result = -7; break;
            case 5: result = 11; break;
            case 6: result = 123456789; break;
            default: result = -987654321; break;
        }
        return result;
    }

    public static void assertEquals(int expected, int actual) {
        if (expected != actual) {
            throw new AssertionError("Expected " + expected + " got " + actual);
        }
    }

    public static void assertEquals(long expected, long actual) {
        if (expected != actual) {
            throw new AssertionError("Expected " + expected + " got " + actual);
        }
    }

    public static void assertEquals(float expected, float actual) {
        if (expected != actual) {
            throw new AssertionError("Expected " + expected + " got " + actual);
        }
    }

    public static void assertEquals(double expected, double actual) {
        if (expected != actual) {
            throw new AssertionError("Expected " + expected + " got " + actual);
        }
    }

    public static void assertTrue(boolean condition) {
        if (!condition) {
            throw new AssertionError("Expected true");
        }
    }
}
