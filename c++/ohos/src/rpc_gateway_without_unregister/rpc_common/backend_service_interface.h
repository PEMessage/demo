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

#ifndef RPC_GATEWAY_BACKEND_SERVICE_INTERFACE_H
#define RPC_GATEWAY_BACKEND_SERVICE_INTERFACE_H

#include "iremote_broker.h"
#include "event_callback_interface.h"

namespace OHOS {

/**
 * @brief Backend Service Interface (the actual business service)
 * This is the service that will trigger callbacks to clients through the Gateway
 */
class IBackendService : public IRemoteBroker {
public:
    enum BackendServiceCode {
        REGISTER_GATEWAY_CALLBACK = 1,
        TRIGGER_EVENT = 2,
        GET_SERVICE_INFO = 3,
    };

    /**
     * @brief Register Gateway's callback
     * Server will send events to this callback (Gateway), which then forwards to actual Client
     * When gateway dies, the callback will be automatically unregistered via death recipient
     * @param callback The Gateway's callback proxy
     * @return Error code, ERR_NONE on success
     */
    virtual int32_t RegisterGatewayCallback(const sptr<IEventCallback> &callback) = 0;

    /**
     * @brief Manually trigger an event (for testing)
     * @param event The event type
     * @param data The event data
     * @return Error code, ERR_NONE on success
     */
    virtual int32_t TriggerEvent(int32_t event, const std::vector<int8_t> &data) = 0;

    /**
     * @brief Get service information
     * @return Service info string
     */
    virtual std::string GetServiceInfo() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.rpc.gateway.IBackendService");
};

} // namespace OHOS

#endif // RPC_GATEWAY_BACKEND_SERVICE_INTERFACE_H
