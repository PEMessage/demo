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

#include "gateway_service_proxy.h"
#include "message_parcel.h"
#include "message_option.h"
#include "hilog/log.h"

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001534, "GatewayServiceProxy"};

GatewayServiceProxy::GatewayServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IGatewayService>(impl) {}

int32_t GatewayServiceProxy::RegisterClientCallback(const sptr<IEventCallback> &callback,
                                                     const std::vector<int32_t> &filterEventTypes)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return ERR_INVALID_DATA;
    }

    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    if (!data.WriteRemoteObject(callback->AsObject())) {
        HiLogError(LABEL, "Failed to write callback");
        return ERR_INVALID_DATA;
    }

    // Write filter event types
    if (!data.WriteInt32(static_cast<int32_t>(filterEventTypes.size()))) {
        HiLogError(LABEL, "Failed to write filter size");
        return ERR_INVALID_DATA;
    }
    for (int32_t type : filterEventTypes) {
        if (!data.WriteInt32(type)) {
            HiLogError(LABEL, "Failed to write filter type");
            return ERR_INVALID_DATA;
        }
    }

    int32_t result = Remote()->SendRequest(REGISTER_CLIENT_CALLBACK, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

int32_t GatewayServiceProxy::UnregisterClientCallback(const sptr<IEventCallback> &callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return ERR_INVALID_DATA;
    }

    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    if (!data.WriteRemoteObject(callback->AsObject())) {
        HiLogError(LABEL, "Failed to write callback");
        return ERR_INVALID_DATA;
    }

    int32_t result = Remote()->SendRequest(UNREGISTER_CLIENT_CALLBACK, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

} // namespace OHOS
