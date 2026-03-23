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

#include "gateway_service_proxy.h"
#include "event_callback_stub.h"
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

static constexpr HiviewDFX::HiLogLabel CLIENT_LABEL = {LOG_CORE, 0xD001560, "Client"};
static volatile bool g_running = true;

// Client's callback implementation
class ClientEventCallback : public EventCallbackStub {
private:
    int id_;
public:
    ClientEventCallback(int id): id_(id) {};
    ErrCode OnEvent(int32_t event, const std::vector<int8_t> &reqData) override {
        // Convert data to string
        std::string dataStr(reqData.begin(), reqData.end());
        HiLogInfo(CLIENT_LABEL, "[Client] ID: %{public}d OnEvent received: event=%{public}d, data=%{public}s",
                  id_, event, dataStr.c_str());
        return ERR_NONE;
    }
};

void SignalHandler(int sig)
{
    HiLogInfo(CLIENT_LABEL, "Received signal %{public}d, shutting down...", sig);
    g_running = false;
    IPCSkeleton::StopWorkThread();
}

int main()
{
    pid_t pid = getpid();
    HiLogInfo(CLIENT_LABEL, "========================================");
    HiLogInfo(CLIENT_LABEL, "Client starting, PID=%{public}d", pid);
    HiLogInfo(CLIENT_LABEL, "========================================");

    // 1. Connect to Samgr
    sptr<IRemoteObject> samgrRemote = IPCSkeleton::GetContextObject();
    if (samgrRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to get Samgr remote object");
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Connected to SamgrServer");

    // 2. Lookup Gateway Service
    sptr<IServiceRegistry> registry = iface_cast<IServiceRegistry>(samgrRemote);
    if (registry == nullptr) {
        HiLogError(CLIENT_LABEL, "Failed to cast Samgr to IServiceRegistry");
        return -1;
    }

    std::u16string gatewayServiceName = u"ohos.rpc.gateway.GatewayService";
    sptr<IRemoteObject> gatewayRemote = registry->CheckService(gatewayServiceName);
    if (gatewayRemote == nullptr) {
        HiLogError(CLIENT_LABEL, "GatewayService not found, please start GatewayServer first");
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Found GatewayService");

    // 3. Create Gateway proxy
    sptr<GatewayServiceProxy> gatewayProxy = new GatewayServiceProxy(gatewayRemote);

    // 4. Create client's callback
    sptr<ClientEventCallback> clientCallback0 = new ClientEventCallback(0);

    // 5. Register callback with Gateway (with optional event filter)
    // Example: only receive events 1000, 1002, 1004
    std::vector<int32_t> filterTypes = {1000, 1002, 1004};
    
    HiLogInfo(CLIENT_LABEL, "[Client] Registering callback with Gateway (filtered events: 1000, 1002, 1004)");
    int32_t result = gatewayProxy->RegisterClientCallback(clientCallback0, filterTypes);
    if (result != ERR_NONE) {
        HiLogError(CLIENT_LABEL, "Failed to register callback: %{public}d", result);
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Callback registered successfully");
    HiLogInfo(CLIENT_LABEL, "[Client] Waiting for events from Server via Gateway...");
    HiLogInfo(CLIENT_LABEL, "[Client] Only events 1000, 1002, 1004 will be received (filtered)");

    sleep(5);

    HiLogInfo(CLIENT_LABEL, "[Client] Overwrite it with new callback");
    sptr<ClientEventCallback> clientCallback1 = new ClientEventCallback(1);
    result = gatewayProxy->RegisterClientCallback(clientCallback1, filterTypes);
    if (result != ERR_NONE) {
        HiLogError(CLIENT_LABEL, "Failed to register callback: %{public}d", result);
        return -1;
    }
    HiLogInfo(CLIENT_LABEL, "[Client] Overwrite Callback registered successfully");

    // 6. Keep running to receive callbacks
    while (g_running) {
        sleep(1);
    }

    // 7. No need to unregister - when this process dies, the callback will be
    // automatically unregistered via death recipient
    HiLogInfo(CLIENT_LABEL, "[Client] Client stopped (callback will auto-unregister)");
    return 0;
}
