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

#include "backend_service_proxy.h"
#include "message_parcel.h"
#include "message_option.h"
#include "hilog/log.h"

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001532, "BackendServiceProxy"};

BackendServiceProxy::BackendServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IBackendService>(impl) {}

int32_t BackendServiceProxy::RegisterGatewayCallback(const sptr<IEventCallback> &callback)
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

    int32_t result = Remote()->SendRequest(REGISTER_GATEWAY_CALLBACK, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

int32_t BackendServiceProxy::UnregisterGatewayCallback()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return ERR_INVALID_DATA;
    }

    int32_t result = Remote()->SendRequest(UNREGISTER_GATEWAY_CALLBACK, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

int32_t BackendServiceProxy::TriggerEvent(int32_t event, const std::vector<int8_t> &data)
{
    MessageParcel parcel;
    MessageParcel reply;
    MessageOption option;

    if (!parcel.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return ERR_INVALID_DATA;
    }

    if (!parcel.WriteInt32(event)) {
        HiLogError(LABEL, "Failed to write event");
        return ERR_INVALID_DATA;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(data.size()))) {
        HiLogError(LABEL, "Failed to write data size");
        return ERR_INVALID_DATA;
    }

    if (!data.empty()) {
        if (!parcel.WriteBuffer(data.data(), data.size())) {
            HiLogError(LABEL, "Failed to write data");
            return ERR_INVALID_DATA;
        }
    }

    int32_t result = Remote()->SendRequest(TRIGGER_EVENT, parcel, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

std::string BackendServiceProxy::GetServiceInfo()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return "";
    }

    int32_t result = Remote()->SendRequest(GET_SERVICE_INFO, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return "";
    }

    std::u16string info16 = reply.ReadString16();
    return std::string(info16.begin(), info16.end());
}

} // namespace OHOS
