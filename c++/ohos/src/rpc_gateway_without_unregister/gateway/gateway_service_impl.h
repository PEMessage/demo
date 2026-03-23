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
#include <map>
#include <mutex>
#include <vector>

namespace OHOS {

/**
 * @brief Gateway Service Implementation
 * Concrete implementation of the GatewayService business logic.
 * This class extends GatewayServiceStub to provide actual functionality.
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

    // Get the callback to register with Backend (Gateway's own callback)
    sptr<IRemoteObject> GetGatewayCallbackForBackend();

protected:
    // Inner class: Gateway's callback implementation that receives events from Backend
    class GatewayEventCallback : public EventCallbackStub {
    public:
        explicit GatewayEventCallback(GatewayServiceImpl *gateway);
        ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) override;
    private:
        GatewayServiceImpl *gateway_;
    };

    // Client callback info with filter
    struct ClientCallbackInfo {
        sptr<EventCallbackProxy> callback;
        std::vector<int32_t> filterTypes;  // Empty = accept all
        sptr<IRemoteObject::DeathRecipient> deathRecipient;
    };

    // Forward declaration
    class ClientDeathRecipient;

    // Forward event to matching clients
    void ForwardEventToClients(int32_t event, const std::vector<int8_t> &data);
    bool ShouldForwardEvent(const ClientCallbackInfo &info, int32_t event);

public:
    // Called by death recipient when client dies
    void RemoveClientCallback(const sptr<IRemoteObject> &callback);

private:
    std::mutex clientCallbacksMutex_;
    std::map<sptr<IRemoteObject>, ClientCallbackInfo> clientCallbacks_;
    
    sptr<BackendServiceProxy> backendProxy_;
    sptr<GatewayEventCallback> gatewayCallback_;
    sptr<IRemoteObject> gatewayCallbackForBackend_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_IMPL_H
