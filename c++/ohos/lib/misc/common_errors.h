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

 /**
  * @file common_errors.h
  *
  * @brief Provides values of the <b>Module</b> segment in <b>ErrCode</b>
  * for all modules in the commonlibrary subsystem.
  */

#ifndef UTILS_COMMON_ERRORS_H
#define UTILS_COMMON_ERRORS_H

namespace OHOS {
namespace Utils {

/**
 * ErrCode layout
 *
 * +-----+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | Bit |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
 * +-----+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |Field|Reserved|        Subsystem      |  Module      |                  Code                         |
 * +-----+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * In this file, the subsystem is "SUBSYS_COMMON".
 */

/**
 * @brief Enumerates the values of the <b>Module</b> segment of ErrCode
 * for modules in the commonlibrary subsystem.
 *
 * @var MODULE_DEFAULT Default
 * @var MODULE_TIMER Timer(timer.h)
 */
enum {
    MODULE_DEFAULT              = 0,
    MODULE_TIMER                = 1,
    MODULE_MAPPED_FILE          = 2,
    MODULE_EVENT_SYS            = 3,
    // new module
};

} // Utils
} // OHOS

#endif
