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

#include "backend_service_stub.h"
#include "samgr_mini.h"
#include "samgr_proxy.h"
#include "ipc_skeleton.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <chrono>

using namespace OHOS;

// // Force SamgrProxy to be linked and initialized
// static volatile bool _samgr_proxy_init = []() {
//     // Touch SamgrProxy to ensure its delegator is registered
//     SamgrProxy::GetDescriptor();
//     return true;
// }();

static constexpr HiviewDFX::HiLogLabel SERVER_LABEL = {LOG_CORE, 0xD001540, "BackendServer"};
static volatile bool g_running = true;
static sptr<BackendServiceStub> g_backendService;

void SignalHandler(int sig)
{
    HiLogInfo(SERVER_LABEL, "Received signal %{public}d, shutting down...", sig);
    g_running = false;
    IPCSkeleton::StopWorkThread();
}

int main()
{
    pid_t pid = getpid();
    HiLogInfo(SERVER_LABEL, "========================================");
    HiLogInfo(SERVER_LABEL, "BackendServer starting, PID=%{public}d", pid);
    HiLogInfo(SERVER_LABEL, "========================================");

    // 1. Connect to Samgr
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(SERVER_LABEL, "Failed to get Samgr remote object");
        return -1;
    }
    HiLogInfo(SERVER_LABEL, "[Server] Connected to SamgrServer");

    // 2. Create BackendServiceStub instance
    g_backendService = new BackendServiceStub();

    // 3. Register service with Samgr
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(SERVER_LABEL, "Failed to cast Samgr to IServiceRegistry");
        return -1;
    }

    std::u16string serviceName = u"ohos.rpc.gateway.BackendService";
    int32_t result = registry->AddService(serviceName, g_backendService->AsObject());
    if (result != ERR_NONE) {
        HiLogError(SERVER_LABEL, "AddService failed: %{public}d", result);
        return -1;
    }
    HiLogInfo(SERVER_LABEL, "[Server] BackendService registered: %{public}s",
              std::string(serviceName.begin(), serviceName.end()).c_str());

    // 4. Periodically trigger events for testing
    std::thread eventThread([]() {
        int eventId = 1000;
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (g_backendService != nullptr && g_backendService->HasCallback()) {
                // Create event data
                std::string dataStr = "Event " + std::to_string(eventId) + " from BackendServer";
                std::vector<int8_t> data(dataStr.begin(), dataStr.end());
                
                HiLogInfo(SERVER_LABEL, "[Server] Auto-triggering event %{public}d", eventId);
                g_backendService->TriggerEvent(eventId, data);
                
                eventId = eventId >= 1005 ? 1000 : eventId + 1;
            }
        }
    });
    eventThread.detach();

    HiLogInfo(SERVER_LABEL, "[Server] Waiting for Gateway to connect...");
    HiLogInfo(SERVER_LABEL, "[Server] Events will be triggered automatically every 5 seconds");

    // 5. Enter work loop
    IPCSkeleton::JoinWorkThread();

    HiLogInfo(SERVER_LABEL, "[Server] BackendServer stopped");
    return 0;
}
