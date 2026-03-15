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

#ifndef SAMGR_MINI_ISERVICE_REGISTRY_H
#define SAMGR_MINI_ISERVICE_REGISTRY_H

#include "iremote_broker.h"

namespace OHOS {

/**
 * @brief IServiceRegistry - 类似Android的ServiceManager
 * 提供按字符串名称注册和查询服务的接口
 */
class IServiceRegistry : public IRemoteBroker {
public:
    /**
     * @brief 注册服务
     * @param name 服务名称
     * @param service 服务对象
     * @return 0表示成功，其他表示失败
     */
    virtual int32_t AddService(const std::u16string& name,
                               const sptr<IRemoteObject>& service) = 0;

    /**
     * @brief 获取服务（阻塞等待服务可用）
     * @param name 服务名称
     * @return 服务对象，失败返回nullptr
     */
    virtual sptr<IRemoteObject> GetService(const std::u16string& name) = 0;

    /**
     * @brief 检查服务是否存在（非阻塞）
     * @param name 服务名称
     * @return 服务对象，不存在返回nullptr
     */
    virtual sptr<IRemoteObject> CheckService(const std::u16string& name) = 0;

    /**
     * @brief 获取接口描述符
     */
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.samgr.IServiceRegistry");
};

} // namespace OHOS

#endif // SAMGR_MINI_ISERVICE_REGISTRY_H
