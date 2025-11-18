/*
 * Copyright (c) 2019-2022 Arm Limited
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file platform_base_address.h
 * \brief This file defines all the peripheral base addresses for AN552 MPS3 SSE-300 +
 *        Ethos-U55 platform.
 */

#ifndef __PLATFORM_BASE_ADDRESS_H__
#define __PLATFORM_BASE_ADDRESS_H__

#define I2C_TOUCH_BASE     0x40022000 /* FPGA - SBCon I2C (Touch Conf) base address */
#define I2C_AUDIO_BASE     0x40023000 /* FPGA - SBCon I2C (Audio Conf) base address */

#define I2C_SHIELD0_BASE    0x40029000
#define I2C_SHIELD1_BASE    0x4002a000

#endif  /* __PLATFORM_BASE_ADDRESS_H__ */
