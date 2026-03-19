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

#include "backend_service_stub.h"
#include "ipc_skeleton.h"
#include "hilog/log.h"
#include <unistd.h>
#include <cstring>

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001533, "BackendServiceStub"};

BackendServiceStub::BackendServiceStub()
{
    HiLogInfo(LABEL, "BackendServiceStub created");
}

int32_t BackendServiceStub::RegisterGatewayCallback(const sptr<IEventCallback> &callback)
{
    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    gatewayCallback_ = new EventCallbackProxy(callback->AsObject());
    HiLogInfo(LABEL, "Gateway callback registered");
    return ERR_NONE;
}

int32_t BackendServiceStub::UnregisterGatewayCallback()
{
    gatewayCallback_ = nullptr;
    HiLogInfo(LABEL, "Gateway callback unregistered");
    return ERR_NONE;
}

int32_t BackendServiceStub::TriggerEvent(int32_t event, const std::vector<int8_t> &data)
{
    if (gatewayCallback_ == nullptr) {
        HiLogWarn(LABEL, "No gateway callback registered, cannot trigger event");
        return ERR_INVALID_DATA;
    }

    HiLogInfo(LABEL, "Triggering event %{public}d to gateway", event);
    ErrCode result = gatewayCallback_->OnEvent(event, data);
    HiLogInfo(LABEL, "Event callback returned: %{public}d", result);
    return result;
}

std::string BackendServiceStub::GetServiceInfo()
{
    pid_t pid = getpid();
    return "BackendService running on PID=" + std::to_string(pid);
}

int BackendServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                         MessageParcel &reply, MessageOption &option)
{
    std::u16string descriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(LABEL, "Interface token mismatch");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case REGISTER_GATEWAY_CALLBACK:
            return HandleRegisterGatewayCallback(data, reply);
        case UNREGISTER_GATEWAY_CALLBACK:
            return HandleUnregisterGatewayCallback(data, reply);
        case TRIGGER_EVENT:
            return HandleTriggerEvent(data, reply);
        case GET_SERVICE_INFO:
            return HandleGetServiceInfo(data, reply);
        default:
            return IRemoteStub<IBackendService>::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t BackendServiceStub::HandleRegisterGatewayCallback(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> callback = data.ReadRemoteObject();
    int32_t result = RegisterGatewayCallback(iface_cast<IEventCallback>(callback));
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t BackendServiceStub::HandleUnregisterGatewayCallback(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t result = UnregisterGatewayCallback();
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t BackendServiceStub::HandleTriggerEvent(MessageParcel &data, MessageParcel &reply)
{
    int32_t event = data.ReadInt32();
    int32_t dataSize = data.ReadInt32();
    std::vector<int8_t> eventData;
    if (dataSize > 0) {
        eventData.resize(dataSize);
        const void *buffer = data.ReadBuffer(static_cast<size_t>(dataSize), false);
        if (buffer != nullptr) {
            memcpy(eventData.data(), buffer, dataSize);
        }
    }
    int32_t result = TriggerEvent(event, eventData);
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t BackendServiceStub::HandleGetServiceInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::string info = GetServiceInfo();
    reply.WriteString16(std::u16string(info.begin(), info.end()));
    return ERR_NONE;
}

} // namespace OHOS
