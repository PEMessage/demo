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
#include <unistd.h>

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001536, "GatewayServiceImpl"};

// Local death recipient class
class GatewayClientDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit GatewayClientDeathRecipient(GatewayServiceImpl *gateway, sptr<IRemoteObject> clientCallback)
        : gateway_(gateway), clientCallback_(clientCallback) {}
    void OnRemoteDied(const wptr<IRemoteObject> &object) override {
        HiLogWarn(LABEL, "[Gateway] Client callback died, removing from registry");
        if (gateway_ != nullptr) {
            sptr<IRemoteObject> obj = object.promote();
            if (obj != nullptr) {
                gateway_->RemoveClientCallback(obj);
            }
        }
    }
private:
    GatewayServiceImpl *gateway_;
    wptr<IRemoteObject> clientCallback_;
};

// GatewayEventCallback implementation
GatewayServiceImpl::GatewayEventCallback::GatewayEventCallback(GatewayServiceImpl *gateway)
    : gateway_(gateway) {}

ErrCode GatewayServiceImpl::GatewayEventCallback::OnEvent(int32_t event, const std::vector<int8_t> &reqData)
{
    if (gateway_ == nullptr) {
        return ERR_INVALID_DATA;
    }
    HiLogInfo(LABEL, "[Gateway] Received event %{public}d from Backend, forwarding to clients", event);
    gateway_->ForwardEventToClients(event, reqData);
    return ERR_NONE;
}

// GatewayServiceImpl implementation
GatewayServiceImpl::GatewayServiceImpl()
{
    HiLogInfo(LABEL, "GatewayServiceImpl created");
    gatewayCallback_ = new GatewayEventCallback(this);
    gatewayCallbackForBackend_ = gatewayCallback_->AsObject();
}

bool GatewayServiceImpl::ConnectToBackend(const sptr<IRemoteObject> &backendRemote)
{
    if (backendRemote == nullptr) {
        HiLogError(LABEL, "Backend remote is null");
        return false;
    }

    backendProxy_ = new BackendServiceProxy(backendRemote);
    
    // Note: We don't register callback with Backend here.
    // Callback is only registered when the first Client registers.
    HiLogInfo(LABEL, "[Gateway] Connected to Backend (callback not registered yet, waiting for clients)");
    return true;
}

sptr<IRemoteObject> GatewayServiceImpl::GetGatewayCallbackForBackend()
{
    return gatewayCallbackForBackend_;
}

int32_t GatewayServiceImpl::RegisterClientCallback(const sptr<IEventCallback> &callback,
                                                    const std::vector<int32_t> &filterEventTypes)
{
    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> callbackObj = callback->AsObject();
    if (callbackObj == nullptr) {
        HiLogError(LABEL, "Callback object is null");
        return ERR_INVALID_DATA;
    }

    std::lock_guard<std::mutex> lock(clientCallbacksMutex_);
    
    // Check if already registered
    if (clientCallbacks_.find(callbackObj) != clientCallbacks_.end()) {
        HiLogWarn(LABEL, "[Gateway] Client callback already registered");
        return ERR_NONE;  // Already registered, treat as success
    }

    // If this is the first client, register with Backend first
    if (clientCallbacks_.empty()) {
        if (backendProxy_ == nullptr) {
            HiLogError(LABEL, "[Gateway] Backend proxy is null");
            return ERR_INVALID_DATA;
        }
        int32_t result = backendProxy_->RegisterGatewayCallback(
            iface_cast<IEventCallback>(gatewayCallbackForBackend_));
        if (result != ERR_NONE) {
            HiLogError(LABEL, "[Gateway] Failed to register callback with backend: %{public}d", result);
            return ERR_INVALID_DATA;
        }
        HiLogInfo(LABEL, "[Gateway] Registered callback with Backend");
    }

    // Create client info
    ClientCallbackInfo info;
    info.callback = new EventCallbackProxy(callbackObj);
    info.filterTypes = filterEventTypes;
    info.deathRecipient = new GatewayClientDeathRecipient(this, callbackObj);

    // Register death recipient
    if (!callbackObj->AddDeathRecipient(info.deathRecipient)) {
        HiLogWarn(LABEL, "[Gateway] Failed to add death recipient for client callback");
    }

    clientCallbacks_[callbackObj] = info;
    
    HiLogInfo(LABEL, "[Gateway] Client callback registered, filters: %{public}zu types, total clients: %{public}zu", 
              filterEventTypes.size(), clientCallbacks_.size());
    return ERR_NONE;
}

void GatewayServiceImpl::RemoveClientCallback(const sptr<IRemoteObject> &callback)
{
    if (callback == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(clientCallbacksMutex_);
    auto it = clientCallbacks_.find(callback);
    if (it != clientCallbacks_.end()) {
        // Remove death recipient
        callback->RemoveDeathRecipient(it->second.deathRecipient);
        clientCallbacks_.erase(it);
        HiLogInfo(LABEL, "[Gateway] Client callback removed, remaining clients: %{public}zu", 
                  clientCallbacks_.size());
    }
}

bool GatewayServiceImpl::ShouldForwardEvent(const ClientCallbackInfo &info, int32_t event)
{
    // If no filter specified, forward all events
    if (info.filterTypes.empty()) {
        return true;
    }

    // Check if event type is in filter list
    for (int32_t type : info.filterTypes) {
        if (type == event) {
            return true;
        }
    }
    return false;
}

void GatewayServiceImpl::ForwardEventToClients(int32_t event, const std::vector<int8_t> &data)
{
    std::lock_guard<std::mutex> lock(clientCallbacksMutex_);
    
    HiLogInfo(LABEL, "[Gateway] Forwarding event %{public}d to %{public}zu clients", 
              event, clientCallbacks_.size());

    for (auto &pair : clientCallbacks_) {
        const ClientCallbackInfo &info = pair.second;
        
        if (!ShouldForwardEvent(info, event)) {
            HiLogInfo(LABEL, "[Gateway] Event %{public}d filtered out for this client", event);
            continue;
        }

        HiLogInfo(LABEL, "[Gateway] Forwarding event %{public}d to client", event);
        ErrCode result = info.callback->OnEvent(event, data);
        if (result != ERR_NONE) {
            HiLogError(LABEL, "[Gateway] Failed to forward event to client: %{public}d", result);
        }
    }
}

} // namespace OHOS
