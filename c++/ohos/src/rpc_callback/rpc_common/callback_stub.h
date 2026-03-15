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

#ifndef CALLBACK_STUB_H
#define CALLBACK_STUB_H

#include "callback_interface.h"
#include "iremote_stub.h"

namespace OHOS {

/**
 * @brief Callback Stub实现（在Client中运行）
 * 接收Server的远程调用请求
 */
class CallbackStub : public IRemoteStub<ICallback> {
public:
    CallbackStub();
    ~CallbackStub() override = default;

    // ICallback接口实现（由Client的具体实现类覆盖）
    int32_t OnEvent(int32_t eventId, const std::string& data) override;
    std::string GetClientData() override;

    // 处理远程请求
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override;

private:
    int32_t HandleOnEvent(MessageParcel& data, MessageParcel& reply);
    int32_t HandleGetClientData(MessageParcel& data, MessageParcel& reply);
};

} // namespace OHOS

#endif // CALLBACK_STUB_H
