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

#include "callback_proxy.h"
#include "hilog/log.h"

namespace OHOS {

// 日志标签
static constexpr HiviewDFX::HiLogLabel CALLBACK_LABEL = {
    LOG_CORE, 0xD001540, "CallbackProxy"
};

CallbackProxy::CallbackProxy(const sptr<IRemoteObject>& impl)
    : IRemoteProxy<ICallback>(impl)
{
    HiLogInfo(CALLBACK_LABEL, "CallbackProxy created");
}

int32_t CallbackProxy::OnEvent(int32_t eventId, const std::string& data)
{
    MessageParcel dataParcel;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    dataParcel.WriteInterfaceToken(GetDescriptor());

    // 写入参数
    dataParcel.WriteInt32(eventId);
    dataParcel.WriteString16(std::u16string(data.begin(), data.end()));

    // 发送远程请求到Client
    int32_t result = Remote()->SendRequest(ON_EVENT, dataParcel, reply, option);
    if (result != ERR_NONE) {
        HiLogError(CALLBACK_LABEL, "[CallbackProxy] OnEvent SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

std::string CallbackProxy::GetClientData()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    data.WriteInterfaceToken(GetDescriptor());

    // 发送远程请求到Client
    int32_t result = Remote()->SendRequest(GET_CLIENT_DATA, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(CALLBACK_LABEL, "[CallbackProxy] GetClientData SendRequest failed: %{public}d", result);
        return "";
    }

    // 读取返回的字符串
    std::u16string data16 = reply.ReadString16();
    return std::string(data16.begin(), data16.end());
}

} // namespace OHOS
