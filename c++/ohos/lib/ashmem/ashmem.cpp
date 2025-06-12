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

#include "ashmem.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "securec.h"
#include "utils_log.h"

namespace OHOS {
static pthread_mutex_t g_ashmemLock = PTHREAD_MUTEX_INITIALIZER;

#ifdef UTILS_CXX_RUST
std::shared_ptr<Ashmem> CreateAshmemStd(const char *name, int32_t size)
{
    if ((name == nullptr) || (size <= 0)) {
        UTILS_LOGE("%s: Parameter is invalid, size= %d", __func__, size);
        return std::shared_ptr<Ashmem>{};
    }

    int fd = AshmemCreate(name, size);
    if (fd < 0) {
        UTILS_LOGE("%s: Failed to exec AshmemCreate, fd= %d", __func__, size);
        return std::shared_ptr<Ashmem>{};
    }

    return std::make_shared<Ashmem>(fd, size);
}

const c_void* AsVoidPtr(const char* inPtr)
{
    return static_cast<const c_void*>(inPtr);
}

const char* AsCharPtr(const c_void* inPtr)
{
    return static_cast<const char*>(inPtr);
}
#endif

static int AshmemOpenLocked()
{
    int fd = TEMP_FAILURE_RETRY(open("/dev/ashmem", O_RDWR | O_CLOEXEC));
    if (fd < 0) {
        UTILS_LOGE("%s: fd is invalid, fd = %d, errno = %d", __func__, fd, errno);
        return fd;
    }

    struct stat st;
    int ret = TEMP_FAILURE_RETRY(fstat(fd, &st));
    if (ret < 0) {
        UTILS_LOGE("%s: Failed to exec fstat, ret = %d, errno = %d", __func__, ret, errno);
        close(fd);
        return ret;
    }

    if (!S_ISCHR(st.st_mode) || !st.st_rdev) {
        UTILS_LOGE("%s: stat status is invalid, st_mode = %u", __func__, st.st_mode);
        close(fd);
        return -1;
    }
    return fd;
}

static int AshmemOpen()
{
    pthread_mutex_lock(&g_ashmemLock);
    int fd = AshmemOpenLocked();
    pthread_mutex_unlock(&g_ashmemLock);
    return fd;
}

/*
 * AshmemCreate - create a new ashmem region and returns the file descriptor
 * fd < 0 means failed
 *
 */
int AshmemCreate(const char *name, size_t size)
{
    int ret;
    int fd = AshmemOpen();
    if (fd < 0) {
        UTILS_LOGE("%s: Failed to exec AshmemOpen fd = %d", __func__, fd);
        return fd;
    }

    if (name != nullptr) {
        char buf[ASHMEM_NAME_LEN] = {0};
        ret = strcpy_s(buf, sizeof(buf), name);
        if (ret != EOK) {
            UTILS_LOGE("%s: Failed to exec strcpy_s, name= %s, ret= %d", __func__, name, ret);
            close(fd);
            return -1;
        }
        ret = TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_SET_NAME, buf));
        if (ret < 0) {
            UTILS_LOGE("%s: Failed to set name, name= %s, ret= %d, errno = %d",
                       __func__, name, ret,  errno);
            close(fd);
            return ret;
        }
    }

    ret = TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_SET_SIZE, size));
    if (ret < 0) {
        UTILS_LOGE("%s: Failed to set size, size= %zu, errno = %d", __func__, size, errno);
        close(fd);
        return ret;
    }
    return fd;
}

int AshmemSetProt(int fd, int prot)
{
    return TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_SET_PROT_MASK, prot));
}

int AshmemGetSize(int fd)
{
    return TEMP_FAILURE_RETRY(ioctl(fd, ASHMEM_GET_SIZE, NULL));
}

Ashmem::Ashmem(int fd, int32_t size) : memoryFd_(fd), memorySize_(size), flag_(0), startAddr_(nullptr)
{
}

Ashmem::~Ashmem()
{
    UnmapAshmem();
    CloseAshmem();
}

sptr<Ashmem> Ashmem::CreateAshmem(const char *name, int32_t size)
{
    if ((name == nullptr) || (size <= 0)) {
        UTILS_LOGE("%s: Parameter is invalid, size= %d", __func__, size);
        return nullptr;
    }

    int fd = AshmemCreate(name, size);
    if (fd < 0) {
        UTILS_LOGE("%s: Failed to exec AshmemCreate, fd= %d", __func__, size);
        return nullptr;
    }

    return new Ashmem(fd, size);
}

