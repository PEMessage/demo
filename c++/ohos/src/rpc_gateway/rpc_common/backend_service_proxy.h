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

#ifndef RPC_GATEWAY_BACKEND_SERVICE_PROXY_H
#define RPC_GATEWAY_BACKEND_SERVICE_PROXY_H

#include "backend_service_interface.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief BackendService Proxy
 * Used by Gateway to call Server methods
 */
class BackendServiceProxy : public IRemoteProxy<IBackendService> {
public:
    explicit BackendServiceProxy(const sptr<IRemoteObject> &impl);
    ~BackendServiceProxy() override = default;

    int32_t RegisterGatewayCallback(const sptr<IEventCallback> &callback) override;
    int32_t UnregisterGatewayCallback() override;
    int32_t TriggerEvent(int32_t event, const std::vector<int8_t> &data) override;
    std::string GetServiceInfo() override;

private:
    static inline BrokerDelegator<BackendServiceProxy> delegator_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_BACKEND_SERVICE_PROXY_H
