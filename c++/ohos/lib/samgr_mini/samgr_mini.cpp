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

#include "samgr_mini.h"
#include "hilog/log.h"
#include "ipc_skeleton.h"
#include <chrono>
#include <thread>

namespace OHOS {

// 日志标签定义
static constexpr OHOS::HiviewDFX::HiLogLabel SAMGR_LABEL = {
    LOG_CORE, 0xD001510, "SamgrMini"
};

// 单例实例初始化
std::mutex SamgrMini::instanceMutex_;
sptr<SamgrMini> SamgrMini::instance_;

// 远程代理静态成员定义
std::mutex SamgrMini::remoteMutex_;
sptr<IRemoteObject> SamgrMini::remoteProxy_;

// ServiceDeathRecipient 实现
ServiceDeathRecipient::ServiceDeathRecipient(DeathCallback callback)
    : callback_(std::move(callback)) {}

void ServiceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    if (callback_) {
        callback_(object);
    }
}

// SamgrMini 实现
SamgrMini::SamgrMini() : IRemoteStub<IServiceRegistry>(false) {}

sptr<SamgrMini> SamgrMini::GetInstance()
{
    // 检查是否有远程代理设置
    {
        std::lock_guard<std::mutex> lock(remoteMutex_);
        if (remoteProxy_ != nullptr) {
            // 客户端模式：返回一个使用远程代理的SamgrMini
            // 这里返回this，但所有操作会通过remoteProxy_转发
            // 为简化实现，我们直接返回本地实例，但接口调用会通过remoteProxy_
            if (instance_ == nullptr) {
                std::lock_guard<std::mutex> instLock(instanceMutex_);
                if (instance_ == nullptr) {
                    instance_ = new SamgrMini();
                }
            }
            return instance_;
        }
    }

    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (instance_ == nullptr) {
            instance_ = new SamgrMini();
        }
    }
    return instance_;
}

bool SamgrMini::IsRemoteMode()
{
    std::lock_guard<std::mutex> lock(remoteMutex_);
    return remoteProxy_ != nullptr;
}

void SamgrMini::SetRemoteProxy(const sptr<IRemoteObject>& remote)
{
    std::lock_guard<std::mutex> lock(remoteMutex_);
    remoteProxy_ = remote;
    HiLogInfo(SAMGR_LABEL, "SetRemoteProxy: remote mode enabled");
}

sptr<IRemoteObject> SamgrMini::AsObject()
{
    return this;
}

// 客户端模式辅助函数
static sptr<IRemoteObject> GetRemoteProxy()
{
    std::lock_guard<std::mutex> lock(SamgrMini::remoteMutex_);
    return SamgrMini::remoteProxy_;
}

// IServiceRegistry 接口实现
int32_t SamgrMini::AddService(const std::u16string& name,
                               const sptr<IRemoteObject>& service)
{
    // 检查是否是客户端模式
    sptr<IRemoteObject> remote = GetRemoteProxy();
    if (remote != nullptr && remote != this->AsObject()) {
        // 客户端模式：转发到远程SamgrMini
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        data.WriteInterfaceToken(IServiceRegistry::GetDescriptor());
        data.WriteString16(name);
        data.WriteRemoteObject(service);

        int32_t result = remote->SendRequest(1, data, reply, option);  // 1 = AddService
        if (result == ERR_NONE) {
            return reply.ReadInt32();
        }
        return result;
    }

    // 服务端模式：本地存储
    if (service == nullptr) {
        HiLogError(SAMGR_LABEL, "AddService failed: service is null");
        return ERR_NULL_OBJECT;
    }

    if (name.empty()) {
        HiLogError(SAMGR_LABEL, "AddService failed: name is empty");
        return ERR_INVALID_DATA;
    }

    // 注册服务死亡通知
    sptr<ServiceDeathRecipient> deathRecipient = new ServiceDeathRecipient(
        [this](const wptr<IRemoteObject>& obj) {
            this->OnServiceDied(obj);
        });

    if (!service->AddDeathRecipient(deathRecipient)) {
        HiLogWarn(SAMGR_LABEL, "AddDeathRecipient failed for service: %{public}s",
                  std::string(name.begin(), name.end()).c_str());
    }

    servicesMap_.EnsureInsert(name, service);
    HiLogInfo(SAMGR_LABEL, "AddService success: %{public}s",
              std::string(name.begin(), name.end()).c_str());
    return ERR_NONE;
}

