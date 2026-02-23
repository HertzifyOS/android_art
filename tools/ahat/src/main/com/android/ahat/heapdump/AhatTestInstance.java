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

package com.android.ahat.heapdump;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Test-only subclass of AhatInstance.
 * This should only be used for testing purposes.
 */
public class AhatTestInstance extends AhatInstance {
  private final List<Reference> mReferences = new ArrayList<>();
  private Size mSize = Size.ZERO;

  /**
   * Create a new AhatTestInstance with the given id.
   * This should only be used for testing purposes.
   *
   * @param id the id of the instance to create
   */
  public AhatTestInstance(long id) {
    super(id);
  }

  /**
   * Set the size of this instance.
   * This should only be used for testing purposes.
   *
   * @param size the size of the instance to set
   */
  public void setSize(Size size) {
    mSize = size;
  }

  /**
   * Add a reference to another instance.
   * This should only be used for testing purposes.
   *
   * @param other the instance to add a reference to
   */
  public void addReference(AhatInstance other) {
    mReferences.add(new Reference(this, "ref", other, Reachability.STRONG));
  }

  /**
   * Add a reference to another instance with the given reachability.
   * This should only be used for testing purposes.
   *
   * @param other the instance to add a reference to
   * @param reachability the reachability of the reference
   */
  public void addReference(AhatInstance other, Reachability reachability) {
    mReferences.add(new Reference(this, "ref", other, reachability));
  }

  @Override
  public Size getSize() {
    return mSize;
  }

  @Override
  long getExtraJavaSize() {
    return 0;
  }

  @Override
  public Iterable<Reference> getReferences() {
    return mReferences;
  }

  @Override
  public String toString() {
    return "AhatTestInstance " + getId();
  }
}
