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

#ifndef CALLBACK_PROXY_H
#define CALLBACK_PROXY_H

#include "callback_interface.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief Callback Proxy实现（在Server中使用）
 * 封装对Client的远程调用
 */
class CallbackProxy : public IRemoteProxy<ICallback> {
public:
    explicit CallbackProxy(const sptr<IRemoteObject>& impl);
    ~CallbackProxy() override = default;

    // ICallback接口实现
    int32_t OnEvent(int32_t eventId, const std::string& data) override;
    std::string GetClientData() override;

private:
    static inline BrokerDelegator<CallbackProxy> delegator_;
};

} // namespace OHOS

#endif // CALLBACK_PROXY_H
