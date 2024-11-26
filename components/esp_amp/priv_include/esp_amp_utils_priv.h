/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#if IS_MAIN_CORE
uint16_t get_aligned_size(uint16_t size);
uint16_t get_power_len(uint16_t len);
#endif

#ifdef __cplusplus
}
#endif
