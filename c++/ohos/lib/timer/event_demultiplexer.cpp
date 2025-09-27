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

#include "event_demultiplexer.h"
#include "event_reactor.h"
#include "event_handler.h"
#include "common_timer_errors.h"
#include "utils_log.h"

#include <vector>
#include <sys/epoll.h>

namespace OHOS {
namespace Utils {

static const int EPOLL_MAX_EVENS_INIT = 8;
static const int HALF_OF_MAX_EVENT = 2;
static const int EPOLL_INVALID_FD = -1;
static const int INTERRUPTED_SYS_CALL = 4;
static const int EPOLL_ERROR_BADF = 9;
static const int EPOLL_ERROR_EINVAL = 22;
static const int QUEUE_MAX_SIZE = 10;

EventDemultiplexer::EventDemultiplexer()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)), maxEvents_(EPOLL_MAX_EVENS_INIT), mutex_(), eventHandlers_(),
    epollCtlErrQueue_()
{
}

EventDemultiplexer::~EventDemultiplexer()
{
    CleanUp();
}

uint32_t EventDemultiplexer::StartUp()
{
    if (epollFd_ < 0) {
        epollFd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epollFd_ < 0) {
            return TIMER_ERR_BADF;
        }
    }
    return TIMER_ERR_OK;
}

void EventDemultiplexer::CleanUp()
{
    if (epollFd_ != EPOLL_INVALID_FD) {
        close(epollFd_);
        epollFd_ = EPOLL_INVALID_FD;
    }
}

uint32_t EventDemultiplexer::UpdateEventHandler(EventHandler* handler)
{
    if (handler == nullptr) {
        return TIMER_ERR_INVALID_VALUE;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto itor = eventHandlers_.find(handler->GetHandle());
    if (itor == eventHandlers_.end()) {
        eventHandlers_.insert(std::make_pair(handler->GetHandle(), handler->shared_from_this()));
        return Update(EPOLL_CTL_ADD, handler);
    }

    if (handler->Events() == EventReactor::NONE_EVENT) {
        eventHandlers_.erase(itor);
        return Update(EPOLL_CTL_DEL, handler);
    }

    if (handler != itor->second.get()) {
        return TIMER_ERR_DEAL_FAILED;
    }
    return Update(EPOLL_CTL_MOD, handler);
}

uint32_t EventDemultiplexer::Update(int operation, EventHandler* handler)
{
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events   = Reactor2Epoll(handler->Events());
    event.data.fd = handler->GetHandle();

    if (epoll_ctl(epollFd_, operation, handler->GetHandle(), &event) != 0) {
        UTILS_LOGE("epoll_ctl %{public}d  operation %{public}d on handle %{public}d failed, errno %{public}d",
            epollFd_, operation, handler->GetHandle(), errno);

        if (epollCtlErrQueue_.size() < QUEUE_MAX_SIZE) {
            epollCtlErrQueue_.push(std::make_tuple(operation, errno, handler->GetHandle()));
        }
        return TIMER_ERR_DEAL_FAILED;
    }
    return TIMER_ERR_OK;
}

int EventDemultiplexer::Polling(int timeout /* ms */)
{
    std::vector<struct epoll_event> epollEvents(maxEvents_);
    std::vector<std::shared_ptr<EventHandler>> taskQue;
    std::vector<uint32_t> eventQue;

    int nfds = epoll_wait(epollFd_, &epollEvents[0], static_cast<int>(epollEvents.size()), timeout);
    if (nfds == 0) {
        return nfds;
    }
    if (nfds == -1) {
        if (errno != INTERRUPTED_SYS_CALL) {
            UTILS_LOGE("epoll_wait failed, errno %{public}d, epollFd_: %{public}d, pollEvents.size: %{public}zu",
                errno, epollFd_, epollEvents.size());
        }
        if (errno == EPOLL_ERROR_BADF || errno == EPOLL_ERROR_EINVAL) {
            UTILS_LOGE("epoll_wait critical error, thread exit");
            return EPOLL_CRITICAL_ERROR;
        }
        return nfds;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (int idx = 0; idx < nfds; ++idx) {
            int targetFd = epollEvents[idx].data.fd;
            uint32_t events = epollEvents[idx].events;

            auto itor = eventHandlers_.find(targetFd);
            if (itor != eventHandlers_.end()) {
                taskQue.emplace_back(itor->second);
                eventQue.emplace_back(events);
            } else {
                UTILS_LOGE("fd not found in eventHandlers_, fd=%{public}d, events=%{public}d", targetFd, events);
                
                while (!epollCtlErrQueue_.empty()) {
                    auto [operation, errnoNum, fd] = epollCtlErrQueue_.front();
                    UTILS_LOGE("epoll_ctl: %{public}d, errno: %{public}d, handle: %{public}d", operation, errnoNum, fd);
                    epollCtlErrQueue_.pop();
                }
            }
        }
    }

    for (size_t idx = 0u; idx < taskQue.size() && idx < eventQue.size(); idx++) {
        taskQue[idx]->HandleEvents(eventQue[idx]);
    }

    if (nfds == maxEvents_) {
        maxEvents_ *= HALF_OF_MAX_EVENT;
    }
    return nfds;
}

uint32_t EventDemultiplexer::Epoll2Reactor(uint32_t epollEvents)
{
    if (epollEvents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        return EventReactor::READ_EVENT;
    }

    return EventReactor::NONE_EVENT;
}

uint32_t EventDemultiplexer::Reactor2Epoll(uint32_t reactorEvent)
{
    switch (reactorEvent) {
        case EventReactor::NONE_EVENT:
            return TIMER_ERR_OK;
        case EventReactor::READ_EVENT:
            return EPOLLIN | EPOLLPRI;
        default:
            UTILS_LOGD("invalid event %{public}u.", reactorEvent);
            return TIMER_ERR_DEAL_FAILED;
    }
}

}
}