bool Ashmem::SetProtection(int protectionType) const
{
    int result = AshmemSetProt(memoryFd_, protectionType);
    return result >= 0;
}

int Ashmem::GetProtection() const
{
    return TEMP_FAILURE_RETRY(ioctl(memoryFd_, ASHMEM_GET_PROT_MASK));
}

int32_t Ashmem::GetAshmemSize() const
{
    return AshmemGetSize(memoryFd_);
}

#ifdef UTILS_CXX_RUST
void Ashmem::CloseAshmem() const
#else
void Ashmem::CloseAshmem()
#endif
{
    if (memoryFd_ > 0) {
        ::close(memoryFd_);
        memoryFd_ = -1;
    }
    memorySize_ = 0;
    flag_ = 0;
    startAddr_ = nullptr;
}

#ifdef UTILS_CXX_RUST
bool Ashmem::MapAshmem(int mapType) const
#else
bool Ashmem::MapAshmem(int mapType)
#endif
{
    void *startAddr = ::mmap(nullptr, memorySize_, mapType, MAP_SHARED, memoryFd_, 0);
    if (startAddr == MAP_FAILED) {
        UTILS_LOGE("Failed to exec mmap, errno = %d", errno);
        return false;
    }

    startAddr_ = startAddr;
    flag_ = mapType;

    return true;
}

#ifdef UTILS_CXX_RUST
bool Ashmem::MapReadAndWriteAshmem() const
#else
bool Ashmem::MapReadAndWriteAshmem()
#endif
{
    return MapAshmem(PROT_READ | PROT_WRITE);
}

#ifdef UTILS_CXX_RUST
bool Ashmem::MapReadOnlyAshmem() const
#else
bool Ashmem::MapReadOnlyAshmem()
#endif
{
    return MapAshmem(PROT_READ);
}

#ifdef UTILS_CXX_RUST
void Ashmem::UnmapAshmem() const
#else
void Ashmem::UnmapAshmem()
#endif
{
    if (startAddr_ != nullptr) {
        ::munmap(startAddr_, memorySize_);
        startAddr_ = nullptr;
    }
    flag_ = 0;
}

#ifdef UTILS_CXX_RUST
bool Ashmem::WriteToAshmem(const void *data, int32_t size, int32_t offset) const
#else
bool Ashmem::WriteToAshmem(const void *data, int32_t size, int32_t offset)
#endif
{
    if (data == nullptr) {
        return false;
    }

    if (!CheckValid(size, offset, PROT_WRITE)) {
        UTILS_LOGE("%s: invalid input or not map", __func__);
        return false;
    }

    auto tmpData = reinterpret_cast<char *>(startAddr_);
    int ret = memcpy_s(tmpData + offset, memorySize_ - offset, reinterpret_cast<const char *>(data), size);
    if (ret != EOK) {
        UTILS_LOGE("%s: Failed to memcpy, ret = %d", __func__, ret);
        return false;
    }

    return true;
}

#ifdef UTILS_CXX_RUST
const void *Ashmem::ReadFromAshmem(int32_t size, int32_t offset) const
#else
const void *Ashmem::ReadFromAshmem(int32_t size, int32_t offset)
#endif
{
    if (!CheckValid(size, offset, PROT_READ)) {
        UTILS_LOGE("%s: invalid input or not map", __func__);
        return nullptr;
    }

    return reinterpret_cast<const char *>(startAddr_) + offset;
}

bool Ashmem::CheckValid(int32_t size, int32_t offset, int cmd) const
{
    if (startAddr_ == nullptr) {
        return false;
    }
    if ((size < 0) || (size > memorySize_) || (offset < 0) || (offset > memorySize_)) {
        UTILS_LOGE("%s: , invalid parameter, size = %d, memorySize_ = %d, offset = %d",
            __func__, size, memorySize_, offset);
        return false;
    }
    if (offset + size > memorySize_) {
        UTILS_LOGE("%s: , invalid parameter, size = %d, memorySize_ = %d, offset = %d",
            __func__, size, memorySize_, offset);
        return false;
    }
    if (!(static_cast<uint32_t>(GetProtection()) & static_cast<uint32_t>(cmd)) ||
        !(static_cast<uint32_t>(flag_) & static_cast<uint32_t>(cmd))) {
        return false;
    }

    return true;
}
}

