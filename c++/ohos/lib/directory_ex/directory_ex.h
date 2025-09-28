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

#ifndef DIRECTORY_EX_H
#define DIRECTORY_EX_H

#include <string>
#include <vector>
#include <sys/stat.h>
#ifdef UTILS_CXX_RUST
#include "cxx.h"
#endif

namespace OHOS {

#ifdef UTILS_CXX_RUST
rust::String RustGetCurrentProcFullFileName();
rust::String RustGetCurrentProcPath();
rust::String RustExtractFilePath(const rust::String& fileFullName);
rust::String RustExtractFileName(const rust::String& fileFullName);
rust::String RustExtractFileExt(const rust::String& fileName);
rust::String RustExcludeTrailingPathDelimiter(const rust::String& path);
rust::String RustIncludeTrailingPathDelimiter(const rust::String& path);
bool RustPathToRealPath(const rust::String& path, rust::String& realPath);
void RustGetDirFiles(const rust::String& path, rust::vec<rust::String>& files);
#endif

/**
 * @brief Obtains the full absolute path of this program.
 *
 * <b>/proc/self/exe</b> indicates the program, and you can obtain its absolute
 * path by using readlink().
 */
std::string GetCurrentProcFullFileName();

/**
 * @brief Obtains the absolute path of this program.
 */
std::string GetCurrentProcPath();

/**
 * @brief Obtains the path of a file based on the full path.
 */
std::string ExtractFilePath(const std::string& fileFullName);

/**
 * @brief Obtains the name of a file based on the full path.
 */
std::string ExtractFileName(const std::string& fileFullName);

/**
 * @brief Obtains the filename extension based on the full path.
 *
 */
std::string ExtractFileExt(const std::string& fileName);

/**
 * @brief Excludes the trailing path delimiter '/' from the <b>strPath</b>.
 *
 * If the path ends with '/', returns the path after removing '/'.
 * Otherwise, returns the path directly.
 */
std::string ExcludeTrailingPathDelimiter(const std::string& path);

/**
 * @brief Includes the trailing path delimiter '/' in the <b>strPath</b>.
 *
 * If the path ends with "/", returns the path.
 * Otherwise, returns the path with an appended delimiter.
 */
std::string IncludeTrailingPathDelimiter(const std::string& path);

/**
 * @brief Obtains the names of all files in the specified directory recursively.
 *
 * @param path Indicates the target directory.
 * @param files Indicates the <b>std::vector</b> to store the file names.
 */
void GetDirFiles(const std::string& path, std::vector<std::string>& files);

/**
 * @brief Checks whether a folder is empty.
 *
 * @return Returns <b>true</b> if the folder is empty;
 * returns <b>false</b> otherwise.
 */
bool IsEmptyFolder(const std::string& path);

/**
 * @brief Creates a directory recursively.
 *
 * The parent directory can be created at the same time when it does not exist.
 *
 * @note If there are errors such as 'Permission Denied', the creation may
 * also fail.
 * @return Returns <b>true</b> if the directory is created;
 * returns <b>false</b> otherwise.
 */
bool ForceCreateDirectory(const std::string& path);

/**
 * @brief Deletes a directory.
 *
 * All subdirectories and files in the specified directory will also be deleted.
 *
 * @note It is not necessarily successful to delete.
 * @note If there are errors such as 'Permission Denied', the deletion may
 * also fail.
 * @return Returns <b>true</b> if the directory is deleted;
 * returns <b>false</b> otherwise.
 */
bool ForceRemoveDirectory(const std::string& path);

bool ForceRemoveDirectoryBMS(const std::string& path);

/**
 * @brief Removes the file specified by <b>fileName</b>.
 *
 * @return Returns <b>true</b> if the file is removed;
 * returns <b>false</b> otherwise.
 */
bool RemoveFile(const std::string& fileName);

/**
 * @brief Obtains the folder size, in bytes.
 */
uint64_t GetFolderSize(const std::string& path);

/**
 * @brief Changes the access permissions on a file.
 *
 * @param mode Indicates the permissions on the file.
 * For details, see <b>chmod()</b>.
 * @return Returns <b>true</b> if the permissions are changed;
 * returns <b>false</b> otherwise.
 */
bool ChangeModeFile(const std::string& fileName, const mode_t& mode);

/**
 * @brief Changes the access permissions on a directory and all its
 * subdirectories.
 *
 * @param mode Indicates the permissions. For details, see <b>chmod()</b>.
 * @return Returns <b>true</b> if the permissions are changed;
 * returns <b>false</b> otherwise.
 */
bool ChangeModeDirectory(const std::string& path, const mode_t& mode);

/**
 * @brief Obtains the real path from a relative path.
 *
 * @return Returns <b>true</b> if the real path is obtained;
 * returns <b>false</b> otherwise.
 */
bool PathToRealPath(const std::string& path, std::string& realPath);

#if defined(IOS_PLATFORM) || defined(_WIN32)
/**
 * @brief Transforms a file name to that for Windows or macOS.
 */
std::string TransformFileName(const std::string& fileName);
#endif
} // OHOS
#endif