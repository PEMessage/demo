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

#include "samgr_mini.h"
#include "ipc_skeleton.h"
#include "hilog/log.h"
#include <iostream>
#include <unistd.h>
#include <csignal>

using namespace OHOS;

// 日志标签
static constexpr HiviewDFX::HiLogLabel SAMGR_LABEL = {
    LOG_CORE, 0xD001510, "SamgrServer"
};

static volatile bool g_running = true;

void SignalHandler(int sig)
{
    HiLogInfo(SAMGR_LABEL, "Received signal %{public}d, shutting down...", sig);
    g_running = false;
    IPCSkeleton::StopWorkThread();
}

/**
 * @brief SamgrServer - 独立的服务管理器进程
 *
 * 作为"DNS"服务器运行：
 * 1. 管理 serviceName -> IRemoteObject 映射
 * 2. 处理 AddService/CheckService/GetService 请求
 * 3. 其他进程通过 IPCSkeleton::GetContextObject() 获取Samgr代理
 */
int main()
{
    // 设置信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    pid_t pid = getpid();
    HiLogInfo(SAMGR_LABEL, "========================================");
    HiLogInfo(SAMGR_LABEL, "SamgrServer starting, PID=%{public}d", pid);
    HiLogInfo(SAMGR_LABEL, "========================================");
    std::cout << "========================================" << std::endl;
    std::cout << "SamgrServer starting, PID=" << pid << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. 获取SamgrMini实例
    sptr<SamgrMini> samgr = SamgrMini::GetInstance();
    if (samgr == nullptr) {
        HiLogError(SAMGR_LABEL, "Failed to get SamgrMini instance");
        return -1;
    }
    std::cout << "SamgrMini instance created" << std::endl;

    // 2. 设置Samgr为ContextObject（让其他进程可以通过GetContextObject获取）
    sptr<IRemoteObject> samgrObject = samgr->AsObject();
    if (!IPCSkeleton::SetContextObject(samgrObject)) {
        HiLogError(SAMGR_LABEL, "Failed to set Samgr as ContextObject");
        return -1;
    }
    HiLogInfo(SAMGR_LABEL, "Samgr registered as ContextObject (handle=0)");
    std::cout << "Samgr registered as ContextObject (handle=0)" << std::endl;
    std::cout << "Other processes can now use IPCSkeleton::GetContextObject() to connect" << std::endl;

    // 3. 进入工作循环，等待其他进程的IPC请求
    HiLogInfo(SAMGR_LABEL, "SamgrServer entering work loop...");
    std::cout << "\nSamgrServer is running, waiting for IPC requests..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    IPCSkeleton::JoinWorkThread();

    HiLogInfo(SAMGR_LABEL, "SamgrServer stopped");
    std::cout << "\n========================================" << std::endl;
    std::cout << "SamgrServer stopped" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
