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

#include "callback_stub.h"
#include "hilog/log.h"
#include <unistd.h>

namespace OHOS {

// 日志标签
static constexpr HiviewDFX::HiLogLabel CALLBACK_LABEL = {
    LOG_CORE, 0xD001540, "CallbackStub"
};

CallbackStub::CallbackStub()
{
    HiLogInfo(CALLBACK_LABEL, "CallbackStub created");
}

int32_t CallbackStub::OnEvent(int32_t eventId, const std::string& data)
{
    pid_t pid = getpid();
    HiLogInfo(CALLBACK_LABEL, "[CallbackStub] OnEvent called, eventId=%{public}d, data=%{public}s, PID=%{public}d",
              eventId, data.c_str(), pid);
    return 0;
}

std::string CallbackStub::GetClientData()
{
    pid_t pid = getpid();
    HiLogInfo(CALLBACK_LABEL, "[CallbackStub] GetClientData called, PID=%{public}d", pid);
    return "Default Client Data";
}

int CallbackStub::OnRemoteRequest(uint32_t code, MessageParcel& data,
                                  MessageParcel& reply, MessageOption& option)
{
    // 检查接口描述符
    std::u16string descriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(CALLBACK_LABEL, "[CallbackStub] Interface token mismatch!");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case ON_EVENT:
            return HandleOnEvent(data, reply);
        case GET_CLIENT_DATA:
            return HandleGetClientData(data, reply);
        default:
            HiLogWarn(CALLBACK_LABEL, "[CallbackStub] Unknown code: %{public}u", code);
            return IRemoteStub<ICallback>::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t CallbackStub::HandleOnEvent(MessageParcel& data, MessageParcel& reply)
{
    int32_t eventId = data.ReadInt32();
    std::u16string data16 = data.ReadString16();
    std::string dataStr(data16.begin(), data16.end());

    int32_t result = OnEvent(eventId, dataStr);
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t CallbackStub::HandleGetClientData(MessageParcel& data, MessageParcel& reply)
{
    (void)data;
    std::string clientData = GetClientData();
    reply.WriteString16(std::u16string(clientData.begin(), clientData.end()));
    return ERR_NONE;
}

} // namespace OHOS
