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

#ifndef SAMGR_MINI_SAMGR_MINI_H
#define SAMGR_MINI_SAMGR_MINI_H

#include "iservice_registry.h"
#include "isystem_ability_manager.h"
#include "iremote_stub.h"
#include "safe_map.h"
#include "singleton.h"
#include <map>
#include <mutex>

namespace OHOS {

/**
 * @brief DeathRecipient 包装类
 * 用于监控服务死亡并自动清理
 */
class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    using DeathCallback = std::function<void(const wptr<IRemoteObject>&)>;

    explicit ServiceDeathRecipient(DeathCallback callback);
    ~ServiceDeathRecipient() override = default;

    void OnRemoteDied(const wptr<IRemoteObject>& object) override;

private:
    DeathCallback callback_;
};

/**
 * @brief SamgrMini - 简化版系统服务管理器
 *
 * 功能：
 * 1. AddService/GetService - 按字符串名称注册/查询服务
 * 2. AddSystemAbility/GetSystemAbility - 按整数SAID注册/查询系统能力
 * 3. DeathRecipient - 服务死亡通知与自动清理
 *
 * 设计：
 * - 单例模式，全局唯一实例
 * - 内部使用线程安全的map存储服务
 * - 支持DeathRecipient注册，服务死亡时自动清理
 */
class SamgrMini : public IRemoteStub<IServiceRegistry>,
                  public ISystemAbilityManager {
public:
    // IServiceRegistry 接口实现
    int32_t AddService(const std::u16string& name,
                       const sptr<IRemoteObject>& service) override;
    sptr<IRemoteObject> GetService(const std::u16string& name) override;
    sptr<IRemoteObject> CheckService(const std::u16string& name) override;

    // ISystemAbilityManager 接口实现
    int32_t AddSystemAbility(int32_t systemAbilityId,
                             const sptr<IRemoteObject>& ability) override;
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId) override;
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) override;

    /**
     * @brief 设置服务死亡通知回调
     * @param name 服务名称
     * @param recipient 死亡通知接收者
     * @return true表示成功
     */
    bool AddServiceDeathRecipient(const std::u16string& name,
                                   const sptr<IRemoteObject::DeathRecipient>& recipient);

    /**
     * @brief 移除服务死亡通知
     * @param name 服务名称
     * @param recipient 死亡通知接收者
     * @return true表示成功
     */
    bool RemoveServiceDeathRecipient(const std::u16string& name,
                                      const sptr<IRemoteObject::DeathRecipient>& recipient);

    /**
     * @brief 获取全局SamgrMini实例
     */
    static sptr<SamgrMini> GetInstance();

    /**
     * @brief 获取SamgrMini实例（用于iface_cast转换）
     */
    sptr<IRemoteObject> AsObject() override;

    /**
     * @brief 处理远程请求（IRemoteStub）
     */
    int OnRemoteRequest(uint32_t code, MessageParcel& data,
                        MessageParcel& reply, MessageOption& option) override;

    /**
     * @brief 列出所有已注册的服务
     */
    void ListServices(std::vector<std::u16string>& names);

    /**
     * @brief 列出所有已注册的系统能力
     */
    void ListSystemAbilities(std::vector<int32_t>& saIds);

private:
    // 私有构造函数（单例模式）
    SamgrMini();
    ~SamgrMini() override = default;

    // 禁止拷贝
    SamgrMini(const SamgrMini&) = delete;
    SamgrMini& operator=(const SamgrMini&) = delete;

    // 服务死亡处理回调
    void OnServiceDied(const wptr<IRemoteObject>& object);

    // 查找并移除死亡的服务
    void RemoveDeadService(const sptr<IRemoteObject>& object);

    // 存储服务名称到对象的映射（线程安全）
    SafeMap<std::u16string, sptr<IRemoteObject>> servicesMap_;

    // 存储SAID到对象的映射（线程安全）
    SafeMap<int32_t, sptr<IRemoteObject>> saMap_;

    // 服务死亡通知映射
    std::mutex deathRecipientMutex_;
    std::map<std::u16string, std::vector<sptr<IRemoteObject::DeathRecipient>>> serviceDeathRecipients_;
    std::map<int32_t, std::vector<sptr<IRemoteObject::DeathRecipient>>> saDeathRecipients_;

    // 单例实例
    static std::mutex instanceMutex_;
    static sptr<SamgrMini> instance_;
};

} // namespace OHOS

#endif // SAMGR_MINI_SAMGR_MINI_H
