/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "esp_amp_platform.h"
#include "esp_amp_log.h"
#include "esp_amp_mem_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if IS_ENV_BM
#define ESP_AMP_ASSERT(_stat) if(!(_stat)) { ESP_AMP_DRAM_LOGE("%s:%d assert failed\r\n", __FILE__, __LINE__); asm volatile ("ebreak\n"); }
#else
#define ESP_AMP_ASSERT(_stat) assert(_stat)
#endif

#ifdef __cplusplus
}
#endif
