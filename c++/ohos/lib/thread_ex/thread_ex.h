/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /**
 * @file thread_ex.h
 *
 * @brief Provides interfaces of the <b>Thread</b> class
 * implemented in c_utils.
 */

#ifndef UTILS_THREAD_EX_H
#define UTILS_THREAD_EX_H

#include <pthread.h>
#include <string>
#include <mutex>
#include <condition_variable>

namespace OHOS {

enum class ThreadStatus {
    OK,
    WOULD_BLOCK,
    INVALID_OPERATION,
    UNKNOWN_ERROR,
};

enum ThreadPrio {
    THREAD_PROI_NORMAL = 0,
    THREAD_PROI_LOW = 10,
    THREAD_PROI_LOWEST = 19,
};

constexpr int INVALID_PTHREAD_T = -1;
constexpr int MAX_THREAD_NAME_LEN = 15;

/**
 * @brief Provides interfaces for creating a thread
 * and obtaining a thread ID.
 */
class Thread {
public:

/**
 * @brief A constructor used to create a <b>Thread</b> object, without
 * starting the thread.
 */
    Thread();
    virtual ~Thread();

/**
 * @brief Creates and starts a child thread, and executes
 * <b>Run()</b> in a loop.
 * The loop stops when <b>Run()</b> returns <b>false</b> or it is notified
 * to exit by `NotifyExitSync()` or `NotifyExitAsync()` from another thread.
 *
 * @param name Indicates the name of the thread.
 * @param priority Indicates the thread priority.
 * @param stack Indicates the size of the thread stack.
 * @return Returns <b>OK</b> if the call is successful;
 * returns <b>INVALID_OPERATION</b> if the thread already exists;
 * returns <b>UNKNOWN_ERROR</b> if the thread creation fails.
 * @see {@link NotifyExitSync()} or {@link NotifyExitAsync()}
 */
    ThreadStatus Start(const std::string& name, int32_t priority = THREAD_PROI_NORMAL, size_t stack = 0);

/**
 * @brief Synchronously instructs this <b>Thread</b> object to exit.
 *
 * This method can be called only by another thread to instruct this
 * <b>Thread</b> object to exit. The calling thread will be blocked until this
 * <b>Thread</b> object exits.
 */
    ThreadStatus NotifyExitSync();

/**
 * @brief Asynchronously instructs this <b>Thread</b> object to exit.
 *
 * This method can be called only by another thread to instruct this
 * <b>Thread</b> object to exit. However, the calling thread will not be blocked
 * when this <b>Thread</b> object exits.
 */
    virtual void NotifyExitAsync();

/**
 * @brief Checks whether the thread is ready.
 */
    virtual bool ReadyToWork();

/**
 * @brief Checks whether there is any thread waiting for exit.
 *
 * If <b>true</b> is returned, the waiting threads who have called
 * `NotifyExitSync()` will be woken up when the current thread finishes
 * running and exits.
 *
 * @return Returns <b>true</b> if there is any thread that is
 * blocked to wait for the current thread to exit.
 * Returns <b>false</b> otherwise.
 */
    bool IsExitPending() const;

/**
 * @brief Checks whether the thread is running.
 *
 * @return Returns <b>true</b> if the thread is running;
 * returns <b>false</b> otherwise.
 */
    bool IsRunning() const;

/**
 * @brief Obtains the thread ID.
 */
    pthread_t GetThread() const { return thread_; }

protected:
    virtual bool Run() = 0; // Derived class must implement Run()

private:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    static int ThreadStart(void* args);

private:
    pthread_t thread_;  // Thread ID
    mutable std::mutex lock_;
    std::condition_variable cvThreadExited_;
    ThreadStatus status_;
    volatile bool exitPending_;
    volatile bool running_; // flag of thread running
};

} // namespace OHOS

#endif

