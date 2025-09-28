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

#include "directory_ex.h"
#include <dirent.h>
#include <fcntl.h>
#include <stack>
#include "securec.h"
#include "unistd.h"
#include "utils_log.h"
using namespace std;

namespace OHOS {

#ifdef UTILS_CXX_RUST
rust::String RustGetCurrentProcFullFileName()
{
    return rust::String(GetCurrentProcFullFileName());
}

rust::String RustGetCurrentProcPath()
{
    return rust::String(GetCurrentProcPath());
}

rust::String RustExtractFilePath(const rust::String& fileFullName)
{
    std::string tmpName = std::string(fileFullName);
    return rust::String(ExtractFilePath(tmpName));
}

rust::String RustExtractFileName(const rust::String& fileFullName)
{
    std::string tmpName = std::string(fileFullName);
    return rust::String(ExtractFileName(tmpName));
}

rust::String RustExtractFileExt(const rust::String& fileName)
{
    std::string tmpName = std::string(fileName);
    return rust::String(ExtractFileExt(tmpName));
}

rust::String RustExcludeTrailingPathDelimiter(const rust::String& path)
{
    std::string tmpPath = std::string(path);
    return rust::String(ExcludeTrailingPathDelimiter(tmpPath));
}

rust::String RustIncludeTrailingPathDelimiter(const rust::String& path)
{
    std::string tmpPath = std::string(path);
    return rust::String(IncludeTrailingPathDelimiter(tmpPath));
}

bool RustPathToRealPath(const rust::String& path, rust::String& realPath)
{
    std::string tmpPath = std::string(path);
    std::string tmpResolved;

    if (PathToRealPath(tmpPath, tmpResolved)) {
        realPath = tmpResolved;
        return true;
    }

    return false;
}

void RustGetDirFiles(const rust::String& path, rust::vec<rust::String>& files)
{
    std::string tmpPath(path);
    std::vector<std::string> tmpFiles(files.begin(), files.end());
    GetDirFiles(tmpPath, tmpFiles);
    std::copy(tmpFiles.begin(), tmpFiles.end(), std::back_inserter(files));
}
#endif

string GetCurrentProcFullFileName()
{
    char procFile[PATH_MAX + 1] = {0};
    int ret = readlink("/proc/self/exe", procFile, PATH_MAX);
    if (ret < 0 || ret > PATH_MAX) {
        UTILS_LOGD("Get proc name failed, ret is: %{public}d!", ret);
        return string();
    }
    procFile[ret] = '\0';
    return string(procFile);
}

string GetCurrentProcPath()
{
    return ExtractFilePath(GetCurrentProcFullFileName());
}

string ExtractFilePath(const string& fileFullName)
{
    return string(fileFullName).substr(0, fileFullName.rfind("/") + 1);
}

std::string ExtractFileName(const std::string& fileFullName)
{
    return string(fileFullName).substr(fileFullName.rfind("/") + 1, fileFullName.size());
}

string ExtractFileExt(const string& fileName)
{
    string::size_type pos = fileName.rfind(".");
    if (pos == string::npos) {
        return "";
    }

    return string(fileName).substr(pos + 1, fileName.size());
}

string ExcludeTrailingPathDelimiter(const std::string& path)
{
    if (path.rfind("/") != path.size() - 1) {
        return path;
    }

    if (!path.empty()) {
        return path.substr(0, (int)path.size() - 1);
    }

    return path;
}

string IncludeTrailingPathDelimiter(const std::string& path)
{
    if (path.empty()) {
        return  "/";
    }
    if (path.rfind("/") != path.size() - 1) {
        return path + "/";
    }

    return path;
}

void GetDirFiles(const string& path, vector<string>& files)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        UTILS_LOGD("Failed to open root dir: %{public}s: %{public}s ", path.c_str(), strerror(errno));
        return;
    }

    string currentPath = ExcludeTrailingPathDelimiter(path);
    stack<DIR *> traverseStack;
    traverseStack.push(dir);
    while (!traverseStack.empty()) {
        DIR *topNode = traverseStack.top();
        dirent *ptr = readdir(topNode);
        if (ptr == nullptr) {
            closedir(topNode);
            traverseStack.pop();
            auto pos = currentPath.find_last_of("/");
            if (pos != string::npos) {
                currentPath.erase(pos);
            }
            continue;
        }

        string name = ptr->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        if (ptr->d_type == DT_DIR) {
            int currentFd = dirfd(topNode);
            if (currentFd < 0) {
                UTILS_LOGD("Failed to get dirfd, fd: %{public}d: %{public}s ", currentFd, strerror(errno));
                continue;
            }
            int subFd = openat(currentFd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (subFd < 0) {
                UTILS_LOGD("Failed in subFd openat: %{public}s ", name.c_str());
                continue;
            }
            DIR *subDir = fdopendir(subFd);
            if (subDir == nullptr) {
                close(subFd);
                UTILS_LOGD("Failed in fdopendir: %{public}s", strerror(errno));
                continue;
            }
            traverseStack.push(subDir);
            currentPath = IncludeTrailingPathDelimiter(currentPath) + name;
        } else {
            files.push_back(IncludeTrailingPathDelimiter(currentPath) + name);
        }
    }
}

