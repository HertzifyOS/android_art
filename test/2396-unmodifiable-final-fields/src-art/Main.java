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
import java.lang.reflect.Field;
import java.util.concurrent.atomic.AtomicIntegerFieldUpdater;

import dalvik.system.VMRuntime;

public class Main {

    private static final MethodHandle FIELD_SETTER;
    private static final VarHandle FIELD;
    private static final AtomicIntegerFieldUpdater FIELD_UPDATER;

    static {
        try {
            FIELD_SETTER = MethodHandles.lookup()
                .findSetter(Main.class, "field", int.class);
            FIELD = MethodHandles.lookup()
                .findVarHandle(Main.class, "field", int.class);
            FIELD_UPDATER = AtomicIntegerFieldUpdater.newUpdater(Main.class, "field");
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    // volatile to make AtomicIntegerFieldUpdater happy.
    private volatile int field;

    public static void main(String[] args) throws Exception {
        int sdkVersion = VMRuntime.getRuntime().getSdkVersion();

        boolean attemptToModifyShouldFail = false;
        if (sdkVersion == 0 || sdkVersion > 36) {
            attemptToModifyShouldFail = true;
        }

        // B is 36, speculatively assuming that C is 37.
        VMRuntime.getRuntime().setTargetSdkVersion(37);

        Field mhTypeField = getAccessible(MethodHandle.class, "type");
        test(attemptToModifyShouldFail, () -> mhTypeField.set(FIELD_SETTER, null));
        Field vhVarTypeField = getAccessible(VarHandle.class, "varType");
        test(attemptToModifyShouldFail, () -> vhVarTypeField.set(FIELD, null));
        Field cclassField = getAccessible(FIELD_UPDATER.getClass(), "cclass");
        test(attemptToModifyShouldFail, () -> cclassField.set(FIELD_UPDATER, null));
    }

    private interface ThrowingRunnable {
        void run() throws Exception;
    }

    private static void test(boolean shouldFail, ThrowingRunnable subject) {
        try {
            subject.run();
        } catch (Exception e) {
            if (!shouldFail) {
                throw new AssertionError("Did not expect subject to fail", e);
            }
            return;
        }

        if (shouldFail) {
            throw new AssertionError("Expected subject to fail, but it didn't");
        }
    }

    private static Field getAccessible(Class<?> clazz, String name) throws Exception {
        Field field = clazz.getDeclaredField(name);
        field.setAccessible(true);
        return field;
    }
}
