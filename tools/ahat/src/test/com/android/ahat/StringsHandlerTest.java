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

package com.android.ahat;

import com.android.ahat.heapdump.AhatTestInstance;
import com.android.ahat.heapdump.Reachability;
import com.android.ahat.heapdump.Size;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import static org.junit.Assert.assertEquals;

public class StringsHandlerTest {

  private static AhatTestInstance createInstance(long id, long javaSize) {
    AhatTestInstance inst = new AhatTestInstance(id);
    inst.setSize(new Size(javaSize, 0));
    return inst;
  }

  @Test
  public void testSingleObject() {
    AhatTestInstance obj = createInstance(1, 10);
    Size size = StringsHandler.getReachableSize(Collections.singletonList(obj), Reachability.STRONG);
    // obj (10) = 10
    assertEquals(10, size.getJavaSize());
  }

  @Test
  public void testTwoSeparateObjects() {
    AhatTestInstance obj1 = createInstance(1, 10);
    AhatTestInstance obj2 = createInstance(2, 20);
    Size size = StringsHandler.getReachableSize(Arrays.asList(obj1, obj2), Reachability.STRONG);
    // obj1 (10) + obj2 (20) = 30
    assertEquals(30, size.getJavaSize());
  }

  @Test
  public void testSharedObject() {
    AhatTestInstance obj1 = createInstance(1, 10);
    AhatTestInstance obj2 = createInstance(2, 10);
    AhatTestInstance shared = createInstance(3, 20);

    obj1.addReference(shared);
    obj2.addReference(shared);

    Size size = StringsHandler.getReachableSize(Arrays.asList(obj1, obj2), Reachability.STRONG);
    // obj1 (10) + obj2 (10) + shared (20) = 40. Shared object counted once.
    assertEquals(40, size.getJavaSize());
  }

  @Test
  public void testChain() {
    AhatTestInstance obj1 = createInstance(1, 10);
    AhatTestInstance obj2 = createInstance(2, 10);
    AhatTestInstance obj3 = createInstance(3, 10);

    obj1.addReference(obj2);
    obj2.addReference(obj3);

    Size size = StringsHandler.getReachableSize(Collections.singletonList(obj1), Reachability.STRONG);
    // obj1 (10) + obj2 (10) + obj3 (10) = 30
    assertEquals(30, size.getJavaSize());
  }

  @Test
  public void testCycle() {
    AhatTestInstance obj1 = createInstance(1, 10);
    AhatTestInstance obj2 = createInstance(2, 10);

    obj1.addReference(obj2);
    obj2.addReference(obj1);

    Size size = StringsHandler.getReachableSize(Collections.singletonList(obj1), Reachability.STRONG);
    // obj1 (10) + obj2 (10) = 20
    assertEquals(20, size.getJavaSize());
  }

  @Test
  public void testRespectsReachability() {
    AhatTestInstance obj1 = createInstance(1, 10);
    AhatTestInstance weakRef = createInstance(2, 20);

    obj1.addReference(weakRef, Reachability.WEAK);

    // When following only STRONG references:
    Size sizeStrong = StringsHandler.getReachableSize(Collections.singletonList(obj1), Reachability.STRONG);
    // only obj1 (10)
    assertEquals(10, sizeStrong.getJavaSize());

    // When following WEAK references:
    Size sizeWeak = StringsHandler.getReachableSize(Collections.singletonList(obj1), Reachability.WEAK);
    // obj1 (10) + weakRef (20) = 30
    assertEquals(30, sizeWeak.getJavaSize());
  }
}
