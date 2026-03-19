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

#include "demo_service_stub.h"
#include "hilog/log.h"
#include "ipc_skeleton.h"
#include <unistd.h>

namespace OHOS {

// 日志标签
static constexpr HiviewDFX::HiLogLabel DEMO_LABEL = {
    LOG_CORE, 0xD001520, "DemoService"
};

DemoServiceStub::DemoServiceStub()
{
    HiLogInfo(DEMO_LABEL, "DemoServiceStub created");
}

void DemoServiceStub::CallbackDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    HiLogWarn(DEMO_LABEL, "[Server] Callback client died, clearing callback proxy");
    if (stub_ != nullptr) {
        stub_->callbackProxy_ = nullptr;
        stub_->callbackDeathRecipient_ = nullptr;
    }
}

int32_t DemoServiceStub::Ping()
{
    pid_t pid = getpid();
    pid_t callingPid = IPCSkeleton::GetCallingPid();
    HiLogInfo(DEMO_LABEL, "[Server] Ping() called, server PID=%{public}d, client PID=%{public}d",
              pid, callingPid);

    // 如果有Callback代理，通知Client
    if (callbackProxy_ != nullptr) {
        HiLogInfo(DEMO_LABEL, "[Server] Calling callback->OnEvent(100, \"Ping received\")...");
        int32_t cbResult = callbackProxy_->OnEvent(100, "Ping received");
        HiLogInfo(DEMO_LABEL, "[Server] Callback OnEvent returned: %{public}d", cbResult);
    }

    return 0;
}

int32_t DemoServiceStub::Add(int32_t a, int32_t b)
{
    pid_t pid = getpid();
    pid_t callingPid = IPCSkeleton::GetCallingPid();
    int32_t result = a + b;
    HiLogInfo(DEMO_LABEL, "[Server] Add(%{public}d, %{public}d) = %{public}d, server PID=%{public}d, client PID=%{public}d",
              a, b, result, pid, callingPid);
    return result;
}

std::string DemoServiceStub::GetName()
{
    HiLogInfo(DEMO_LABEL, "[Server] GetName() called");
    return "DemoService with Callback";
}

int32_t DemoServiceStub::RegisterCallback(const sptr<IRemoteObject>& callback)
{
    pid_t callingPid = IPCSkeleton::GetCallingPid();
    HiLogInfo(DEMO_LABEL, "[Server] RegisterCallback called from PID=%{public}d", callingPid);

    if (callback == nullptr) {
        HiLogError(DEMO_LABEL, "[Server] RegisterCallback: callback is null!");
        return ERR_INVALID_DATA;
    }

    // 创建Callback代理
    callbackProxy_ = new CallbackProxy(callback);
    
    // 注册死亡回调，当client死亡时清理callbackProxy_
    callbackDeathRecipient_ = new CallbackDeathRecipient(this);
    if (!callback->AddDeathRecipient(callbackDeathRecipient_)) {
        HiLogWarn(DEMO_LABEL, "[Server] Failed to add death recipient for callback");
    }
    
    HiLogInfo(DEMO_LABEL, "[Server] Client registered callback successfully");

    return ERR_NONE;
}

void DemoServiceStub::TestCallback()
{
    if (callbackProxy_ == nullptr) {
        HiLogWarn(DEMO_LABEL, "[Server] No callback registered (client may have died), skipping callback test");
        return;
    }

    HiLogInfo(DEMO_LABEL, "[Server] ========== Testing Callback ==========");

    // 测试OnEvent
    HiLogInfo(DEMO_LABEL, "[Server] Testing callback->OnEvent(200, \"Test from Server\")...");
    int32_t eventResult = callbackProxy_->OnEvent(200, "Test from Server");
    HiLogInfo(DEMO_LABEL, "[Server] OnEvent returned: %{public}d", eventResult);

    // 测试GetClientData
    HiLogInfo(DEMO_LABEL, "[Server] Testing callback->GetClientData()...");
    std::string clientData = callbackProxy_->GetClientData();
    HiLogInfo(DEMO_LABEL, "[Server] GetClientData returned: %{public}s", clientData.c_str());

    HiLogInfo(DEMO_LABEL, "[Server] ========== Callback Test Complete ==========");
}

int DemoServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data,
                                      MessageParcel& reply, MessageOption& option)
{
    // 检查接口描述符
    std::u16string descriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(DEMO_LABEL, "[Server] Interface token mismatch!");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case PING:
            return HandlePing(data, reply);
        case ADD:
            return HandleAdd(data, reply);
        case GET_NAME:
            return HandleGetName(data, reply);
        case REGISTER_CALLBACK:
            return HandleRegisterCallback(data, reply);
        default:
            HiLogWarn(DEMO_LABEL, "[Server] Unknown code: %{public}u", code);
            return IRemoteStub<IDemoService>::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t DemoServiceStub::HandlePing(MessageParcel& data, MessageParcel& reply)
{
    (void)data;
    int32_t result = Ping();
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t DemoServiceStub::HandleAdd(MessageParcel& data, MessageParcel& reply)
{
    int32_t a = data.ReadInt32();
    int32_t b = data.ReadInt32();
    int32_t result = Add(a, b);
    reply.WriteInt32(result);
    return ERR_NONE;
}

int32_t DemoServiceStub::HandleGetName(MessageParcel& data, MessageParcel& reply)
{
    (void)data;
    std::string name = GetName();
    reply.WriteString16(std::u16string(name.begin(), name.end()));
    return ERR_NONE;
}

int32_t DemoServiceStub::HandleRegisterCallback(MessageParcel& data, MessageParcel& reply)
{
    // 读取Callback的远程对象
    sptr<IRemoteObject> callback = data.ReadRemoteObject();
    int32_t result = RegisterCallback(callback);
    reply.WriteInt32(result);
    return ERR_NONE;
}

} // namespace OHOS
