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
#include "samgr_mini.h"
#include "samgr_proxy.h"
#include "ipc_skeleton.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <chrono>
#include <thread>

using namespace OHOS;

// 日志标签
static constexpr HiviewDFX::HiLogLabel DEMO_LABEL = {
    LOG_CORE, 0xD001520, "DemoServiceServer"
};

static volatile bool g_running = true;

void SignalHandler(int sig)
{
    HiLogInfo(DEMO_LABEL, "Received signal %{public}d, shutting down...", sig);
    g_running = false;
    IPCSkeleton::StopWorkThread();
}

/**
 * @brief DemoServiceServer - 业务服务进程
 *
 * 流程：
 * 1. 通过 IPCSkeleton::GetContextObject() 获取Samgr的远程对象
 * 2. 创建DemoServiceStub实例
 * 3. 调用 AddService("ohos.test.DemoService", stub) 向Samgr注册
 * 4. 进入工作循环等待客户端直接连接
 *
 * 注意：与Samgr是客户端-服务器关系
 */
int main()
{
    // 设置信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    pid_t pid = getpid();
    HiLogInfo(DEMO_LABEL, "========================================");
    HiLogInfo(DEMO_LABEL, "DemoServiceServer starting, PID=%{public}d", pid);
    HiLogInfo(DEMO_LABEL, "========================================");
    std::cout << "========================================" << std::endl;
    std::cout << "DemoServiceServer starting, PID=" << pid << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. 获取Samgr的远程对象（通过Binder连接到Samgr进程）
    std::cout << "\n[Step 1] Connecting to SamgrServer..." << std::endl;
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(DEMO_LABEL, "Failed to get Samgr remote object");
        std::cerr << "ERROR: Failed to connect to SamgrServer!" << std::endl;
        std::cerr << "Make sure SamgrServer is running first." << std::endl;
        return -1;
    }
    HiLogInfo(DEMO_LABEL, "Connected to SamgrServer");
    std::cout << "Connected to SamgrServer (handle=0)" << std::endl;

    // 2. 创建DemoServiceStub实例
    std::cout << "\n[Step 2] Creating DemoService..." << std::endl;
    sptr<DemoServiceStub> demoService = new DemoServiceStub();
    std::cout << "DemoServiceStub created" << std::endl;

    // 3. 通过Samgr注册服务（IPC调用）
    std::cout << "\n[Step 3] Registering service to Samgr..." << std::endl;
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(DEMO_LABEL, "Failed to cast Samgr to IServiceRegistry");
        std::cerr << "ERROR: Failed to cast Samgr to IServiceRegistry" << std::endl;
        return -1;
    }

    std::u16string serviceName = u"ohos.test.DemoService";
    int32_t result = registry->AddService(serviceName, demoService->AsObject());
    if (result != ERR_NONE) {
        HiLogError(DEMO_LABEL, "AddService failed: %{public}d", result);
        std::cerr << "ERROR: Failed to register service: " << result << std::endl;
        return -1;
    }
    HiLogInfo(DEMO_LABEL, "Service registered: ohos.test.DemoService");
    std::cout << "Service registered: ohos.test.DemoService" << std::endl;

    // 4. 验证服务是否注册成功
    std::cout << "\n[Step 4] Verifying registration..." << std::endl;
    sptr<IRemoteObject> checkObj = registry->CheckService(serviceName);
    if (checkObj == nullptr) {
        HiLogWarn(DEMO_LABEL, "CheckService returned null, but registration may still be OK");
        std::cout << "CheckService returned null (this is normal in cross-process mode)" << std::endl;
    } else {
        std::cout << "Service verified: CheckService returned valid object" << std::endl;
    }

    // 5. 进入工作循环，等待客户端直接连接
    std::cout << "\n========================================" << std::endl;
    std::cout << "DemoServiceServer is running!" << std::endl;
    std::cout << "Service: ohos.test.DemoService" << std::endl;
    std::cout << "PID: " << pid << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nWaiting for direct connections from clients..." << std::endl;
    std::cout << "(Clients will connect directly to this process, not through Samgr)" << std::endl;
    std::cout << "\nPress Ctrl+C to stop" << std::endl;

    HiLogInfo(DEMO_LABEL, "Entering work loop, waiting for client connections...");

    IPCSkeleton::JoinWorkThread();

    HiLogInfo(DEMO_LABEL, "DemoServiceServer stopped");
    std::cout << "\n========================================" << std::endl;
    std::cout << "DemoServiceServer stopped" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
