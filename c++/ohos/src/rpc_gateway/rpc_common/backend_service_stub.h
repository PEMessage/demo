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

#ifndef RPC_GATEWAY_BACKEND_SERVICE_STUB_H
#define RPC_GATEWAY_BACKEND_SERVICE_STUB_H

#include "backend_service_interface.h"
#include "iremote_stub.h"
#include "event_callback_proxy.h"
#include <vector>

namespace OHOS {

/**
 * @brief Backend Service Stub
 * Server-side implementation that receives requests from Gateway
 */
class BackendServiceStub : public IRemoteStub<IBackendService> {
public:
    BackendServiceStub();
    ~BackendServiceStub() override = default;

    // IBackendService interface
    int32_t RegisterGatewayCallback(const sptr<IEventCallback> &callback) override;
    int32_t UnregisterGatewayCallback() override;
    int32_t TriggerEvent(int32_t event, const std::vector<int8_t> &data) override;
    std::string GetServiceInfo() override;

    // IRemoteStub interface
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                        MessageParcel &reply, MessageOption &option) override;

    // Check if callback is registered
    bool HasCallback() const { return gatewayCallback_ != nullptr; }

protected:
    sptr<EventCallbackProxy> gatewayCallback_;

private:
    int32_t HandleRegisterGatewayCallback(MessageParcel &data, MessageParcel &reply);
    int32_t HandleUnregisterGatewayCallback(MessageParcel &data, MessageParcel &reply);
    int32_t HandleTriggerEvent(MessageParcel &data, MessageParcel &reply);
    int32_t HandleGetServiceInfo(MessageParcel &data, MessageParcel &reply);
};

} // namespace OHOS

#endif // RPC_GATEWAY_BACKEND_SERVICE_STUB_H
