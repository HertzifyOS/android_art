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

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;

public class Main {

    private static final int SIZE = 64;

    private static final VarHandle vhArray = MethodHandles.arrayElementVarHandle(boolean[].class);

    private static final MethodHandle setter = vhArray.toMethodHandle(VarHandle.AccessMode.SET);
    private static final MethodHandle getter = vhArray.toMethodHandle(VarHandle.AccessMode.GET);

    public static void main(String[] args) throws Throwable {
        System.loadLibrary(args[0]);

        $noinline$doWork();
    }

    private static void $noinline$doWork() throws Throwable {
        boolean[] array = new boolean[SIZE];

        boolean interpreting = isInInterpreter("$noinline$doWork");

        int i;
        // Straight from 570-checker-osr.
        for (i = 0; i < 300000; ++i) {}

        if (interpreting) {
            while (!isInOsrCode("$noinline$doWork")) {}
        }

        for (i = 0; i < SIZE; ++i) {
            setter.invokeExact(array, i, true);
            boolean x = (boolean) getter.invokeExact(array, i);

            assertEquals(x, true, "get boolean value");
        }
    }

    private static void assertEquals(boolean lhs, boolean rhs, String msg) {
        if (lhs != rhs) {
            throw new AssertionError(msg);
        }
    }

    public static native boolean isInOsrCode(String methodName);
    public static native boolean isInInterpreter(String methodName);
}
