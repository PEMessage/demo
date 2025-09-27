/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef UTILS_EVENT_HANDLER_H
#define UTILS_EVENT_HANDLER_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <unistd.h>

namespace OHOS {
namespace Utils {

class EventReactor;

class EventHandler : public std::enable_shared_from_this<EventHandler> {
public:
    using Callback = std::function<void()>;

    EventHandler(int fd, EventReactor* r);
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&&) = delete;
    EventHandler(const EventHandler&&) = delete;
    virtual ~EventHandler() {}

    int GetHandle() const { return (fd_); }
    void SetHandle(int fd) { fd_ = fd; }
    uint32_t Events() const { return (events_); }

    void EnableRead();
    void DisableAll();

    const EventReactor* GetEventReactor() const { return reactor_; }

    void SetReadCallback(const Callback& readCallback) { readCallback_ = readCallback; }

    void HandleEvents(uint32_t events);

private:
    void Update();

private:
    int             fd_;
    uint32_t        events_;
    EventReactor*   reactor_;

    Callback  readCallback_;
};

}
}
#endif /* UTILS_EVENT_HANDLER_H_ */
