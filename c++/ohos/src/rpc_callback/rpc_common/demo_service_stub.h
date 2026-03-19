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

#ifndef RPC_COMMON_DEMO_SERVICE_STUB_H
#define RPC_COMMON_DEMO_SERVICE_STUB_H

#include "demo_service_interface.h"
#include "iremote_stub.h"
#include "callback_proxy.h"

namespace OHOS {

/**
 * @brief Demo服务 Stub 实现（服务端）
 * 接收远程调用请求，分发到具体的服务方法
 * 支持Callback机制，可以回调Client的方法
 */
class DemoServiceStub : public IRemoteStub<IDemoService> {
public:
    DemoServiceStub();
    ~DemoServiceStub() override = default;

    // IDemoService 接口实现
    int32_t Ping() override;
    int32_t Add(int32_t a, int32_t b) override;
    std::string GetName() override;
    int32_t RegisterCallback(const sptr<IRemoteObject>& callback) override;

    // 处理远程请求
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override;

    // 测试Callback的方法
    void TestCallback();

private:
    int32_t HandlePing(MessageParcel& data, MessageParcel& reply);
    int32_t HandleAdd(MessageParcel& data, MessageParcel& reply);
    int32_t HandleGetName(MessageParcel& data, MessageParcel& reply);
    int32_t HandleRegisterCallback(MessageParcel& data, MessageParcel& reply);

    // 保存Client注册的Callback代理
    sptr<CallbackProxy> callbackProxy_;
    
    // Callback死亡通知接收者
    class CallbackDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit CallbackDeathRecipient(DemoServiceStub* stub) : stub_(stub) {}
        void OnRemoteDied(const wptr<IRemoteObject>& object) override;
    private:
        DemoServiceStub* stub_;
    };
    sptr<CallbackDeathRecipient> callbackDeathRecipient_;
};

} // namespace OHOS

#endif // RPC_COMMON_DEMO_SERVICE_STUB_H
