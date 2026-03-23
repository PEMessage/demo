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

#include "event_callback_stub.h"
#include "ipc_types.h"
#include "message_parcel.h"
#include "message_option.h"
#include "hilog/log.h"
#include <cstring>

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001531, "EventCallbackStub"};

EventCallbackStub::EventCallbackStub() {}

int EventCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                        MessageParcel &reply, MessageOption &option)
{
    std::u16string descriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(LABEL, "Interface token mismatch");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case ON_EVENT:
            return HandleOnEvent(data, reply);
        default:
            return IRemoteStub<IEventCallback>::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t EventCallbackStub::HandleOnEvent(MessageParcel &data, MessageParcel &reply)
{
    int32_t event = data.ReadInt32();
    
    int32_t dataSize = data.ReadInt32();
    std::vector<int8_t> reqData;
    if (dataSize > 0) {
        reqData.resize(dataSize);
        const void *buffer = data.ReadBuffer(static_cast<size_t>(dataSize), false);
        if (buffer != nullptr) {
            memcpy(reqData.data(), buffer, dataSize);
        }
    }

    ErrCode result = OnEvent(event, reqData);
    reply.WriteInt32(result);
    return ERR_NONE;
}

} // namespace OHOS
