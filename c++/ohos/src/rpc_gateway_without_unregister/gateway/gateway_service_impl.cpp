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

// GatewayServiceImpl implementation
GatewayServiceImpl::GatewayServiceImpl()
{
    HiLogInfo(LABEL, "GatewayServiceImpl created");
    gatewayCallback_ = new GatewayEventCallback(this);
}

bool GatewayServiceImpl::ConnectToBackend(const sptr<IRemoteObject> &backendRemote)
{
    if (backendRemote == nullptr) {
        HiLogError(LABEL, "Backend remote is null");
        return false;
    }

    backendProxy_ = new BackendServiceProxy(backendRemote);
    
    // Register Gateway's callback with Backend immediately
    int32_t result = backendProxy_->RegisterGatewayCallback(
        iface_cast<IEventCallback>(gatewayCallback_->AsObject()));
    if (result != ERR_NONE) {
        HiLogError(LABEL, "Failed to register with backend: %{public}d", result);
        return false;
    }
    
    HiLogInfo(LABEL, "[Gateway] Connected to Backend");
    return true;
}

int32_t GatewayServiceImpl::RegisterClientCallback(const sptr<IEventCallback> &callback,
                                                    const std::vector<int32_t> &filterEventTypes)
{
    (void)filterEventTypes;  // Not used in simplified version
    
    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    // Simply store the client callback
    clientCallback_ = new EventCallbackProxy(callback->AsObject());
    HiLogInfo(LABEL, "[Gateway] Client callback registered");
    return ERR_NONE;
}

void GatewayServiceImpl::ForwardEventToClient(int32_t event, const std::vector<int8_t> &data)
{
    // When client dies, clientCallback_ becomes null automatically via Binder death detection
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
