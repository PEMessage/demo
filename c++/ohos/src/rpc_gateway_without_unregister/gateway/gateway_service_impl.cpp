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

#include "gateway_service_impl.h"
#include "hilog/log.h"

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001536, "GatewayServiceImpl"};

// Death recipient for client callback
class ClientDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit ClientDeathRecipient(GatewayServiceImpl *gateway) : gateway_(gateway) {}
    void OnRemoteDied(const wptr<IRemoteObject> &object) override {
        (void)object;
        HiLogWarn(LABEL, "[Gateway] Client died, clearing callback");
        if (gateway_ != nullptr) {
            gateway_->OnClientDied();
        }
    }
private:
    GatewayServiceImpl *gateway_;
};

// GatewayEventCallback implementation
GatewayServiceImpl::GatewayEventCallback::GatewayEventCallback(GatewayServiceImpl *gateway)
    : gateway_(gateway) {}

ErrCode GatewayServiceImpl::GatewayEventCallback::OnEvent(int32_t event, const std::vector<int8_t> &reqData)
{
    if (gateway_ == nullptr) {
        return ERR_INVALID_DATA;
    }
    gateway_->ForwardEventToClient(event, reqData);
    return ERR_NONE;
}

// ConnectToBackend: Just save the proxy, don't register callback yet
bool GatewayServiceImpl::ConnectToBackend(const sptr<IRemoteObject> &backendRemote)
{
    if (backendRemote == nullptr) {
        HiLogError(LABEL, "Backend remote is null");
        return false;
    }

    backendProxy_ = new BackendServiceProxy(backendRemote);
    HiLogInfo(LABEL, "[Gateway] Connected to Backend");
    return true;
}

// RegisterClientCallback: Create callbacks and register with backend
int32_t GatewayServiceImpl::RegisterClientCallback(const sptr<IEventCallback> &callback,
                                                    const std::vector<int32_t> &filterEventTypes)
{
    (void)filterEventTypes;
    
    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> callbackObj = callback->AsObject();
    if (callbackObj == nullptr) {
        HiLogError(LABEL, "Callback object is null");
        return ERR_INVALID_DATA;
    }

    // Create client callback
    clientCallback_ = new EventCallbackProxy(callbackObj);
    clientCallback_->EnableTracker();
    
    // Add death recipient to detect when client dies
    clientDeathRecipient_ = new ClientDeathRecipient(this);
    if (!callbackObj->AddDeathRecipient(clientDeathRecipient_)) {
        HiLogWarn(LABEL, "[Gateway] Failed to add death recipient");
    }

    // Create and register gateway callback
    gatewayCallback_ = new GatewayEventCallback(this);
    
    if (backendProxy_ != nullptr) {
        int32_t result = backendProxy_->RegisterGatewayCallback(
            iface_cast<IEventCallback>(gatewayCallback_->AsObject()));
        if (result != ERR_NONE) {
            HiLogError(LABEL, "Failed to register with backend: %{public}d", result);
            return result;
        }
        HiLogInfo(LABEL, "[Gateway] Registered callback with Backend");
    }

    HiLogInfo(LABEL, "[Gateway] Client callback registered");
    return ERR_NONE;
}

// Called when client dies
void GatewayServiceImpl::OnClientDied()
{
    clientCallback_ = nullptr;
    clientDeathRecipient_ = nullptr;
    HiLogInfo(LABEL, "[Gateway] Client callback cleared");
}

// Forward event from Backend to Client
void GatewayServiceImpl::ForwardEventToClient(int32_t event, const std::vector<int8_t> &data)
{
    if (clientCallback_ == nullptr) {
        HiLogWarn(LABEL, "[Gateway] No client callback (client may have died)");
        return;
    }

    HiLogInfo(LABEL, "[Gateway] Forwarding event %{public}d to client", event);
    ErrCode result = clientCallback_->OnEvent(event, data);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "[Gateway] Failed to forward event: %{public}d", result);
    }
}

} // namespace OHOS
