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

#ifndef UTILS_BASE_SINGLETON_H
#define UTILS_BASE_SINGLETON_H

#include "nocopyable.h"
#include <mutex>
#include <memory>

namespace OHOS {

/**
 * The purpose of the following macro definitions is to reduce the need
 * to write repetitive code when defining singleton classes on the client side.
 *
 * Taking DelayedSingleton as an example, when declaring the target class as a
 * singleton, add the DECLARE_DELAYED_SINGLETON (class_name) to the class
 * declaration. When using the target singleton, call
 * class_name::GetInstance()->.
 */

/**
 * @brief Sets `MyClass` as a `DelayedSingleton`.
 *
 * A `MyClass` object can be obtained by calling
 * `DelayedSingleton<MyClass>::GetInstance()`.
 *
 * @param MyClass Target class to be set as a singleton.
 *
 * @note This macro definition should be used in the body of a class definition.
 */
#define DECLARE_DELAYED_SINGLETON(MyClass)\
public:\
    ~MyClass();\
private:\
    friend DelayedSingleton<MyClass>;\
    MyClass();

/**
 * @brief Sets `MyClass` as a `DelayedRefSingleton`.
 *
 * A `MyClass` object can be obtained by calling
 * `DelayedRefSingleton<MyClass>::GetInstance()`.
 *
 * @param MyClass Target class to be set as a singleton.
 *
 * @note This macro definition should be used in the body of a class definition.
 */
#define DECLARE_DELAYED_REF_SINGLETON(MyClass)\
private:\
    friend DelayedRefSingleton<MyClass>;\
    ~MyClass();\
    MyClass();

/**
 * @brief Sets `MyClass` as a `Singleton`.
 *
 * A `MyClass` object can be obtained by calling
 * `Singleton<MyClass>::GetInstance()`.
 *
 * @param MyClass Target class to be set as a singleton.
 *
 * @note This macro definition should be used in the body of a class definition.
 */
#define DECLARE_SINGLETON(MyClass)\
private:\
    friend Singleton<MyClass>;\
    MyClass& operator=(const MyClass&) = delete;\
    MyClass(const MyClass&) = delete;\
    MyClass();\
    ~MyClass();

/**
 * @brief DelayedSingleton is a thread-safe, memory-safe lazy initialized
 * singleton (with smart pointer and lock).
 */
template<typename T>
class DelayedSingleton : public NoCopyable {
public:
    /**
     * @brief Creates a unique instance object.
     *
     * Use a smart pointer to manage resources. If all shared_ptrs are
     * destroyed, the new object will also be deleted. This avoids memory leaks.
     * A lock is added only when the pointer is empty, so as to avoid locking
     * every time `GetInstance()` is called, reducing overhead of the lock.
     */
    static std::shared_ptr<T> GetInstance();
    /**
     * @brief Releases the ownership of managed object of the smart pointer.
     *
     * @note After this API is called successfully, 'GetInstance()' will create
     * a new object. If the old object has an external 'std::shared_ptr'
     * reference, you need to release it to maintain a singleton.
     */
    static void DestroyInstance();

private:
    static std::shared_ptr<T> instance_; // Record the created DelayedSingleton instance.
    static std::mutex mutex_; // Mutex, which guarantees that only one thread is accessing a common resource at any time.
};

template<typename T>
std::shared_ptr<T> DelayedSingleton<T>::instance_ = nullptr;

template<typename T>
std::mutex DelayedSingleton<T>::mutex_;

template<typename T>
std::shared_ptr<T> DelayedSingleton<T>::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            std::shared_ptr<T> temp(new (std::nothrow) T);
            instance_ = temp;
        }
    }

    return instance_;
}

template<typename T>
void DelayedSingleton<T>::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        instance_.reset();
        instance_ = nullptr;
    }
}

/**
 * @brief DelayedRefSingleton is a thread-safe, lazy initialized
 * singleton (with ordinary pointer and lock).
 */
template<typename T>
class DelayedRefSingleton : public NoCopyable {
public:
    /**
     * @brief Creates a unique instance object.
     *
     * Pointer is used in the implementation, and the return value is a
     * reference. It points to an instance that has a lifetime managed by
     * the non-user code.
     *
     * @note The instance may not have been created at a certain point in time,
     * or it may have been deleted, and therefore it is not possible to
     * prohibit use of the delete keyword to destroy the object in advance.
     */
    static T& GetInstance();

private:
    static T* instance_; // Record the DelayedRefSingleton instance created.
    static std::mutex mutex_;
};

template<typename T>
T* DelayedRefSingleton<T>::instance_ = nullptr;

template<typename T>
std::mutex DelayedRefSingleton<T>::mutex_;

template<typename T>
T& DelayedRefSingleton<T>::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new T();
        }
    }

    return *instance_;
}

/**
 * @brief Singleton is a normally initialized singleton (without pointers and
 * locks).
 */
template<typename T>
class Singleton : public NoCopyable {
public:

    /**
     * @brief Gets a singleton instance object.
     */
    static T& GetInstance() { return instance_; }

private:
    static T instance_;
};

template<typename T>
T Singleton<T>::instance_;
} // namespace OHOS

#endif
