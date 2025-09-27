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

#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include <sys/types.h>
#include <cstdint>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace OHOS {
namespace Utils {
class EventReactor;
/**
 * @brief Implements a timer manager.
 *
 * After a timer is started, users can register several timed events, which
 * can be continuous or one-shot, to it. Some points need to be noticed:\n
 * 1. A timer must be set up (through Setup()) before use, and be shut down
 * (via Shutdown()) before its deconstruction.\n
 * 2. A timer must be set up first and then shut down. Avoid delegating a
 * timer to different threads. Otherwise, multithreading issues may occur.\n
 * 3. Setting up a timer again will not reset the timer, but return
 * `TIMER_ERR_INVALID_VALUE`. If a reset operation is required, shut down
 * the timer first and then set it up.\n
 * 4. The parameter in Shutdown() determines whether the thread in the timer
 * will be detached. A detach operation may cause possible
 * multithreading problems, and is therefore not recommended. If a
 * detach operation is required, availability of related objects used in
 * `thread_` must be guaranteed.
 */
class Timer {
public:
    using TimerCallback = std::function<void ()>;
    using TimerCallbackPtr = std::shared_ptr<TimerCallback>;
    using TimerListCallback = std::function<void (int timerFd)>;

public:
    /**
     * @brief Creates a timer.
     *
     * In performance-sensitive scenarios, set `timeoutMs` to a
	 * greater value before timer setup based on your timed event setttings. The
     * default value is 1000 ms. The timeout event requires 100 us to respond.
     *
     * @param name Indicates the name of the timer. It is used as the name
	 * of the thread in the timer.
     * @param timeoutMs Indicates the duration for which the timer will wait.
     * The value is an integer in [-1, INT32MAX], but `-1` and `0` are not
     * recommended. `-1` means to wait indefinitely (until the timed event is
     * triggered). `0` means not to wait, which occupies too much CPU time.
     */
    explicit Timer(const std::string& name, int timeoutMs = 1000);
    virtual ~Timer();

    /**
     * @brief Sets up a timer.
     *
     * Do not set up a timer before shutting down the existing one.
     */
    virtual uint32_t Setup();

    /**
     * @brief Shuts down this timer.
     *
     * A timer can be shut down in blocking or unblocking mode. In blocking
     * mode, the timer will be shut down only after all running events
     * in the timer have finished. If `timeoutMs` is set to `-1`, use
     * unblocking mode to avoid deadloop.
     *
     * @param useJoin Specifies whether to use blocking mode. The value `true`
     * means to use blocking mode, and `false` (not recommended) means
     * the opposite.
     */
    virtual void Shutdown(bool useJoin = true);

    /**
     * @brief Registers timed events.
     *
     * @param callback Indicates the callback function of a timed event.
     * @param interval Indicates the interval of a timed event, in ms.
     * @param once Indicates whether the timed event is one-shot.
     * The value `true` means that the timed event is one-shot,
	 * and `false` means the opposite. The default value is `false`.
     * @return Returns the ID of a timed event. You can use it as the
     * parameter of Unregister().
     * @see Unregister
     */
    uint32_t Register(const TimerCallback& callback, uint32_t interval /* ms */, bool once = false);
    /**
     * @brief Deletes a timed event.
     *
     * @param timerId Indicates the ID of the timed event to delete.
     * It can be obtained through Register().
     * @see Register
     */
    void Unregister(uint32_t timerId);

private:
    void MainLoop();
    void OnTimer(int timerFd);
    virtual uint32_t DoRegister(const TimerListCallback& callback, uint32_t interval, bool once, int &timerFd);
    virtual void DoUnregister(uint32_t interval);
    void DoTimerListCallback(const TimerListCallback& callback, int timerFd);
    uint32_t GetValidId(uint32_t timerId) const;
    int GetTimerFd(uint32_t interval /* ms */);
    void EraseUnusedTimerId(uint32_t interval, const std::vector<uint32_t>& unusedIds);

private:
    struct TimerEntry {
        uint32_t       timerId;  // Unique ID.
        uint32_t       interval;  // million second
        TimerCallback  callback;
        bool           once;
        int            timerFd;
    };

    using TimerEntryPtr = std::shared_ptr<TimerEntry>;
    using TimerEntryList = std::list<TimerEntryPtr>;

    std::map<uint32_t, TimerEntryList> intervalToTimers_;  // interval to TimerEntryList
    std::map<uint32_t, TimerEntryPtr> timerToEntries_;  // timer_id to TimerEntry

    std::string name_;
    int timeoutMs_;
    std::thread thread_;
    EventReactor *reactor_;
    std::map<uint32_t, uint32_t> timers_;  // timer_fd to interval
    std::mutex mutex_;
};

} // namespace Utils
} // namespace OHOS
#endif

