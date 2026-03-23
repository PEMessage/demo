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

#ifndef RPC_COMMON_FUNCTIONAL_DEATH_RECIPIENT_H
#define RPC_COMMON_FUNCTIONAL_DEATH_RECIPIENT_H

#include "iremote_object.h"
#include <functional>

namespace OHOS {

/**
 * @brief Generic Death Recipient using C++ std::function style
 * 
 * Usage:
 *   auto deathRecipient = new FunctionalDeathRecipient(
 *       [this](const wptr<IRemoteObject>& obj) {
 *           this->OnServiceDied(obj);
 *       });
 *   remoteObject->AddDeathRecipient(deathRecipient);
 * 
 * Note: AddDeathRecipient holds a strong reference, so you don't need
 * to keep the deathRecipient sptr yourself.
 */
class FunctionalDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    using DeathCallback = std::function<void(const wptr<IRemoteObject>&)>;

    explicit FunctionalDeathRecipient(DeathCallback callback);
    ~FunctionalDeathRecipient() override = default;

    void OnRemoteDied(const wptr<IRemoteObject>& object) override;

private:
    DeathCallback callback_;
};

} // namespace OHOS

#endif // RPC_COMMON_FUNCTIONAL_DEATH_RECIPIENT_H
