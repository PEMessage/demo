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

#ifndef UTILS_BASE_SAFE_MAP_H
#define UTILS_BASE_SAFE_MAP_H

#include <map>
#include <mutex>

namespace OHOS {

/**
 * @brief Provides interfaces for thread-safe map operations.
 */
template <typename K, typename V>
class SafeMap {
public:
    SafeMap() {}

    ~SafeMap() {}

    SafeMap(const SafeMap& rhs)
    {
        operator=(rhs);
    }

    SafeMap& operator=(const SafeMap& rhs)
    {
        if (this == &rhs) {
            return *this;
        }
        auto tmp = rhs.Clone();
        std::lock_guard<std::mutex> lock(mutex_);
        map_ = std::move(tmp);

        return *this;
    }

    V ReadVal(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_[key];
    }

    template<typename LambdaCallback>
    void ChangeValueByLambda(const K& key, LambdaCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback(map_[key]);
    }

    /**
     * @brief Obtains the map size.
     *
     * In the multithread scenario, the map size returned is a tmp status,
     * because elements may be inserted or removed by other threads after
     * <b>Size()</b> is called.
     */
    int Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * @brief Checks whether the map is empty.
     *
     * In the multithread scenario, the value returned by <b>Empty()</b> is a
     * tmp status, because elements may be inserted or removed by other threads
     * after <b>Empty()</b> is called.
     *
     * @return Returns <b>true</b> if the map is empty;
     * returns <b>false</b> otherwise.
     */
    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }

    /**
     * @brief Inserts an element to the map.
     *
     * @param key Indicates the key of the key-value (KV) pair to insert.
     * @param value Indicates the value of the KV pair to insert.
     * @return Returns <b>true</b> if the KV pair is inserted; returns
     * <b>false</b> otherwise.
     */
    bool Insert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = map_.insert(std::pair<K, V>(key, value));
        return ret.second;
    }

    /**
     * @brief Forcibly inserts an element to the map.
     *
     * @param key Indicates the key of the KV pair to insert.
     * @param value Indicates the value of the KV pair to insert.
     * @note If the key to insert already exists, delete and then insert
     * the KV pair to ensure that the value is inserted.
     */
    void EnsureInsert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = map_.insert(std::pair<K, V>(key, value));
        // find key and cannot insert
        if (!ret.second) {
            map_.erase(ret.first);
            map_.insert(std::pair<K, V>(key, value));
            return;
        }
        return;
    }

    /**
     * @brief Searches for an element in the map.
     *
     * @param Key Indicates the key to search.
     * @param value Indicates the value of the KV pair to search.
     * @return Returns <b>true</b> if the KV pair is found;
     * returns <b>false</b> otherwise.
     */
    bool Find(const K& key, V& value)
    {
        bool ret = false;
        std::lock_guard<std::mutex> lock(mutex_);

        auto iter = map_.find(key);
        if (iter != map_.end()) {
            value = iter->second;
            ret = true;
        }

        return ret;
    }

    /**
     * @brief Replaces the value of a KV pair.
     *
     * @param Key Indicates the key of the KV pair.
     * @param oldValue Indicates the value to be replaced.
     * @param newValue Indicates the new value of the KV pair.
     * @return Returns <b>true</b> if the key is replaced;
     * returns <b>false</b> otherwise.
     */
    bool FindOldAndSetNew(const K& key, V& oldValue, const V& newValue)
    {
        bool ret = false;
        std::lock_guard<std::mutex> lock(mutex_);
        if (map_.size() > 0) {
            auto iter = map_.find(key);
            if (iter != map_.end()) {
                oldValue = iter->second;
                map_.erase(iter);
                map_.insert(std::pair<K, V>(key, newValue));
                ret = true;
            }
        }

        return ret;
    }

    /**
     * @brief Erases a KV pair.
     *
     * @param Key Indicates the key of the KV pair to erase.
     */
    void Erase(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.erase(key);
    }

    /**
     * @brief Deletes all KV pairs from the map.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        return;
    }

    using SafeMapCallBack = std::function<void(const K, V&)>;

    /**
     * @brief Iterates over the elements of the map.
     *
     * @param callback Called to perform the custom operations on
     * each KV pair.
     */
    void Iterate(const SafeMapCallBack& callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!map_.empty()) {
            for (auto it = map_.begin(); it != map_.end(); it++) {
                callback(it -> first, it -> second);
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::map<K, V> map_;

    std::map<K, V> Clone() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_;
    }
};

} // namespace OHOS
#endif
