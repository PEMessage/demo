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

#ifndef RPC_GATEWAY_EVENT_CALLBACK_INTERFACE_H
#define RPC_GATEWAY_EVENT_CALLBACK_INTERFACE_H

#include "iremote_broker.h"
#include "ipc_types.h"
#include <cstdint>
#include <vector>

namespace OHOS {

/**
 * @brief Event Callback Interface
 * Used for bi-directional communication where events can be filtered by type
 */
class IEventCallback : public IRemoteBroker {
public:
    enum EventCallbackCode {
        ON_EVENT = 1,
    };

    /**
     * @brief Called when an event occurs
     * @param event The event type code
     * @param reqData The event data as byte array
     * @return Error code, ERR_NONE on success
     */
    virtual ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.rpc.gateway.IEventCallback");
};

} // namespace OHOS

#endif // RPC_GATEWAY_EVENT_CALLBACK_INTERFACE_H