sptr<IRemoteObject> SamgrMini::GetService(const std::u16string& name)
{
    // 检查是否是客户端模式
    sptr<IRemoteObject> remote = GetRemoteProxy();
    if (remote != nullptr && remote != this->AsObject()) {
        // 客户端模式：转发到远程SamgrMini
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        data.WriteInterfaceToken(IServiceRegistry::GetDescriptor());
        data.WriteString16(name);

        int32_t result = remote->SendRequest(2, data, reply, option);  // 2 = GetService
        if (result == ERR_NONE) {
            return reply.ReadRemoteObject();
        }
        return nullptr;
    }

    // 服务端模式：本地查询
    // 先检查服务是否存在
    sptr<IRemoteObject> service;
    const int maxRetry = 50;  // 最多等待5秒
    const int sleepMs = 100;  // 每次等待100ms

    for (int i = 0; i < maxRetry; ++i) {
        if (servicesMap_.Find(name, service) && service != nullptr) {
            HiLogInfo(SAMGR_LABEL, "GetService success: %{public}s (waited %{public}dms)",
                      std::string(name.begin(), name.end()).c_str(), i * sleepMs);
            return service;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    HiLogWarn(SAMGR_LABEL, "GetService timeout: %{public}s",
              std::string(name.begin(), name.end()).c_str());
    return nullptr;
}

sptr<IRemoteObject> SamgrMini::CheckService(const std::u16string& name)
{
    // 检查是否是客户端模式
    sptr<IRemoteObject> remote = GetRemoteProxy();
    if (remote != nullptr && remote != this->AsObject()) {
        // 客户端模式：转发到远程SamgrMini
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        data.WriteInterfaceToken(IServiceRegistry::GetDescriptor());
        data.WriteString16(name);

        int32_t result = remote->SendRequest(3, data, reply, option);  // 3 = CheckService
        if (result == ERR_NONE) {
            return reply.ReadRemoteObject();
        }
        return nullptr;
    }

    // 服务端模式：本地查询
    sptr<IRemoteObject> service;
    if (servicesMap_.Find(name, service)) {
        HiLogInfo(SAMGR_LABEL, "CheckService found: %{public}s",
                  std::string(name.begin(), name.end()).c_str());
        return service;
    }
    HiLogInfo(SAMGR_LABEL, "CheckService not found: %{public}s",
              std::string(name.begin(), name.end()).c_str());
    return nullptr;
}

// ISystemAbilityManager 接口实现
int32_t SamgrMini::AddSystemAbility(int32_t systemAbilityId,
                                     const sptr<IRemoteObject>& ability)
{
    if (ability == nullptr) {
        HiLogError(SAMGR_LABEL, "AddSystemAbility failed: ability is null");
        return ERR_NULL_OBJECT;
    }

    // 注册服务死亡通知
    sptr<ServiceDeathRecipient> deathRecipient = new ServiceDeathRecipient(
        [this](const wptr<IRemoteObject>& obj) {
            this->OnServiceDied(obj);
        });

    if (!ability->AddDeathRecipient(deathRecipient)) {
        HiLogWarn(SAMGR_LABEL, "AddDeathRecipient failed for SA: %{public}d", systemAbilityId);
    }

    saMap_.EnsureInsert(systemAbilityId, ability);
    HiLogInfo(SAMGR_LABEL, "AddSystemAbility success: SAID=%{public}d", systemAbilityId);
    return ERR_NONE;
}

sptr<IRemoteObject> SamgrMini::GetSystemAbility(int32_t systemAbilityId)
{
    // 先检查能力是否存在
    sptr<IRemoteObject> ability;
    const int maxRetry = 50;
    const int sleepMs = 100;

    for (int i = 0; i < maxRetry; ++i) {
        if (saMap_.Find(systemAbilityId, ability) && ability != nullptr) {
            HiLogInfo(SAMGR_LABEL, "GetSystemAbility success: SAID=%{public}d (waited %{public}dms)",
                      systemAbilityId, i * sleepMs);
            return ability;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    HiLogWarn(SAMGR_LABEL, "GetSystemAbility timeout: SAID=%{public}d", systemAbilityId);
    return nullptr;
}

sptr<IRemoteObject> SamgrMini::CheckSystemAbility(int32_t systemAbilityId)
{
    sptr<IRemoteObject> ability;
    if (saMap_.Find(systemAbilityId, ability)) {
        HiLogInfo(SAMGR_LABEL, "CheckSystemAbility found: SAID=%{public}d", systemAbilityId);
        return ability;
    }
    HiLogInfo(SAMGR_LABEL, "CheckSystemAbility not found: SAID=%{public}d", systemAbilityId);
    return nullptr;
}

// 服务死亡通知处理
bool SamgrMini::AddServiceDeathRecipient(const std::u16string& name,
                                           const sptr<IRemoteObject::DeathRecipient>& recipient)
{
    std::lock_guard<std::mutex> lock(deathRecipientMutex_);
    serviceDeathRecipients_[name].push_back(recipient);
    HiLogInfo(SAMGR_LABEL, "AddServiceDeathRecipient: %{public}s",
              std::string(name.begin(), name.end()).c_str());
    return true;
}

bool SamgrMini::RemoveServiceDeathRecipient(const std::u16string& name,
                                              const sptr<IRemoteObject::DeathRecipient>& recipient)
{
    std::lock_guard<std::mutex> lock(deathRecipientMutex_);
    auto it = serviceDeathRecipients_.find(name);
    if (it != serviceDeathRecipients_.end()) {
        auto& recipients = it->second;
        recipients.erase(
            std::remove(recipients.begin(), recipients.end(), recipient),
            recipients.end());
        HiLogInfo(SAMGR_LABEL, "RemoveServiceDeathRecipient: %{public}s",
                  std::string(name.begin(), name.end()).c_str());
        return true;
    }
    return false;
}

void SamgrMini::OnServiceDied(const wptr<IRemoteObject>& object)
{
    sptr<IRemoteObject> diedObj = object.promote();
    if (diedObj == nullptr) {
        return;
    }

    HiLogWarn(SAMGR_LABEL, "Service died, cleaning up...");
    RemoveDeadService(diedObj);
}

void SamgrMini::RemoveDeadService(const sptr<IRemoteObject>& object)
{
    // 从服务映射中移除死亡的服务
    servicesMap_.Iterate([this, &object](const std::u16string& name, sptr<IRemoteObject>& service) {
        if (service == object) {
            HiLogWarn(SAMGR_LABEL, "Removing dead service: %{public}s",
                      std::string(name.begin(), name.end()).c_str());
            service.clear();
            servicesMap_.Erase(name);

            // 通知注册的死亡通知接收者
            std::lock_guard<std::mutex> lock(deathRecipientMutex_);
            auto it = serviceDeathRecipients_.find(name);
            if (it != serviceDeathRecipients_.end()) {
                for (auto& recipient : it->second) {
                    if (recipient != nullptr) {
                        recipient->OnRemoteDied(object);
                    }
                }
                serviceDeathRecipients_.erase(it);
            }
        }
    });

    // 从SA映射中移除死亡的系统能力
    saMap_.Iterate([this, &object](const int32_t saId, sptr<IRemoteObject>& ability) {
        if (ability == object) {
            HiLogWarn(SAMGR_LABEL, "Removing dead system ability: SAID=%{public}d", saId);
            ability.clear();
            saMap_.Erase(saId);

            // 通知注册的死亡通知接收者
            std::lock_guard<std::mutex> lock(deathRecipientMutex_);
            auto it = saDeathRecipients_.find(saId);
            if (it != saDeathRecipients_.end()) {
                for (auto& recipient : it->second) {
                    if (recipient != nullptr) {
                        recipient->OnRemoteDied(object);
                    }
                }
                saDeathRecipients_.erase(it);
            }
        }
    });
}

// 远程请求处理（IRemoteStub）
int SamgrMini::OnRemoteRequest(uint32_t code, MessageParcel& data,
                                MessageParcel& reply, MessageOption& option)
{
    HiLogInfo(SAMGR_LABEL, "OnRemoteRequest: code=%{public}u", code);

    // 标准接口调用检查
    std::u16string descriptor = IServiceRegistry::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HiLogError(SAMGR_LABEL, "Interface token mismatch");
        return ERR_INVALID_DATA;
    }

    switch (code) {
        case 1: {  // AddService
            std::u16string name = data.ReadString16();
            sptr<IRemoteObject> service = data.ReadRemoteObject();
            int32_t result = AddService(name, service);
            reply.WriteInt32(result);
            return ERR_NONE;
        }
        case 2: {  // GetService
            std::u16string name = data.ReadString16();
            sptr<IRemoteObject> service = GetService(name);
            reply.WriteRemoteObject(service);
            return ERR_NONE;
        }
        case 3: {  // CheckService
            std::u16string name = data.ReadString16();
            sptr<IRemoteObject> service = CheckService(name);
            reply.WriteRemoteObject(service);
            return ERR_NONE;
        }
        default:
            return IRemoteStub<IServiceRegistry>::OnRemoteRequest(code, data, reply, option);
    }
}

void SamgrMini::ListServices(std::vector<std::u16string>& names)
{
    names.clear();
    servicesMap_.Iterate([&names](const std::u16string& name, sptr<IRemoteObject>&) {
        names.push_back(name);
    });
}

void SamgrMini::ListSystemAbilities(std::vector<int32_t>& saIds)
{
    saIds.clear();
    saMap_.Iterate([&saIds](const int32_t saId, sptr<IRemoteObject>&) {
        saIds.push_back(saId);
    });
}

} // namespace OHOS
