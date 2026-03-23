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

#ifndef RPC_GATEWAY_SERVICE_IMPL_H
#define RPC_GATEWAY_SERVICE_IMPL_H

#include "gateway_service_stub.h"
#include "event_callback_proxy.h"
#include "event_callback_stub.h"
#include "backend_service_proxy.h"

namespace OHOS {

/**
 * @brief Gateway Service Implementation (Simplified - Single Client)
 * Only supports one client at a time. When client dies, callback becomes null automatically.
 */
class GatewayServiceImpl : public GatewayServiceStub {
public:
    GatewayServiceImpl();
    ~GatewayServiceImpl() override = default;

    // IGatewayService interface implementation
    int32_t RegisterClientCallback(const sptr<IEventCallback> &callback,
                                    const std::vector<int32_t> &filterEventTypes) override;

    // Connect to Backend Server
    bool ConnectToBackend(const sptr<IRemoteObject> &backendRemote);

protected:
    // Inner class: Gateway's callback that receives events from Backend
    class GatewayEventCallback : public EventCallbackStub {
    public:
        explicit GatewayEventCallback(GatewayServiceImpl *gateway);
        ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) override;
    private:
        GatewayServiceImpl *gateway_;
    };

    // Forward event to client (if registered)
    void ForwardEventToClient(int32_t event, const std::vector<int8_t> &data);

private:
    // Backend connection
    sptr<BackendServiceProxy> backendProxy_;
    sptr<GatewayEventCallback> gatewayCallback_;
    
    // Single client callback - when client dies, sptr becomes null automatically
    sptr<EventCallbackProxy> clientCallback_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_IMPL_H
