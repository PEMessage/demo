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

#include "backend_service_impl.h"
#include "hilog/log.h"
#include <unistd.h>

namespace OHOS {

static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001534, "BackendServiceImpl"};

BackendServiceImpl::BackendServiceImpl()
{
    HiLogInfo(LABEL, "BackendServiceImpl created");
}

int32_t BackendServiceImpl::RegisterGatewayCallback(const sptr<IEventCallback> &callback)
{
    if (callback == nullptr) {
        HiLogError(LABEL, "Callback is null");
        return ERR_INVALID_DATA;
    }

    gatewayCallback_ = new EventCallbackProxy(callback->AsObject());
    HiLogInfo(LABEL, "Gateway callback registered");
    return ERR_NONE;
}

int32_t BackendServiceImpl::TriggerEvent(int32_t event, const std::vector<int8_t> &data)
{
    if (gatewayCallback_ == nullptr) {
        HiLogWarn(LABEL, "No gateway callback registered, cannot trigger event");
        return ERR_INVALID_DATA;
    }

    HiLogInfo(LABEL, "Triggering event %{public}d to gateway", event);
    ErrCode result = gatewayCallback_->OnEvent(event, data);
    HiLogInfo(LABEL, "Event callback returned: %{public}d", result);
    return result;
}

std::string BackendServiceImpl::GetServiceInfo()
{
    pid_t pid = getpid();
    return "BackendService running on PID=" + std::to_string(pid);
}

} // namespace OHOS
