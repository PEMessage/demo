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

#ifndef RPC_GATEWAY_BACKEND_SERVICE_IMPL_H
#define RPC_GATEWAY_BACKEND_SERVICE_IMPL_H

#include "backend_service_stub.h"
#include "event_callback_proxy.h"

namespace OHOS {

/**
 * @brief Backend Service Implementation
 * Concrete implementation of the BackendService business logic.
 * This class extends BackendServiceStub to provide actual functionality.
 */
class BackendServiceImpl : public BackendServiceStub {
public:
    BackendServiceImpl();
    ~BackendServiceImpl() override = default;

    // IBackendService interface implementation
    int32_t RegisterGatewayCallback(const sptr<IEventCallback> &callback) override;
    int32_t TriggerEvent(int32_t event, const std::vector<int8_t> &data) override;
    std::string GetServiceInfo() override;

    // Check if callback is registered
    bool HasCallback() const { return gatewayCallback_ != nullptr; }

private:
    sptr<EventCallbackProxy> gatewayCallback_;
};

} // namespace OHOS

#endif // RPC_GATEWAY_BACKEND_SERVICE_IMPL_H
