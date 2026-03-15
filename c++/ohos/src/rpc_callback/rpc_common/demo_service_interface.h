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

#ifndef RPC_COMMON_DEMO_SERVICE_INTERFACE_H
#define RPC_COMMON_DEMO_SERVICE_INTERFACE_H

#include "iremote_broker.h"
#include <string>

namespace OHOS {

/**
 * @brief Demo服务接口定义
 * 用于跨进程RPC测试，支持Callback双向通信
 */
class IDemoService : public IRemoteBroker {
public:
    // 接口命令码定义
    enum {
        PING = 1,               // 测试连接
        ADD = 2,                // 加法运算
        GET_NAME = 3,           // 获取服务名
        REGISTER_CALLBACK = 4,  // 注册回调（新增）
    };

    /**
     * @brief 测试连接
     * @return 0表示成功
     */
    virtual int32_t Ping() = 0;

    /**
     * @brief 加法运算
     * @param a 第一个加数
     * @param b 第二个加数
     * @return 运算结果
     */
    virtual int32_t Add(int32_t a, int32_t b) = 0;

    /**
     * @brief 获取服务名
     * @return 服务名称字符串
     */
    virtual std::string GetName() = 0;

    /**
     * @brief 注册回调
     * @param callback Callback服务的远程对象
     * @return 0表示成功
     */
    virtual int32_t RegisterCallback(const sptr<IRemoteObject>& callback) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.test.DemoService");
};

} // namespace OHOS

#endif // RPC_COMMON_DEMO_SERVICE_INTERFACE_H
