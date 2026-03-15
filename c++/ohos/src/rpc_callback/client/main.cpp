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
#include "demo_service_interface.h"
#include "callback_stub.h"
#include "callback_interface.h"
#include "samgr_mini.h"
#include "samgr_proxy.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "hilog/log.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <atomic>

using namespace OHOS;

// 日志标签
static constexpr HiviewDFX::HiLogLabel CLIENT_LABEL = {
    LOG_CORE, 0xD001530, "DemoClient"
};

static std::atomic<bool> g_running(true);

/**
 * @brief CallbackImpl - Client端Callback服务实现
 *
 * 这个类在Client进程中运行，作为服务接收Server的回调调用
 */
class CallbackImpl : public CallbackStub {
public:
    CallbackImpl() : clientPid_(getpid()) {}
    ~CallbackImpl() override = default;

    // ICallback接口实现 - Server调用这些方法
    int32_t OnEvent(int32_t eventId, const std::string& data) override
    {
        HiLogInfo(CLIENT_LABEL, "[Client] OnEvent called by Server!");
        HiLogInfo(CLIENT_LABEL, "[Client]   eventId=%{public}d, data=%{public}s",
                  eventId, data.c_str());
        HiLogInfo(CLIENT_LABEL, "[Client]   Client PID=%{public}d", clientPid_);

        // 存储最后一次接收到的事件
        lastEventId_ = eventId;
        lastEventData_ = data;

        return 0;
    }

    std::string GetClientData() override
    {
        HiLogInfo(CLIENT_LABEL, "[Client] GetClientData() called by Server!");
        HiLogInfo(CLIENT_LABEL, "[Client]   Client PID=%{public}d", clientPid_);

        // 返回Client的一些数据
        std::string data = "Hello from Client PID=" + std::to_string(clientPid_);
        return data;
    }

private:
    pid_t clientPid_;
    int32_t lastEventId_ = 0;
    std::string lastEventData_;
};

/**
 * @brief DemoClient - 客户端进程（支持Callback双向通信）
 *
 * 流程：
 * 1. 创建CallbackImpl实例（在Client中运行的服务）
 * 2. 连接到Samgr（通过 IPCSkeleton::GetContextObject()）
 * 3. 查询DemoService（调用 CheckService("ohos.test.DemoService")）
 * 4. 调用 RegisterCallback(callbackImpl) 将Callback注册给Server
 * 5. 进入工作循环等待Server调用Callback
 * 6. 也可以主动调用Server的方法
 */
int main()
{
    pid_t pid = getpid();
    HiLogInfo(CLIENT_LABEL, "========================================");
    HiLogInfo(CLIENT_LABEL, "DemoClient starting, PID=%{public}d", pid);
    HiLogInfo(CLIENT_LABEL, "[Client] This client supports Callback mechanism!");
    HiLogInfo(CLIENT_LABEL, "========================================");

    // 1. 创建CallbackImpl实例（这个对象会被Server远程调用）
    sptr<CallbackImpl> callbackImpl = new CallbackImpl();
    HiLogInfo(CLIENT_LABEL, "[Client] CallbackImpl created, ready to register");

    // 2. 连接到Samgr
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to get Samgr remote object");
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Connected to SamgrServer");

    // 3. 查询DemoService
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to cast Samgr to IServiceRegistry");
        return -1;
    }

    std::u16string serviceName = u"ohos.test.DemoService";

    // 等待服务注册（可能需要重试）
    sptr<IRemoteObject> serviceRemote = nullptr;
    const int maxRetry = 50;
    const int sleepMs = 100;

    for (int i = 0; i < maxRetry; ++i) {
        serviceRemote = registry->CheckService(serviceName);
        if (serviceRemote != nullptr) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    if (serviceRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "DemoService not found");
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Found DemoService");

    // 4. 创建DemoService代理
    sptr<IDemoService> demoService = iface_cast<IDemoService>(serviceRemote);
    if (demoService == nullptr) {
        demoService = new DemoServiceProxy(serviceRemote);
    }

    // 5. 注册Callback到Server
    HiLogInfo(CLIENT_LABEL, "[Client] Registering callback to Server...");
    sptr<IRemoteObject> callbackRemote = callbackImpl->AsObject();
    int32_t regResult = demoService->RegisterCallback(callbackRemote);
    if (regResult != ERR_NONE) {
        HiLogError(CLIENT_LABEL, "[Client] RegisterCallback failed: %{public}d", regResult);
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Callback registered successfully!");

    // 6. 调用Server的方法，Server会通过Callback通知我们
    HiLogInfo(CLIENT_LABEL, "[Client] Calling Server->Ping()...");
    int32_t pingResult = demoService->Ping();
    if (pingResult == ERR_NONE) {
        HiLogInfo(CLIENT_LABEL, "[Client] Ping() succeeded");
    } else {
        HiLogError(CLIENT_LABEL, "[Client] Ping() failed: %{public}d", pingResult);
    }

    // Test 2: Add
    HiLogInfo(CLIENT_LABEL, "[Client] Calling Server->Add(100, 200)...");
    int32_t addResult = demoService->Add(100, 200);
    HiLogInfo(CLIENT_LABEL, "[Client] Add(100, 200) = %{public}d", addResult);

    // Test 3: GetName
    HiLogInfo(CLIENT_LABEL, "[Client] Calling Server->GetName()...");
    std::string name = demoService->GetName();
    HiLogInfo(CLIENT_LABEL, "[Client] GetName() = %{public}s", name.c_str());

    // 7. 进入工作循环，等待Server调用Callback
    HiLogInfo(CLIENT_LABEL, "[Client] Entering work loop, waiting for Server callbacks...");
    HiLogInfo(CLIENT_LABEL, "[Client] Press Ctrl+C to exit");

    IPCSkeleton::JoinWorkThread();

    HiLogInfo(CLIENT_LABEL, "[Client] DemoClient stopped");
    return 0;
}
