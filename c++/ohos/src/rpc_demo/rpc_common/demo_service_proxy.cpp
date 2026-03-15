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

#include "demo_service_proxy.h"
#include "hilog/log.h"

namespace OHOS {

// 日志标签
static constexpr HiviewDFX::HiLogLabel DEMO_LABEL = {
    LOG_CORE, 0xD001520, "DemoService"
};

DemoServiceProxy::DemoServiceProxy(const sptr<IRemoteObject>& impl)
    : IRemoteProxy<IDemoService>(impl)
{
    HiLogInfo(DEMO_LABEL, "DemoServiceProxy created");
}

int32_t DemoServiceProxy::Ping()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    data.WriteInterfaceToken(GetDescriptor());

    // 发送远程请求
    int32_t result = Remote()->SendRequest(PING, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(DEMO_LABEL, "[Client] Ping SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

int32_t DemoServiceProxy::Add(int32_t a, int32_t b)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    data.WriteInterfaceToken(GetDescriptor());

    // 写入参数
    data.WriteInt32(a);
    data.WriteInt32(b);

    // 发送远程请求
    int32_t result = Remote()->SendRequest(ADD, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(DEMO_LABEL, "[Client] Add SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

std::string DemoServiceProxy::GetName()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    data.WriteInterfaceToken(GetDescriptor());

    // 发送远程请求
    int32_t result = Remote()->SendRequest(GET_NAME, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(DEMO_LABEL, "[Client] GetName SendRequest failed: %{public}d", result);
        return "";
    }

    // 读取返回的字符串
    std::u16string name16 = reply.ReadString16();
    return std::string(name16.begin(), name16.end());
}

} // namespace OHOS
