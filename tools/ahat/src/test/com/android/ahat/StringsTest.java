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

import com.android.ahat.heapdump.AhatInstance;
import com.android.ahat.heapdump.AhatSnapshot;
import java.io.IOException;
import java.util.List;
import org.junit.Test;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class StringsTest {
  @Test
  public void testDuplicateStrings() throws IOException {
    TestDump dump = TestDump.getTestDump();
    AhatSnapshot snapshot = dump.getAhatSnapshot();

    // Verify duplicates are found
    List<List<AhatInstance>> duplicates = snapshot.getDuplicateStrings();
    assertFalse("Duplicate strings should be found", duplicates.isEmpty());

    boolean foundDuplicate = false;
    for (List<AhatInstance> list : duplicates) {
      if (list.size() >= 2) {
        String value = list.get(0).asString();
        if ("duplicate".equals(value)) {
          foundDuplicate = true;
          break;
        }
      }
    }
    assertTrue("Should find specific duplicate string 'duplicate'", foundDuplicate);

  }
}
