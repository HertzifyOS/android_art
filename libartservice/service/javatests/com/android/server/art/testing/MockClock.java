/*
 * Copyright (C) 2022 The Android Open Source Project
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

package com.android.server.art.testing;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyLong;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;

import android.os.Handler;
import android.os.HandlerThread;

import com.android.server.art.utils.AsyncExecutor;

import java.util.Comparator;
import java.util.PriorityQueue;

public class MockClock {
    private Handler mHandler;
    private HandlerThread mHandlerThread;
    private AsyncExecutor mExecutor;

    private long mCurrentTimeMs = 0;
    private PriorityQueue<Task> mTasks =
            new PriorityQueue<>(Comparator.comparingLong(Task::scheduledTimeMs));
    private boolean mIsShutdown = false;

    public MockClock() {
        mHandler = mock(Handler.class);
        mHandlerThread = mock(HandlerThread.class);

        lenient()
                .when(mHandler.postDelayed(any(Runnable.class), any(), anyLong()))
                .thenAnswer(invocation -> {
                    Runnable runnable = invocation.getArgument(0);
                    Object token = invocation.getArgument(1);
                    long delayMillis = invocation.getArgument(2);

                    assertThat(mIsShutdown).isFalse();
                    mTasks.add(new Task(runnable, token, mCurrentTimeMs + delayMillis));
                    return true;
                });

        lenient()
                .doAnswer(invocation -> {
                    Object token = invocation.getArgument(0);
                    mTasks.removeIf(task -> task.token == token);
                    return null;
                })
                .when(mHandler)
                .removeCallbacksAndMessages(any());

        lenient().when(mHandlerThread.quitSafely()).thenAnswer(invocation -> {
            mTasks.clear();
            mIsShutdown = true;
            return true;
        });

        mExecutor = new AsyncExecutor(mHandler, mHandlerThread);
    }

    public AsyncExecutor getAsyncExecutor() {
        return mExecutor;
    }

    public long getCurrentTimeMs() {
        return mCurrentTimeMs;
    }

    public void advanceTime(long timeMs) {
        mCurrentTimeMs += timeMs;
        while (!mTasks.isEmpty() && mTasks.peek().scheduledTimeMs <= mCurrentTimeMs) {
            mTasks.poll().runnable.run();
        }
    }

    public Handler getHandler() {
        return mHandler;
    }

    private record Task(Runnable runnable, Object token, long scheduledTimeMs) {}
}
