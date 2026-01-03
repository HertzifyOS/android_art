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
import com.android.ahat.heapdump.Size;
import com.android.ahat.heapdump.Sort;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

class StringsHandler implements AhatHandler {
  private static final String STRINGS_ID = "strings";

  private AhatSnapshot mSnapshot;

  private static class DuplicateStringInfo {
    public final int index;
    public final AhatInstance representative;
    public final long size;
    public final int count;
    public final long sizeCount;
    public final String heapName;

    public DuplicateStringInfo(int index, List<AhatInstance> instances) {
      this.index = index;
      this.representative = instances.get(0);
      this.size = representative.asString().length();
      this.count = instances.size();
      this.sizeCount = this.size * this.count;
      this.heapName = representative.getHeap().getName();
    }
  }

  public StringsHandler(AhatSnapshot snapshot) {
    mSnapshot = snapshot;
  }

  @Override
  public void handle(Doc doc, Query query) throws IOException {
    int id = query.getInt("id", -1);
    List<List<AhatInstance>> duplicates = mSnapshot.getDuplicateStrings();
    if (id >= 0 && duplicates != null && id < duplicates.size()) {
      printStringInstances(doc, query, duplicates.get(id));
    } else {
      doc.title("Strings");

      doc.section("Duplicate Strings");
      printDuplicateStrings(doc, query);

      doc.section("All Strings");
      doc.println(DocString.link(
          DocString.uri("objects?class=java.lang.String"),
          DocString.text("All Strings")));
    }
  }

  private void printDuplicateStrings(Doc doc, Query query) {
    List<List<AhatInstance>> duplicates = mSnapshot.getDuplicateStrings();
    if (duplicates == null || duplicates.isEmpty()) {
       doc.println(DocString.text("(no duplicates)"));
       return;
    }

    List<DuplicateStringInfo> duplicateInfos = new ArrayList<>(duplicates.size());
    for (int i = 0; i < duplicates.size(); ++i) {
      duplicateInfos.add(new DuplicateStringInfo(i, duplicates.get(i)));
    }

    Comparator<DuplicateStringInfo> sizeCompare = new Comparator<DuplicateStringInfo>() {
      @Override
      public int compare(DuplicateStringInfo i1, DuplicateStringInfo i2) {
        return Long.compare(i1.size, i2.size);
      }
    };

    Comparator<DuplicateStringInfo> countCompare = new Comparator<DuplicateStringInfo>() {
      @Override
      public int compare(DuplicateStringInfo i1, DuplicateStringInfo i2) {
        return Integer.compare(i1.count, i2.count);
      }
    };

    Comparator<DuplicateStringInfo> sizeCountCompare = new Comparator<DuplicateStringInfo>() {
      @Override
      public int compare(DuplicateStringInfo i1, DuplicateStringInfo i2) {
        return Long.compare(i1.sizeCount, i2.sizeCount);
      }
    };

    Comparator<DuplicateStringInfo> heapCompare = new Comparator<DuplicateStringInfo>() {
      @Override
      public int compare(DuplicateStringInfo i1, DuplicateStringInfo i2) {
        return i1.heapName.compareTo(i2.heapName);
      }
    };

    Sorter<DuplicateStringInfo> sorter = new Sorter<DuplicateStringInfo>(
        query, sizeCountCompare.reversed());
    sorter.addKey("size", sizeCompare);
    sorter.addKey("count", countCompare);
    sorter.addKey("sc", sizeCountCompare);
    sorter.addKey("heap", heapCompare);
    sorter.sort(duplicateInfos);

    doc.table(
        new Column(sorter.link("size", "Size"), Column.Align.RIGHT),
        new Column(sorter.link("count", "Count"), Column.Align.RIGHT),
        new Column(sorter.link("sc", "Size * Count"), Column.Align.RIGHT),
        new Column(sorter.link("heap", "Heap"), Column.Align.LEFT),
        new Column("Value", Column.Align.LEFT)
    );

    for (DuplicateStringInfo info : duplicateInfos) {
        doc.row(
            DocString.format("%,d", info.size),
            DocString.link(
                DocString.formattedUri("strings?id=%d", info.index),
                DocString.format("%,d", info.count)),
            DocString.format("%,d", info.sizeCount),
            DocString.text(info.heapName),
            Summarizer.summarizeString(info.representative)
        );
    }
    doc.end();
  }

  private void printStringInstances(Doc doc, Query query, List<AhatInstance> instances) {
    doc.title("Duplicate String Instances");
    doc.description(DocString.text("Value"), Summarizer.summarizeString(instances.get(0)));
    doc.end();

    SizeTable.table(doc, mSnapshot.isDiffed(),
        new Column("Heap"),
        new Column("Object"));

    SubsetSelector<AhatInstance> selector = new SubsetSelector(query, STRINGS_ID, instances);
    for (AhatInstance inst : selector.selected()) {
      AhatInstance base = inst.getBaseline();
      SizeTable.row(doc,
          inst.getTotalRetainedSize(), base.getTotalRetainedSize(),
          DocString.text(inst.getHeap().getName()),
          Summarizer.summarize(inst));
    }
    SizeTable.end(doc);
    selector.render(doc);
  }


}