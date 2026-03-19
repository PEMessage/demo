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

#include "gateway_service_stub.h"
#include "hilog/log.h"
#include "ipc_skeleton.h"
#include <unistd.h>

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001535, "GatewayServiceStub"};

// GatewayEventCallback implementation
GatewayServiceStub::GatewayEventCallback::GatewayEventCallback(GatewayServiceStub *gateway)
    : gateway_(gateway) {}

ErrCode GatewayServiceStub::GatewayEventCallback::OnEvent(int32_t event, const std::vector<int8_t> &reqData)
{
    if (gateway_ == nullptr) {
        return ERR_INVALID_DATA;
    }
    HiLogInfo(LABEL, "[Gateway] Received event %{public}d from Backend, forwarding to clients", event);
    gateway_->ForwardEventToClients(event, reqData);
    return ERR_NONE;
}

// ClientDeathRecipient implementation
GatewayServiceStub::ClientDeathRecipient::ClientDeathRecipient(GatewayServiceStub *gateway,
                                                                sptr<IRemoteObject> clientCallback)
    : gateway_(gateway), clientCallback_(clientCallback) {}

void GatewayServiceStub::ClientDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    HiLogWarn(LABEL, "[Gateway] Client callback died, removing from registry");
    if (gateway_ != nullptr) {
        sptr<IRemoteObject> obj = object.promote();
        if (obj != nullptr) {
            gateway_->RemoveClientCallback(obj);
        }
    }
}

// GatewayServiceStub implementation
GatewayServiceStub::GatewayServiceStub()
{
    HiLogInfo(LABEL, "GatewayServiceStub created");
    gatewayCallback_ = new GatewayEventCallback(this);
    gatewayCallbackForBackend_ = gatewayCallback_->AsObject();
}

bool GatewayServiceStub::ConnectToBackend(const sptr<IRemoteObject> &backendRemote)
{
    if (backendRemote == nullptr) {
        HiLogError(LABEL, "Backend remote is null");
        return false;
    }

    backendProxy_ = new BackendServiceProxy(backendRemote);
    
    // Register Gateway's callback with Backend
    int32_t result = backendProxy_->RegisterGatewayCallback(
        iface_cast<IEventCallback>(gatewayCallbackForBackend_));
    if (result != ERR_NONE) {
        HiLogError(LABEL, "Failed to register gateway callback with backend: %{public}d", result);
        backendProxy_ = nullptr;
        return false;
    }

    HiLogInfo(LABEL, "[Gateway] Successfully connected to Backend and registered callback");
    return true;
}

void GatewayServiceStub::DisconnectFromBackend()
{
    if (backendProxy_ != nullptr) {
        backendProxy_->UnregisterGatewayCallback();
        backendProxy_ = nullptr;
        HiLogInfo(LABEL, "[Gateway] Disconnected from Backend");
    }
}

sptr<IRemoteObject> GatewayServiceStub::GetGatewayCallbackForBackend()
{
    return gatewayCallbackForBackend_;
}

int32_t GatewayServiceStub::RegisterClientCallback(const sptr<IEventCallback> &callback,
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

    // Create client info
    ClientCallbackInfo info;
    info.callback = new EventCallbackProxy(callbackObj);
    info.filterTypes = filterEventTypes;
    info.deathRecipient = new ClientDeathRecipient(this, callbackObj);

    // Register death recipient
    if (!callbackObj->AddDeathRecipient(info.deathRecipient)) {
        HiLogWarn(LABEL, "[Gateway] Failed to add death recipient for client callback");
    }

    clientCallbacks_[callbackObj] = info;
    
    HiLogInfo(LABEL, "[Gateway] Client callback registered, filters: %{public}zu types", 
              filterEventTypes.size());
    return ERR_NONE;
}

int32_t GatewayServiceStub::UnregisterClientCallback(const sptr<IEventCallback> &callback)
{
    if (callback == nullptr) {
        return ERR_INVALID_DATA;
    }

    RemoveClientCallback(callback->AsObject());
    return ERR_NONE;
}

void GatewayServiceStub::RemoveClientCallback(const sptr<IRemoteObject> &callback)
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
        HiLogInfo(LABEL, "[Gateway] Client callback removed");
    }
}

bool GatewayServiceStub::ShouldForwardEvent(const ClientCallbackInfo &info, int32_t event)
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

void GatewayServiceStub::ForwardEventToClients(int32_t event, const std::vector<int8_t> &data)
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

int GatewayServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                         MessageParcel &reply, MessageOption &option)
{
    std::u16string descriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(LABEL, "Interface token mismatch");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case REGISTER_CLIENT_CALLBACK:
            return HandleRegisterClientCallback(data, reply);
        case UNREGISTER_CLIENT_CALLBACK:
            return HandleUnregisterClientCallback(data, reply);
        default:
            return IRemoteStub<IGatewayService>::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t GatewayServiceStub::HandleRegisterClientCallback(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> callbackObj = data.ReadRemoteObject();
    if (callbackObj == nullptr) {
        HiLogError(LABEL, "Failed to read callback object");
        reply.WriteInt32(ERR_INVALID_DATA);
        return ERR_NONE;
    }

    // Read filter event types
    int32_t filterSize = data.ReadInt32();
    std::vector<int32_t> filterTypes;
    for (int32_t i = 0; i < filterSize && i < 100; i++) {  // Limit to 100 types
        filterTypes.push_back(data.ReadInt32());
    }

    sptr<IEventCallback> callback = iface_cast<IEventCallback>(callbackObj);
    int32_t result = RegisterClientCallback(callback, filterTypes);
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t GatewayServiceStub::HandleUnregisterClientCallback(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> callbackObj = data.ReadRemoteObject();
    if (callbackObj == nullptr) {
        reply.WriteInt32(ERR_INVALID_DATA);
        return ERR_NONE;
    }

    sptr<IEventCallback> callback = iface_cast<IEventCallback>(callbackObj);
    int32_t result = UnregisterClientCallback(callback);
    reply.WriteInt32(result);
    return ERR_NONE;
}

} // namespace OHOS
