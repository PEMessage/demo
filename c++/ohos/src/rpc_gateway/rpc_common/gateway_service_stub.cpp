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
#include "event_callback_proxy.h"

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001535, "GatewayServiceStub"};

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
