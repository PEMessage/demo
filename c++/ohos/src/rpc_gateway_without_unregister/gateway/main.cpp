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

#include "gateway_service_impl.h"
#include "backend_service_proxy.h"
#include "samgr_mini.h"
#include "samgr_proxy.h"
#include "ipc_skeleton.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include <iostream>
#include <unistd.h>
#include <csignal>

using namespace OHOS;

// Force SamgrProxy to be linked and initialized
static volatile bool _samgr_proxy_init = []() {
    SamgrProxy::GetDescriptor();
    return true;
}();

static constexpr HiviewDFX::HiLogLabel GATEWAY_LABEL = {LOG_CORE, 0xD001550, "GatewayServer"};
static volatile bool g_running = true;
static sptr<GatewayServiceImpl> g_gatewayService;

void SignalHandler(int sig)
{
    HiLogInfo(GATEWAY_LABEL, "Received signal %{public}d, shutting down...", sig);
    g_running = false;
    // No need to explicitly disconnect - when this process dies,
    // the backend will be notified via death recipient
    IPCSkeleton::StopWorkThread();
}

int main()
{
    pid_t pid = getpid();
    HiLogInfo(GATEWAY_LABEL, "========================================");
    HiLogInfo(GATEWAY_LABEL, "GatewayServer starting, PID=%{public}d", pid);
    HiLogInfo(GATEWAY_LABEL, "========================================");

    // 1. Connect to Samgr
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(GATEWAY_LABEL, "Failed to get Samgr remote object");
        return -1;
    }
    HiLogInfo(GATEWAY_LABEL, "[Gateway] Connected to SamgrServer");

    // 2. Lookup Backend Service
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(GATEWAY_LABEL, "Failed to cast Samgr to IServiceRegistry");
        return -1;
    }

    std::u16string backendServiceName = u"ohos.rpc.gateway.BackendService";
    sptr<IRemoteObject> backendRemote = registry->CheckService(backendServiceName);
    if (backendRemote == nullptr) {
        HiLogError(GATEWAY_LABEL, "BackendService not found, please start BackendServer first");
        return -1;
    }
    HiLogInfo(GATEWAY_LABEL, "[Gateway] Found BackendService");

    // 3. Create GatewayServiceImpl instance
    g_gatewayService = new GatewayServiceImpl();

    // 4. Connect to Backend (register Gateway's callback)
    if (!g_gatewayService->ConnectToBackend(backendRemote)) {
        HiLogError(GATEWAY_LABEL, "Failed to connect to Backend");
        return -1;
    }

    // 5. Register Gateway Service with Samgr
    std::u16string gatewayServiceName = u"ohos.rpc.gateway.GatewayService";
    int32_t result = registry->AddService(gatewayServiceName, g_gatewayService->AsObject());
    if (result != ERR_NONE) {
        HiLogError(GATEWAY_LABEL, "AddService failed: %{public}d", result);
        return -1;
    }
    HiLogInfo(GATEWAY_LABEL, "[Gateway] GatewayService registered: %{public}s",
              std::string(gatewayServiceName.begin(), gatewayServiceName.end()).c_str());

    HiLogInfo(GATEWAY_LABEL, "[Gateway] Gateway ready - forwarding events between Backend and Clients");
    HiLogInfo(GATEWAY_LABEL, "[Gateway] Event flow: Backend -> Gateway -> Filter -> Client");

    // 6. Enter work loop
    IPCSkeleton::JoinWorkThread();

    HiLogInfo(GATEWAY_LABEL, "[Gateway] GatewayServer stopped");
    return 0;
}
