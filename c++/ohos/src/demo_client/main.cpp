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
#include "samgr_mini.h"
#include "samgr_proxy.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "hilog/log.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>

using namespace OHOS;

// 日志标签
static constexpr HiviewDFX::HiLogLabel CLIENT_LABEL = {
    LOG_CORE, 0xD001530, "DemoClient"
};

/**
 * @brief DemoClient - 客户端进程
 *
 * 流程：
 * 1. 连接到Samgr（通过 IPCSkeleton::GetContextObject()）
 * 2. 查询DemoService（调用 CheckService("ohos.test.DemoService")）
 * 3. Samgr返回DemoService的远程对象
 * 4. 直接连接到DemoService进行RPC调用（不经过Samgr）
 */
int main()
{
    pid_t pid = getpid();
    HiLogInfo(CLIENT_LABEL, "========================================");
    HiLogInfo(CLIENT_LABEL, "DemoClient starting, PID=%{public}d", pid);
    HiLogInfo(CLIENT_LABEL, "========================================");

    // 1. 连接到Samgr
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to get Samgr remote object");
        return -1;
    }

    // 2. 查询DemoService
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
    HiLogInfo(CLIENT_LABEL, "Found DemoService");

    // 3. 直接连接到DemoService（不再经过Samgr）
    sptr<IDemoService> demoService = iface_cast<IDemoService>(serviceRemote);
    if (demoService == nullptr) {
        // 如果没有注册代理创建器，直接使用DemoServiceProxy
        demoService = new DemoServiceProxy(serviceRemote);
    }

    int32_t pingResult = demoService->Ping();
    if (pingResult == ERR_NONE) {
        HiLogInfo(CLIENT_LABEL, "Ping() succeeded");
    } else {
        HiLogError(CLIENT_LABEL, "Ping() failed: %{public}d", pingResult);
    }

    // Test 2: Add
    int32_t addResult = demoService->Add(100, 200);
    HiLogInfo(CLIENT_LABEL, "Add(100, 200) = %{public}d", addResult);

    // Test 3: GetName
    std::string name = demoService->GetName();
    HiLogInfo(CLIENT_LABEL, "GetName() = %{public}s", name.c_str());

    // Test 4: Multiple calls
    for (int i = 0; i < 5; ++i) {
        int a = 10 * i;
        int b = 20 * i;
        int result = demoService->Add(a, b);
        std::cout << "  Add(" << a << ", " << b << ") = " << result << std::endl;
    }

    HiLogInfo(CLIENT_LABEL, "All tests completed");

    return 0;
}
