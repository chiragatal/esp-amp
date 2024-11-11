/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "sdkconfig.h"

#if CONFIG_IDF_TARGET_ESP32C6
#define ESP_AMP_SHARED_MEM_BOUNDARY 0x4087c610
#elif CONFIG_IDF_TARGET_ESP32P4
#define ESP_AMP_SHARED_MEM_BOUNDARY 0x4ff80000
#endif

#if CONFIG_IDF_TARGET_ESP32C6
#define ESP_AMP_SUBCORE_START_ENTRY 0x50000080
#endif

#if CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM
#if CONFIG_ESP_AMP_SHARED_MEM_IN_HP
#define SUBCORE_USE_HP_MEM_BOUNDARY (ESP_AMP_SHARED_MEM_BOUNDARY - CONFIG_ESP_AMP_SHARED_MEM_SIZE)
#else
#define SUBCORE_USE_HP_MEM_BOUNDARY ESP_AMP_SHARED_MEM_BOUNDARY
#endif /* ESP_AMP_SHARED_MEM_IN_HP */
#endif

#define SUBCORE_USE_HP_MEM_SIZE CONFIG_ESP_AMP_SUBCORE_USE_HP_MEM_SIZE
