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

#ifndef RPC_GATEWAY_SERVICE_PROXY_H
#define RPC_GATEWAY_SERVICE_PROXY_H

#include "gateway_service_interface.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief GatewayService Proxy
 * Used by Client to register callback with Gateway
 */
class GatewayServiceProxy : public IRemoteProxy<IGatewayService> {
public:
    explicit GatewayServiceProxy(const sptr<IRemoteObject> &impl);
    ~GatewayServiceProxy() override = default;

    int32_t RegisterClientCallback(const sptr<IEventCallback> &callback,
                                    const std::vector<int32_t> &filterEventTypes) override;

private:
    static inline BrokerDelegator<GatewayServiceProxy> delegator_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_PROXY_H
