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

import java.util.concurrent.ThreadLocalRandom;

public final class Main {
    private static final boolean BOOLEAN = $noinline$boolean();
    private static final byte BYTE = $noinline$byte();
    private static final char CHAR = $noinline$char();
    private static final short SHORT = $noinline$short();
    private static final int INT = $noinline$int();
    private static final float FLOAT = $noinline$float();
    private static final long LONG = $noinline$long();
    private static final double DOUBLE = $noinline$double();

    private static final class Values {
        static volatile boolean BOOLEAN;
        static volatile byte BYTE;
        static volatile char CHAR;
        static volatile short SHORT;
        static volatile int INT;
        static volatile long LONG;

        static {
            BOOLEAN = ThreadLocalRandom.current().nextBoolean();
            BYTE = (byte) ThreadLocalRandom.current().nextInt();
            CHAR = (char) ThreadLocalRandom.current().nextInt();
            SHORT = (short) ThreadLocalRandom.current().nextInt();
            INT = ThreadLocalRandom.current().nextInt();
            LONG = ThreadLocalRandom.current().nextLong();
        }
    }

    public static void main(String[] args) {
        System.loadLibrary(args[0]);

        ensureJitCompiled(Main.class, "$noinline$testBoolean");
        $noinline$testBoolean();

        ensureJitCompiled(Main.class, "$noinline$testByte");
        $noinline$testByte();

        ensureJitCompiled(Main.class, "$noinline$testChar");
        $noinline$testChar();

        ensureJitCompiled(Main.class, "$noinline$testShort");
        $noinline$testShort();

        ensureJitCompiled(Main.class, "$noinline$testInt");
        $noinline$testInt();

        ensureJitCompiled(Main.class, "$noinline$testFloat");
        $noinline$testFloat();

        ensureJitCompiled(Main.class, "$noinline$testLong");
        $noinline$testLong();

        ensureJitCompiled(Main.class, "$noinline$testDouble");
        $noinline$testDouble();
    }

    private static void $noinline$testBoolean() {
        boolean actual = BOOLEAN;
        if (actual != Values.BOOLEAN) {
            throw new AssertionError("Expected to be " + Values.BOOLEAN + ", got " + actual);
        }
    }

    private static boolean $noinline$boolean() {
        return Values.BOOLEAN;
    }

    private static void $noinline$testByte() {
        byte actual = BYTE;
        if (actual != Values.BYTE) {
            throw new AssertionError("Expected: " + Values.BYTE + ", got: " + actual);
        }
    }

    private static byte $noinline$byte() {
        return Values.BYTE;
    }

    private static void $noinline$testChar() {
        char actual = CHAR;
        if (actual != Values.CHAR) {
            throw new AssertionError("Expected: " + Values.CHAR + ", got: " + actual);
        }
    }

    private static char $noinline$char() {
        return Values.CHAR;
    }

    private static void $noinline$testShort() {
        short actual = SHORT;
        if (actual != Values.SHORT) {
            throw new AssertionError("Expected: " + Values.SHORT + ", got: " + actual);
        }
    }

    private static short $noinline$short() {
        return Values.SHORT;
    }

    private static void $noinline$testInt() {
        int actual = INT;
        if (actual != Values.INT) {
            throw new AssertionError("Expected: " + Values.INT + ", got: " + actual);
        }
    }

    private static int $noinline$int() {
        return Values.INT;
    }

    private static void $noinline$testFloat() {
        float actual = FLOAT;
        int bits = Float.floatToRawIntBits(actual);
        if (bits != Values.INT) {
            throw new AssertionError("Expected bits: " + Values.INT + ", got: " + bits);
        }
    }

    private static float $noinline$float() {
        return Float.intBitsToFloat(Values.INT);
    }

    private static void $noinline$testLong() {
        long actual = LONG;
        if (actual != Values.LONG) {
            throw new AssertionError("Expected: " + Values.LONG + ", got: " + actual);
        }
    }

    private static long $noinline$long() {
        return Values.LONG;
    }

    private static void $noinline$testDouble() {
        double actual = DOUBLE;
        long bits = Double.doubleToRawLongBits(actual);
        if (bits != Values.LONG) {
            throw new AssertionError("Expected bits: " + Values.LONG + ", got: " + bits);
        }
    }

    private static double $noinline$double() {
        return Double.longBitsToDouble(Values.LONG);
    }

    private native static void ensureJitCompiled(Class<?> clazz, String method);
}
