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

#ifndef SAMGR_MINI_SAMGR_PROXY_H
#define SAMGR_MINI_SAMGR_PROXY_H

#include "iservice_registry.h"
#include "iremote_proxy.h"

namespace OHOS {

/**
 * @brief SamgrProxy - Samgr的客户端代理
 *
 * 用于客户端通过iface_cast<IServiceRegistry>获取Samgr代理
 * 将本地调用转换为IPC请求发送到SamgrServer
 */
class SamgrProxy : public IRemoteProxy<IServiceRegistry> {
public:
    explicit SamgrProxy(const sptr<IRemoteObject>& impl);
    ~SamgrProxy() override = default;

    // IServiceRegistry 接口实现
    int32_t AddService(const std::u16string& name,
                       const sptr<IRemoteObject>& service) override;
    sptr<IRemoteObject> GetService(const std::u16string& name) override;
    sptr<IRemoteObject> CheckService(const std::u16string& name) override;

private:
    static inline BrokerDelegator<SamgrProxy> delegator_;

    // 远程请求命令码
    enum SamgrRequestCode {
        ADD_SERVICE = 1,
        GET_SERVICE = 2,
        CHECK_SERVICE = 3,
    };
};

} // namespace OHOS

#endif // SAMGR_MINI_SAMGR_PROXY_H
