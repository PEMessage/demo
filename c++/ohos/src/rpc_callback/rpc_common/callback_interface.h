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

#ifndef CALLBACK_INTERFACE_H
#define CALLBACK_INTERFACE_H

#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include <string>

namespace OHOS {

/**
 * @brief Callback接口定义
 * 用于Server回调Client的双向RPC测试
 */
class ICallback : public IRemoteBroker {
public:
    enum {
        ON_EVENT = 1,
        GET_CLIENT_DATA = 2,
    };

    /**
     * @brief Server调用：通知Client某个事件
     * @param eventId 事件ID
     * @param data 事件数据
     * @return 0表示成功
     */
    virtual int32_t OnEvent(int32_t eventId, const std::string& data) = 0;

    /**
     * @brief Server调用：获取Client的数据
     * @return Client返回的数据
     */
    virtual std::string GetClientData() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.test.Callback");
};

} // namespace OHOS

#endif // CALLBACK_INTERFACE_H
