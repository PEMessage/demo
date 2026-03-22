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

#ifndef RPC_GATEWAY_SERVICE_STUB_H
#define RPC_GATEWAY_SERVICE_STUB_H

#include "gateway_service_interface.h"
#include "iremote_stub.h"

namespace OHOS {

/**
 * @brief Gateway Service Stub
 * RPC layer that handles serialization/deserialization.
 * Business logic should be implemented in a derived class.
 */
class GatewayServiceStub : public IRemoteStub<IGatewayService> {
public:
    GatewayServiceStub() = default;
    ~GatewayServiceStub() override = default;

    // IRemoteStub interface - handles RPC dispatch
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                         MessageParcel &reply, MessageOption &option) override;

protected:
    // Handle methods - only do deserialization and call virtual interface methods
    int32_t HandleRegisterClientCallback(MessageParcel &data, MessageParcel &reply);
    int32_t HandleUnregisterClientCallback(MessageParcel &data, MessageParcel &reply);
};

} // namespace OHOS

#endif // RPC_GATEWAY_SERVICE_STUB_H
