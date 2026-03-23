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

#ifndef RPC_GATEWAY_SERVICE_INTERFACE_H
#define RPC_GATEWAY_SERVICE_INTERFACE_H

#include "iremote_broker.h"
#include "event_callback_interface.h"
#include <string>

namespace OHOS {

/**
 * @brief Gateway Service Interface
 * Client registers its callback through Gateway, and Gateway forwards it to Server
 */
class IGatewayService : public IRemoteBroker {
public:
    enum GatewayServiceCode {
        REGISTER_CLIENT_CALLBACK = 1,
    };

    /**
     * @brief Register client's callback with the Gateway
     * Gateway will store this callback and forward events from Server to Client
     * When client dies, the callback will be automatically unregistered via death recipient
     * @param callback The client's callback object
     * @param filterEventTypes Optional event types to filter (empty = all events)
     * @return Error code, ERR_NONE on success
     */
    virtual int32_t RegisterClientCallback(const sptr<IEventCallback> &callback, 
                                            const std::vector<int32_t> &filterEventTypes) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.rpc.gateway.IGatewayService");
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_INTERFACE_H
