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

#include "samgr_proxy.h"
#include "hilog/log.h"

namespace OHOS {

// 日志标签
static constexpr HiviewDFX::HiLogLabel SAMGR_LABEL = {
    LOG_CORE, 0xD001510, "SamgrProxy"
};

SamgrProxy::SamgrProxy(const sptr<IRemoteObject>& impl)
    : IRemoteProxy<IServiceRegistry>(impl)
{
    HiLogInfo(SAMGR_LABEL, "SamgrProxy created");
}

int32_t SamgrProxy::AddService(const std::u16string& name,
                               const sptr<IRemoteObject>& service)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(SAMGR_LABEL, "WriteInterfaceToken failed");
        return ERR_INVALID_DATA;
    }

    // 写入服务名
    if (!data.WriteString16(name)) {
        HiLogError(SAMGR_LABEL, "WriteString16 failed");
        return ERR_INVALID_DATA;
    }

    // 写入服务对象
    if (!data.WriteRemoteObject(service)) {
        HiLogError(SAMGR_LABEL, "WriteRemoteObject failed");
        return ERR_INVALID_DATA;
    }

    // 发送远程请求
    int32_t result = Remote()->SendRequest(ADD_SERVICE, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(SAMGR_LABEL, "SendRequest failed: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

sptr<IRemoteObject> SamgrProxy::GetService(const std::u16string& name)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(SAMGR_LABEL, "WriteInterfaceToken failed");
        return nullptr;
    }

    // 写入服务名
    if (!data.WriteString16(name)) {
        HiLogError(SAMGR_LABEL, "WriteString16 failed");
        return nullptr;
    }

    // 发送远程请求
    int32_t result = Remote()->SendRequest(GET_SERVICE, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(SAMGR_LABEL, "SendRequest failed: %{public}d", result);
        return nullptr;
    }

    return reply.ReadRemoteObject();
}

sptr<IRemoteObject> SamgrProxy::CheckService(const std::u16string& name)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    // 写入接口描述符
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLogError(SAMGR_LABEL, "WriteInterfaceToken failed");
        return nullptr;
    }

    // 写入服务名
    if (!data.WriteString16(name)) {
        HiLogError(SAMGR_LABEL, "WriteString16 failed");
        return nullptr;
    }

    // 发送远程请求
    int32_t result = Remote()->SendRequest(CHECK_SERVICE, data, reply, option);
    if (result != ERR_NONE) {
        HiLogError(SAMGR_LABEL, "SendRequest failed: %{public}d", result);
        return nullptr;
    }

    return reply.ReadRemoteObject();
}

} // namespace OHOS
