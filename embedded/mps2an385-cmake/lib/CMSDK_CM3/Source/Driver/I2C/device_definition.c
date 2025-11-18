/*
 * Copyright (c) 2019-2023 Arm Limited. All rights reserved.
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
 * \file device_definition.c
 * \brief This file defines exports the structures based on the peripheral
 * definitions from device_cfg.h.
 * This file is meant to be used as a helper for baremetal
 * applications and/or as an example of how to configure the generic
 * driver structures.
 */

#include "device_definition.h"
#include "platform_base_address.h"

#include "cmsis_gcc.h"

void wait_ms_nop(uint32_t ms) {
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

#ifdef I2C0_SBCON
/* I2C_SBCon driver structures */
static struct i2c_sbcon_dev_cfg_t I2C0_SBCON_DEV_CFG = {
    .base = I2C_TOUCH_BASE, .default_freq_hz = 100000, .sleep_us = &wait_ms_nop};
static struct i2c_sbcon_dev_data_t I2C0_SBCON_DEV_DATA = {.freq_us = 0, .sys_clk = 0, .state = 0};
struct i2c_sbcon_dev_t I2C0_SBCON_DEV = {.cfg = &(I2C0_SBCON_DEV_CFG), .data = &(I2C0_SBCON_DEV_DATA)};
#endif

#ifdef I2C1_SBCON
/* I2C_SBCon driver structures */
static struct i2c_sbcon_dev_cfg_t I2C1_SBCON_DEV_CFG = {
    .base = I2C_AUDIO_BASE, .default_freq_hz = 100000, .sleep_us = &wait_ms_nop};
static struct i2c_sbcon_dev_data_t I2C1_SBCON_DEV_DATA = {.freq_us = 0, .sys_clk = 0, .state = 0};
struct i2c_sbcon_dev_t I2C1_SBCON_DEV = {.cfg = &(I2C1_SBCON_DEV_CFG), .data = &(I2C1_SBCON_DEV_DATA)};
#endif

#ifdef I2C2_SBCON
/* I2C_SBCon driver structures */
static struct i2c_sbcon_dev_cfg_t I2C2_SBCON_DEV_CFG = {
    .base = I2C_SHIELD0_BASE, .default_freq_hz = 100000, .sleep_us = &wait_ms_nop};
static struct i2c_sbcon_dev_data_t I2C2_SBCON_DEV_DATA = {.freq_us = 0, .sys_clk = 0, .state = 0};
struct i2c_sbcon_dev_t I2C2_SBCON_DEV = {.cfg = &(I2C2_SBCON_DEV_CFG), .data = &(I2C2_SBCON_DEV_DATA)};
#endif

#ifdef I2C3_SBCON
/* I2C_SBCon driver structures */
static struct i2c_sbcon_dev_cfg_t I2C3_SBCON_DEV_CFG = {
    .base = I2C_SHIELD1_BASE, .default_freq_hz = 100000, .sleep_us = &wait_ms_nop};
static struct i2c_sbcon_dev_data_t I2C3_SBCON_DEV_DATA = {.freq_us = 0, .sys_clk = 0, .state = 0};
struct i2c_sbcon_dev_t I2C3_SBCON_DEV = {.cfg = &(I2C3_SBCON_DEV_CFG), .data = &(I2C3_SBCON_DEV_DATA)};
#endif
