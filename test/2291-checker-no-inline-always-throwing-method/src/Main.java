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

public class Main {
    public static void main(String[] args) throws Error {
        try {
            $noinline$testDivideUnsignedLong();
            throw new Error("Was expecting an ArithmeticException");
        } catch (ArithmeticException expected) {
        }

        try {
            $noinline$testRegularDivisionByZero();
            throw new Error("Was expecting an ArithmeticException");
        } catch (ArithmeticException expected) {
        }
    }

    // divideAndThrow shouldn't be inlined as it ends in an always-throwing instruction.

    /// CHECK-START: void Main.$noinline$testDivideUnsignedLong() inliner (before)
    /// CHECK: InvokeStaticOrDirect method_name:Main.divideAndThrow always_throws:false

    /// CHECK-START: void Main.$noinline$testDivideUnsignedLong() inliner (after)
    /// CHECK: InvokeStaticOrDirect method_name:Main.divideAndThrow always_throws:true
    private static void $noinline$testDivideUnsignedLong() {
        divideAndThrow();
    }

    /// CHECK-START: void Main.divideAndThrow() dead_code_elimination$initial (before)
    /// CHECK:     InvokeStaticOrDirect method_name:Main.assertLongEquals

    /// CHECK-START: void Main.divideAndThrow() dead_code_elimination$initial (after)
    /// CHECK-NOT: InvokeStaticOrDirect method_name:Main.assertLongEquals

    /// CHECK-START: void Main.divideAndThrow() inliner (after)
    /// CHECK:     InvokeStaticOrDirect method_name:java.lang.Long.divideUnsigned always_throws:true
    private static void divideAndThrow() {
        long dividend = 0x12345678L;
        long divisor = 0x0L;
        // Divide by zero!
        long result = Long.divideUnsigned(dividend, divisor);
        // The following lines will not be reached.
        assertLongEquals(0xdeadbeef, result);
    }

    private static void assertLongEquals(long expected, long result) {
        if (expected != result) {
            throw new Error("Expected: " + expected + ", found: " + result);
        }
    }

    // divideByZero shouldn't be inlined as it ends in an always-throwing instruction.

    /// CHECK-START: void Main.$noinline$testRegularDivisionByZero() inliner (before)
    /// CHECK: InvokeStaticOrDirect method_name:Main.divideByZero always_throws:false

    /// CHECK-START: void Main.$noinline$testRegularDivisionByZero() inliner (after)
    /// CHECK: InvokeStaticOrDirect method_name:Main.divideByZero always_throws:true
    private static void $noinline$testRegularDivisionByZero() {
        divideByZero();
    }

    /// CHECK-START: int Main.divideByZero() dead_code_elimination$initial (before)
    /// CHECK:     Add
    /// CHECK:     Mul

    /// CHECK-START: int Main.divideByZero() dead_code_elimination$initial (after)
    /// CHECK-NOT: Add

    /// CHECK-START: int Main.divideByZero() dead_code_elimination$initial (after)
    /// CHECK-NOT: Mul
    private static int divideByZero() {
        int dividend = 1234;
        int divisor = 0;
        int result = dividend / divisor;
        // Do calculations that should be eliminated since the divisor is zero
        result += 2;
        if (result == 456) {
            result = 789;
        } else {
            result = result * result;
        }
        return result;
    }
}
