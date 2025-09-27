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
 * @file safe_block_queue.h
 *
 * Provides interfaces for thread-safe blocking queues in c_utils.
 * The file includes the <b>SafeBlockQueue</b> class and
 * the <b>SafeBlockQueueTracking</b> class for trackable tasks.
 */

#ifndef UTILS_BASE_BLOCK_QUEUE_H
#define UTILS_BASE_BLOCK_QUEUE_H

#include <climits>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>

namespace OHOS {

/**
 * @brief Provides interfaces for thread-safe blocking queues.
 *
 * The interfaces can be used to perform blocking and non-blocking push and
 * pop operations on queues.
 */
template <typename T>
class SafeBlockQueue {
public:
    explicit SafeBlockQueue(int capacity) : maxSize_(capacity)
    {
    }

/**
 * @brief Inserts an element at the end of this queue in blocking mode.
 *
 * If the queue is full, the thread of the push operation will be blocked
 * until the queue has space.
 * If the queue is not full, the push operation can be performed and one of the
 * pop threads (blocked when the queue is empty) is woken up.
 *
 * @param elem Indicates the element to insert.
 */
    virtual void Push(T const& elem)
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        while (queueT_.size() >= maxSize_) {
            // If the queue is full, wait for jobs to be taken.
            cvNotFull_.wait(lock, [&]() { return (queueT_.size() < maxSize_); });
        }

        // Insert the element into the queue if the queue is not full.
        queueT_.push(elem);
        cvNotEmpty_.notify_one();
    }

/**
 * @brief Removes the first element from this queue in blocking mode.
 *
 * If the queue is empty, the thread of the pop operation will be blocked
 * until the queue has elements.
 * If the queue is not empty, the pop operation can be performed, the first
 * element of the queue is returned, and one of the push threads (blocked
 * when the queue is full) is woken up.
 */
    T Pop()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);

        while (queueT_.empty()) {
            // If the queue is empty, wait for elements to be pushed in.
            cvNotEmpty_.wait(lock, [&] { return !queueT_.empty(); });
        }

        T elem = queueT_.front();
        queueT_.pop();
        cvNotFull_.notify_one();
        return elem;
    }

/**
 * @brief Inserts an element at the end of this queue in non-blocking mode.
 *
 * If the queue is full, <b>false</b> is returned directly.
 * If the queue is not full, the push operation can be performed, one of the
 * pop threads (blocked when the queue is empty) is woken up, and <b>true</b>
 * is returned.
 *
 * @param elem Indicates the element to insert.
 */
    virtual bool PushNoWait(T const& elem)
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        if (queueT_.size() >= maxSize_) {
            return false;
        }
        // Insert the element if the queue is not full.
        queueT_.push(elem);
        cvNotEmpty_.notify_one();
        return true;
    }

/**
 * @brief Removes the first element from this queue in non-blocking mode.
 *
 * If the queue is empty, <b>false</b> is returned directly.
 * If the queue is not empty, the pop operation can be performed, one of the
 * push threads (blocked when the queue is full) is woken up, and <b>true</b>
 * is returned.
 *
 * @param outtask Indicates the data of the pop operation.
 */
    bool PopNotWait(T& outtask)
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        if (queueT_.empty()) {
            return false;
        }
        outtask = queueT_.front();
        queueT_.pop();

        cvNotFull_.notify_one();

        return true;
    }

    unsigned int Size()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        return queueT_.size();
    }

    bool IsEmpty()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        return queueT_.empty();
    }

    bool IsFull()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        return queueT_.size() == maxSize_;
    }

    virtual ~SafeBlockQueue() {}

protected:
    unsigned long maxSize_;  // Capacity of the queue
    std::mutex mutexLock_;
    std::condition_variable cvNotEmpty_;
    std::condition_variable cvNotFull_;
    std::queue<T> queueT_;
};

/**
 * @brief Provides interfaces for operating the thread-safe blocking queues
 * and tracking the number of pending tasks.
 * This class inherits from <b>SafeBlockQueue</b>.
 */
template <typename T>
class SafeBlockQueueTracking : public SafeBlockQueue<T> {
public:
    explicit SafeBlockQueueTracking(int capacity) : SafeBlockQueue<T>(capacity)
    {
        unfinishedTaskCount_ = 0;
    }

    virtual ~SafeBlockQueueTracking() {}

/**
 * @brief Inserts an element at the end of this queue in blocking mode.
 *
 * If the queue is full, the thread of the push operation will be blocked
 * until the queue has space.
 * If the queue is not full, the push operation can be performed and one of the
 * pop threads (blocked when the queue is empty) is woken up.
 */
    virtual void Push(T const& elem)
    {
        unfinishedTaskCount_++;
        std::unique_lock<std::mutex> lock(mutexLock_);
        while (queueT_.size() >= maxSize_) {
            // If the queue is full, wait for jobs to be taken.
            cvNotFull_.wait(lock, [&]() { return (queueT_.size() < maxSize_); });
        }

        // If the queue is not full, insert the element.
        queueT_.push(elem);

        cvNotEmpty_.notify_one();
    }

/**
 * @brief Inserts an element at the end of this queue in non-blocking mode.
 *
 * If the queue is full, <b>false</b> is returned directly.
 * If the queue is not full, the push operation can be performed,
 * one of the pop threads (blocked when the queue is empty) is woken up,
 * and <b>true</b> is returned.
 */
    virtual bool PushNoWait(T const& elem)
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        if (queueT_.size() >= maxSize_) {
            return false;
        }
        // Insert the element if the queue is not full.
        queueT_.push(elem);
        unfinishedTaskCount_++;
        cvNotEmpty_.notify_one();
        return true;
    }

/**
 * @brief Called to return the result when a task is complete.
 *
 * If the count of unfinished tasks < 1, <b>false</b> is returned directly.
 * If the count of unfinished tasks = 1, all the threads blocked
 * by calling Join() will be woken up,
 * the count of unfinished tasks decrements by 1, and <b>true</b> is returned.
 * If the count of unfinished tasks > 1,
 * the count of unfinished tasks decrements by 1, and <b>true</b> is returned.
 */
    bool OneTaskDone()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        int unfinished = unfinishedTaskCount_ - 1;

        if (unfinished <= 0) {
            if (unfinished < 0) {
                return false; // false mean call elem done too many times
            }
            cvAllTasksDone_.notify_all();
        }

        unfinishedTaskCount_ = unfinished;
        return true;
    }

/**
 * @brief Waits for all tasks to complete.
 *
 * If there is any task not completed, the current thread will be
 * blocked even if it is just woken up.
 */
    void Join()
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        cvAllTasksDone_.wait(lock, [&] { return unfinishedTaskCount_ == 0; });
    }

/**
 * @brief Obtains the number of unfinished tasks.
 */
    int GetUnfinishTaskNum()
    {
        return unfinishedTaskCount_;
    }

protected:
    using SafeBlockQueue<T>::maxSize_;
    using SafeBlockQueue<T>::mutexLock_;
    using SafeBlockQueue<T>::cvNotEmpty_;
    using SafeBlockQueue<T>::cvNotFull_;
    using SafeBlockQueue<T>::queueT_;

    std::atomic<int> unfinishedTaskCount_;
    std::condition_variable cvAllTasksDone_;
};

} // namespace OHOS

#endif
