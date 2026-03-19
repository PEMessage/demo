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

#ifndef RPC_GATEWAY_SERVICE_STUB_H
#define RPC_GATEWAY_SERVICE_STUB_H

#include "gateway_service_interface.h"
#include "iremote_stub.h"
#include "event_callback_proxy.h"
#include "event_callback_stub.h"
#include "backend_service_proxy.h"
#include <map>
#include <mutex>
#include <vector>

namespace OHOS {

/**
 * @brief Gateway Service Stub
 * Receives client registrations and forwards events to clients with filtering
 */
class GatewayServiceStub : public IRemoteStub<IGatewayService> {
public:
    GatewayServiceStub();
    ~GatewayServiceStub() override = default;

    // IGatewayService interface
    int32_t RegisterClientCallback(const sptr<IEventCallback> &callback,
                                    const std::vector<int32_t> &filterEventTypes) override;
    int32_t UnregisterClientCallback(const sptr<IEventCallback> &callback) override;

    // IRemoteStub interface
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                        MessageParcel &reply, MessageOption &option) override;

    // Connect to Backend Server
    bool ConnectToBackend(const sptr<IRemoteObject> &backendRemote);
    void DisconnectFromBackend();

    // Register/unregister callback with Backend (called when first client registers / last client unregisters)
    bool EnsureBackendCallbackRegistered();
    void EnsureBackendCallbackUnregistered();

    // Get the callback to register with Backend (Gateway's own callback)
    sptr<IRemoteObject> GetGatewayCallbackForBackend();

protected:
    // Inner class: Gateway's callback implementation that receives events from Backend
    class GatewayEventCallback : public EventCallbackStub {
    public:
        explicit GatewayEventCallback(GatewayServiceStub *gateway);
        ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) override;
    private:
        GatewayServiceStub *gateway_;
    };

    // Client callback info with filter
    struct ClientCallbackInfo {
        sptr<EventCallbackProxy> callback;
        std::vector<int32_t> filterTypes;  // Empty = accept all
        sptr<IRemoteObject::DeathRecipient> deathRecipient;
    };

    // Death recipient for client callbacks
    class ClientDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ClientDeathRecipient(GatewayServiceStub *gateway, sptr<IRemoteObject> clientCallback);
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
    private:
        GatewayServiceStub *gateway_;
        wptr<IRemoteObject> clientCallback_;
    };

    // Forward event to matching clients
    void ForwardEventToClients(int32_t event, const std::vector<int8_t> &data);
    bool ShouldForwardEvent(const ClientCallbackInfo &info, int32_t event);
    void RemoveClientCallback(const sptr<IRemoteObject> &callback);

private:
    int32_t HandleRegisterClientCallback(MessageParcel &data, MessageParcel &reply);
    int32_t HandleUnregisterClientCallback(MessageParcel &data, MessageParcel &reply);

    std::mutex clientCallbacksMutex_;
    std::map<sptr<IRemoteObject>, ClientCallbackInfo> clientCallbacks_;
    
    sptr<BackendServiceProxy> backendProxy_;
    sptr<GatewayEventCallback> gatewayCallback_;
    sptr<IRemoteObject> gatewayCallbackForBackend_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_STUB_H
