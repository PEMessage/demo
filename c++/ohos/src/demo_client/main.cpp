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
    std::cout << "========================================" << std::endl;
    std::cout << "DemoClient starting, PID=" << pid << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. 连接到Samgr
    std::cout << "\n[Step 1] Connecting to SamgrServer..." << std::endl;
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to get Samgr remote object");
        std::cerr << "ERROR: Failed to connect to SamgrServer!" << std::endl;
        std::cerr << "Make sure SamgrServer is running first." << std::endl;
        return -1;
    }
    std::cout << "Connected to SamgrServer (handle=0)" << std::endl;

    // 2. 查询DemoService
    std::cout << "\n[Step 2] Querying DemoService from Samgr..." << std::endl;
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to cast Samgr to IServiceRegistry");
        std::cerr << "ERROR: Failed to cast Samgr to IServiceRegistry" << std::endl;
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
            std::cout << "Found DemoService after " << (i * sleepMs) << "ms" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    if (serviceRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "DemoService not found");
        std::cerr << "ERROR: DemoService not found!" << std::endl;
        std::cerr << "Make sure DemoServiceServer is running and registered." << std::endl;
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "Found DemoService");
    std::cout << "Found DemoService: ohos.test.DemoService" << std::endl;

    // 3. 直接连接到DemoService（不再经过Samgr）
    std::cout << "\n[Step 3] Creating service proxy for direct connection..." << std::endl;
    sptr<IDemoService> demoService = iface_cast<IDemoService>(serviceRemote);
    if (demoService == nullptr) {
        // 如果没有注册代理创建器，直接使用DemoServiceProxy
        demoService = new DemoServiceProxy(serviceRemote);
    }
    std::cout << "DemoServiceProxy created (direct connection)" << std::endl;

    // 4. 直接调用DemoService的方法
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing RPC calls (direct to DemoServiceServer)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Test 1: Ping
    std::cout << "\n[Test 1] Calling Ping()..." << std::endl;
    int32_t pingResult = demoService->Ping();
    if (pingResult == ERR_NONE) {
        std::cout << "  Ping() succeeded!" << std::endl;
        HiLogInfo(CLIENT_LABEL, "Ping() succeeded");
    } else {
        std::cout << "  Ping() failed: " << pingResult << std::endl;
        HiLogError(CLIENT_LABEL, "Ping() failed: %{public}d", pingResult);
    }

    // Test 2: Add
    std::cout << "\n[Test 2] Calling Add(100, 200)..." << std::endl;
    int32_t addResult = demoService->Add(100, 200);
    std::cout << "  Add(100, 200) = " << addResult << std::endl;
    HiLogInfo(CLIENT_LABEL, "Add(100, 200) = %{public}d", addResult);

    // Test 3: GetName
    std::cout << "\n[Test 3] Calling GetName()..." << std::endl;
    std::string name = demoService->GetName();
    std::cout << "  GetName() = \"" << name << "\"" << std::endl;
    HiLogInfo(CLIENT_LABEL, "GetName() = %{public}s", name.c_str());

    // Test 4: Multiple calls
    std::cout << "\n[Test 4] Multiple Add calls..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        int a = 10 * i;
        int b = 20 * i;
        int result = demoService->Add(a, b);
        std::cout << "  Add(" << a << ", " << b << ") = " << result << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "All tests completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;
    HiLogInfo(CLIENT_LABEL, "All tests completed");

    return 0;
}
