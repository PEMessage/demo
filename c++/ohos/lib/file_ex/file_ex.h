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

/** @file file_ex.h
*
*  @brief Provides global file operation functions implemented in c_utils.
*/

/**
* @defgroup FileReadWrite
* @{
* @brief Provides interfaces for reading data from and writing data to files.
*
* You can use the interfaces to read data from a file, write data to a file,
* and search for the specified string.
*/

#ifndef UTILS_BASE_FILE_EX_H
#define UTILS_BASE_FILE_EX_H

#include <string>
#include <vector>
#ifdef UTILS_CXX_RUST
#include "cxx.h"
#endif

namespace OHOS {
#ifdef UTILS_CXX_RUST
bool RustLoadStringFromFile(const rust::String& filePath, rust::String& content);
bool RustLoadStringFromFd(int fd, rust::String& content);
bool RustLoadBufferFromFile(const rust::String& filePath, rust::vec<char>& content);
bool RustSaveBufferToFile(const rust::String& filePath, const rust::vec<char>& content, bool truncated);
bool RustSaveStringToFile(const rust::String& filePath, const rust::String& content, bool truncated);
bool RustSaveStringToFd(int fd, const rust::String& content);
bool RustFileExists(const rust::String& fileName);
bool RustStringExistsInFile(const rust::String& fileName, const rust::String& subStr, bool caseSensitive);
int  RustCountStrInFile(const rust::String& fileName, const rust::String& subStr, bool caseSensitive);
#endif
/**
 * @ingroup FileReadWrite
 * @brief Reads a string from a file.
 *
 * @param filePath Indicates the path of the target file.
 * @param content Indicates the <b>std::string</b> object used to hold
 * the data read.
 * @return Returns <b>true</b> if the string is read successfully;
 * returns <b>false</b> otherwise.
 * @note The maximum file size is 32 MB.
 */
bool LoadStringFromFile(const std::string& filePath, std::string& content);

/**
 * @ingroup FileReadWrite
 * @brief Writes a string to a file.
 *
 * @param filePath Indicates the path of the target file.
 * @param content Indicates the <b>std::string</b> object to write.
 * @param truncated Indicates whether to truncate the original file.
 * @return Returns <b>true</b> if the string is written successfully;
 * returns <b>false</b> otherwise.
 */
bool SaveStringToFile(const std::string& filePath, const std::string& content, bool truncated = true);

/**
 * @ingroup FileReadWrite
 * @brief Reads a string from a file specified by its file descriptor (FD).
 *
 * @param fd Indicates the FD of the file to read.
 * @param content Indicates the <b>std::string</b> object used to hold
 * the data read.
 * @return Returns <b>true</b> if the string is read successfully;
 * returns <b>false</b> otherwise.
 */
bool LoadStringFromFd(int fd, std::string& content);

/**
 * @ingroup FileReadWrite
 * @brief Writes a string to a file specified by its FD.
 *
 * @param fd Indicates the FD of the file to write.
 * @param content Indicates the <b>std::string</b> object to write.
 * @return Returns <b>true</b> if the string is written successfully;
 * returns <b>false</b> otherwise.
 */
bool SaveStringToFd(int fd, const std::string& content);

/**
 * @ingroup FileReadWrite
 * @brief Reads data as a vector from a file.
 *
 * @param filePath Indicates the path of the target file.
 * @param content Indicates the <b>std::vector</b> object used to hold
 * the data read.
 * @return Returns <b>true</b> if the data is read successfully;
 * returns <b>false</b> otherwise.
 */
bool LoadBufferFromFile(const std::string& filePath, std::vector<char>& content);

/**
 * @ingroup FileReadWrite
 * @brief Writes data of a vector to a file.
 *
 * @param filePath Indicates the path of the target file.
 * @param content Indicates the <b>std::vector</b> object to write.
 * @return Returns <b>true</b> if the data is written successfully;
 * returns <b>false</b> otherwise.
 */
bool SaveBufferToFile(const std::string& filePath, const std::vector<char>& content, bool truncated = true);

/**
 * @ingroup FileReadWrite
 * @brief Checks whether a file exists.
 *
 * @param filePath Indicates the file to check.
 * @return Returns <b>true</b> if the file exists; returns <b>false</b>
 * if any error (e.g. Permission Denied) occurs.
 */
bool FileExists(const std::string& fileName);

/**
 * @ingroup FileReadWrite
 * @brief Checks whether a file contains the specified string.
 *
 * @param fileName Indicates the path of the target file.
 * @param subStr Indicates the <b>std::string</b> object to check.
 * @param caseSensitive Indicates whether the string is case-sensitive.
 * @return Returns <b>true</b> if the file contains the specified string;
 * returns <b>false</b> otherwise.
 */
bool StringExistsInFile(const std::string& fileName, const std::string& subStr, bool caseSensitive = true);

/**
 * @ingroup FileReadWrite
 * @brief Obtains the number of occurrences of the specified string in a file.
 *
 * @param fileName Indicates the path of the target file.
 * @param subStr Indicates the <b>std::string</b> object to search.
 * @param caseSensitive Indicates whether the string is case-sensitive.
 * @return Returns the number of occurrences of the string in the file;
 * returns <b>0</b> if <b>subStr</b> is null.
 */
int  CountStrInFile(const std::string& fileName, const std::string& subStr, bool caseSensitive = true);
}

#endif

/**@}*/