bool ForceCreateDirectory(const string& path)
{
    string::size_type index = 0;
    do {
        string subPath;
        index = path.find('/', index + 1);
        if (index == string::npos) {
            subPath = path;
        } else {
            subPath = path.substr(0, index);
        }

        if (access(subPath.c_str(), F_OK) != 0) {
            if (mkdir(subPath.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != 0 && errno != EEXIST) {
                return false;
            }
        }
    } while (index != string::npos);

    return access(path.c_str(), F_OK) == 0;
}

struct DirectoryNode {
    DIR *dir;
    int currentFd;
    char name[256]; // the same max char length with d_name in struct dirent
};

bool ForceRemoveDirectory(const string& path)
{
    bool ret = true;
    int strRet;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        UTILS_LOGE("Failed to open root dir: %{public}s: %{public}s ", path.c_str(), strerror(errno));
        return false;
    }
    stack<DIR *> traversStack;
    stack<DirectoryNode> removeStack;
    traversStack.push(dir);
    while (!traversStack.empty()) {
        DIR *currentDir = traversStack.top();
        traversStack.pop();
        DirectoryNode node;
        int currentFd = dirfd(currentDir);
        if (currentFd < 0) {
            UTILS_LOGE("Failed to get dirfd, fd: %{public}d: %{public}s ", currentFd, strerror(errno));
            ret = false;
            continue;
        }

        while (true) {
            struct dirent *ptr = readdir(currentDir);
            if (ptr == nullptr) {
                break;
            }
            const char *name = ptr->d_name;
            // current dir or parent dir
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            if (ptr->d_type == DT_DIR) {
                int subFd = openat(currentFd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
                if (subFd < 0) {
                    UTILS_LOGE("Failed in subFd openat: %{public}s ", name);
                    ret = false;
                    continue;
                }
                DIR *subDir = fdopendir(subFd);
                if (subDir == nullptr) {
                    close(subFd);
                    UTILS_LOGE("Failed in fdopendir: %{public}s", strerror(errno));
                    ret = false;
                    continue;
                }
                node.dir = subDir;
                node.currentFd = currentFd;
                strRet = strcpy_s(node.name, sizeof(node.name), name);
                if (strRet != EOK) {
                    UTILS_LOGE("Failed to exec strcpy_s, name= %{public}s, strRet= %{public}d", name, strRet);
                }
                removeStack.push(node);
                traversStack.push(subDir);
            } else {
                if (faccessat(currentFd, name, F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
                    if (unlinkat(currentFd, name, 0) < 0) {
                        UTILS_LOGE("Couldn't unlinkat subFile %{public}s: %{public}s", name, strerror(errno));
                        ret = false;
                        break;
                    }
                } else {
                    UTILS_LOGE("Access to file: %{public}s is failed", name);
                    ret = false;
                    break;
                }
            }
        }
    }
    if (!ret) {
        UTILS_LOGD("Failed to remove some subfile under path: %{public}s", path.c_str());
    }
    while (!removeStack.empty()) {
        DirectoryNode node = removeStack.top();
        removeStack.pop();
        closedir(node.dir);
        if (unlinkat(node.currentFd, node.name, AT_REMOVEDIR) < 0) {
            UTILS_LOGE("Couldn't unlinkat subDir %{public}s: %{public}s", node.name, strerror(errno));
            continue;
        }
    }
    closedir(dir);
    if (faccessat(AT_FDCWD, path.c_str(), F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
        if (remove(path.c_str()) != 0) {
            UTILS_LOGE("Failed to remove root dir: %{public}s: %{public}s ", path.c_str(), strerror(errno));
            return false;
        }
    }
    return faccessat(AT_FDCWD, path.c_str(), F_OK, AT_SYMLINK_NOFOLLOW) != 0;
}

bool ForceRemoveDirectoryInternal(DIR *dir)
{
    bool ret = true;
    int rootFd = dirfd(dir);
    if (rootFd < 0) {
        UTILS_LOGE("BMS: Failed to get dirfd, fd: %{public}d: %{public}s ", rootFd, strerror(errno));
        return false;
    }

    while (true) {
        struct dirent *ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
        const char *name = ptr->d_name;

        // current dir or parent dir
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (ptr->d_type == DT_DIR) {
            int subFd = openat(rootFd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (subFd < 0) {
                UTILS_LOGE("BMS: Failed in subFd openat: %{public}s, errno: %{public}s ", name, strerror(errno));
                ret = false;
                continue;
            }
            DIR *subDir = fdopendir(subFd);
            if (subDir == nullptr) {
                close(subFd);
                UTILS_LOGE("BMS: Failed in fdopendir: %{public}s", strerror(errno));
                ret = false;
                continue;
            }
            ret = ForceRemoveDirectoryInternal(subDir);
            closedir(subDir);
            if (unlinkat(rootFd, name, AT_REMOVEDIR) < 0) {
                UTILS_LOGE("BMS: Couldn't unlinkat subDir %{public}s: %{public}s", name, strerror(errno));
                ret = false;
                continue;
            }
        } else {
            if (faccessat(rootFd, name, F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
                if (unlinkat(rootFd, name, 0) < 0) {
                    UTILS_LOGE("BMS: Couldn't unlinkat subFile %{public}s: %{public}s", name, strerror(errno));
                    return false;
                }
            } else {
                UTILS_LOGE("BMS: Access to file: %{public}s is failed, errno: %{public}s", name, strerror(errno));
                return false;
            }
        }
    }

    return ret;
}

bool ForceRemoveDirectoryBMS(const string& path)
{
    bool ret = true;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        UTILS_LOGE("BMS: Failed to open root dir: %{public}s: %{public}s ", path.c_str(), strerror(errno));
        return false;
    }
    ret = ForceRemoveDirectoryInternal(dir);
    if (!ret) {
        UTILS_LOGE("BMS: Failed to remove some subfile under path: %{public}s", path.c_str());
    }
    closedir(dir);
    if (faccessat(AT_FDCWD, path.c_str(), F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
        if (remove(path.c_str()) != 0) {
            UTILS_LOGE("BMS: Failed to remove root dir: %{public}s: %{public}s ", path.c_str(), strerror(errno));
            return false;
        }
    }

    return faccessat(AT_FDCWD, path.c_str(), F_OK, AT_SYMLINK_NOFOLLOW) != 0;
}

bool RemoveFile(const string& fileName)
{
    if (access(fileName.c_str(), F_OK) == 0) {
        return remove(fileName.c_str()) == 0;
    }

    return true;
}

bool IsEmptyFolder(const string& path)
{
    vector<string> files;
    GetDirFiles(path, files);
    return files.empty();
}

uint64_t GetFolderSize(const string& path)
{
    vector<string> files;
    struct stat statbuf = {0};
    GetDirFiles(path, files);
    uint64_t totalSize = 0;
    for (auto& file : files) {
        if (stat(file.c_str(), &statbuf) == 0) {
            totalSize += statbuf.st_size;
        }
    }

    return totalSize;
}

// inner function, and param is legitimate
bool ChangeMode(const string& fileName, const mode_t& mode)
{
    return (chmod(fileName.c_str(), mode) == 0);
}

bool ChangeModeFile(const string& fileName, const mode_t& mode)
{
    if (access(fileName.c_str(), F_OK) != 0) {
        return false;
    }

    return ChangeMode(fileName, mode);
}

bool ChangeModeDirectory(const string& path, const mode_t& mode)
{
    string subPath;
    bool ret = true;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }

    while (true) {
        struct dirent *ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }

        // current dir or parent dir
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        }
        subPath = IncludeTrailingPathDelimiter(path) + string(ptr->d_name);
        if (ptr->d_type == DT_DIR) {
            ret = ChangeModeDirectory(subPath, mode);
        } else {
            if (access(subPath.c_str(), F_OK) == 0) {
                if (!ChangeMode(subPath, mode)) {
                    UTILS_LOGD("Failed to exec ChangeMode");
                    closedir(dir);
                    return false;
                }
            }
        }
    }
    closedir(dir);
    string currentPath = ExcludeTrailingPathDelimiter(path);
    if (access(currentPath.c_str(), F_OK) == 0) {
        if (!ChangeMode(currentPath, mode)) {
            UTILS_LOGD("Failed to exec ChangeMode");
            return false;
        }
    }
    return ret;
}

bool PathToRealPath(const string& path, string& realPath)
{
    if (path.empty()) {
        UTILS_LOGD("path is empty!");
        return false;
    }

    if ((path.length() >= PATH_MAX)) {
        UTILS_LOGD("path len is error, the len is: [%{public}zu]", path.length());
        return false;
    }

    char tmpPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), tmpPath) == nullptr) {
        UTILS_LOGE("path (%{public}s) to realpath error: %{public}s", path.c_str(), strerror(errno));
        return false;
    }

    realPath = tmpPath;
    if (access(realPath.c_str(), F_OK) != 0) {
        UTILS_LOGE("check realpath (%{private}s) error: %{public}s", realPath.c_str(), strerror(errno));
        return false;
    }
    return true;
}

#if defined(IOS_PLATFORM) || defined(_WIN32)
string TransformFileName(const string& fileName)
{
    string::size_type pos = fileName.find(".");
    string transformfileName = "";
    if (pos == string::npos) {
        transformfileName = fileName;

#ifdef _WIN32
        transformfileName = transformfileName.append(".dll");
#elif defined IOS_PLATFORM
        transformfileName = transformfileName.append(".dylib");
#endif

        return transformfileName;
    } else {
        transformfileName = string(fileName).substr(0, pos + 1);

#ifdef _WIN32
        transformfileName = transformfileName.append("dll");
#elif defined IOS_PLATFORM
        transformfileName = transformfileName.append("dylib");
#endif

        return transformfileName;
    }
}
#endif

} // OHOS
