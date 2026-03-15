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

#ifndef RPC_COMMON_DEMO_SERVICE_PROXY_H
#define RPC_COMMON_DEMO_SERVICE_PROXY_H

#include "demo_service_interface.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief Demo服务 Proxy 实现（客户端）
 * 封装远程调用，将本地调用转换为IPC请求
 * 支持Callback注册
 */
class DemoServiceProxy : public IRemoteProxy<IDemoService> {
public:
    explicit DemoServiceProxy(const sptr<IRemoteObject>& impl);
    ~DemoServiceProxy() override = default;

    // IDemoService 接口实现
    int32_t Ping() override;
    int32_t Add(int32_t a, int32_t b) override;
    std::string GetName() override;
    int32_t RegisterCallback(const sptr<IRemoteObject>& callback) override;

private:
    static inline BrokerDelegator<DemoServiceProxy> delegator_;
};

} // namespace OHOS

#endif // RPC_COMMON_DEMO_SERVICE_PROXY_H
