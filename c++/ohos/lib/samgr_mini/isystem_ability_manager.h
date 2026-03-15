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

#ifndef SAMGR_MINI_ISYSTEM_ABILITY_MANAGER_H
#define SAMGR_MINI_ISYSTEM_ABILITY_MANAGER_H

#include "iremote_broker.h"

namespace OHOS {

/**
 * @brief ISystemAbilityManager - 系统能力管理器接口
 * 提供按整数SAID注册和查询系统能力的接口
 */
class ISystemAbilityManager : public IRemoteBroker {
public:
    /**
     * @brief 注册系统能力
     * @param systemAbilityId 系统能力ID (SAID)
     * @param ability 系统能力对象
     * @return 0表示成功，其他表示失败
     */
    virtual int32_t AddSystemAbility(int32_t systemAbilityId,
                                     const sptr<IRemoteObject>& ability) = 0;

    /**
     * @brief 获取系统能力（阻塞等待）
     * @param systemAbilityId 系统能力ID
     * @return 系统能力对象，失败返回nullptr
     */
    virtual sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId) = 0;

    /**
     * @brief 检查系统能力是否存在（非阻塞）
     * @param systemAbilityId 系统能力ID
     * @return 系统能力对象，不存在返回nullptr
     */
    virtual sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) = 0;

    /**
     * @brief 获取接口描述符
     */
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.samgr.ISystemAbilityManager");
};

} // namespace OHOS

#endif // SAMGR_MINI_ISYSTEM_ABILITY_MANAGER_H
