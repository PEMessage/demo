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

#ifndef RPC_GATEWAY_EVENT_CALLBACK_PROXY_H
#define RPC_GATEWAY_EVENT_CALLBACK_PROXY_H

#include "event_callback_interface.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief EventCallback Proxy
 * Used by Gateway to forward events to Server, or by Server to send events to Gateway
 */
class EventCallbackProxy : public IRemoteProxy<IEventCallback> {
public:
    explicit EventCallbackProxy(const sptr<IRemoteObject> &impl);
    ~EventCallbackProxy() override = default;

    ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) override;

private:
    static inline BrokerDelegator<EventCallbackProxy> delegator_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_EVENT_CALLBACK_PROXY_H
