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

#include "event_callback_proxy.h"
#include "ipc_types.h"
#include "message_parcel.h"
#include "message_option.h"
#include "hilog/log.h"

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001530, "EventCallbackProxy"};

EventCallbackProxy::EventCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IEventCallback>(impl) {}

ErrCode EventCallbackProxy::OnEvent(int32_t event, const std::vector<int8_t> &reqData)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(LABEL, "Failed to write interface token");
        return ERR_INVALID_DATA;
    }

    if (!data.WriteInt32(event)) {
        HiLogError(LABEL, "Failed to write event");
        return ERR_INVALID_DATA;
    }

    // Write vector<int8_t> as raw bytes
    if (!data.WriteInt32(static_cast<int32_t>(reqData.size()))) {
        HiLogError(LABEL, "Failed to write data size");
        return ERR_INVALID_DATA;
    }
    
    if (!reqData.empty()) {
        if (!data.WriteBuffer(reqData.data(), reqData.size())) {
            HiLogError(LABEL, "Failed to write data");
            return ERR_INVALID_DATA;
        }
    }

    int32_t result = Remote()->SendRequest(ON_EVENT, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

} // namespace OHOS
