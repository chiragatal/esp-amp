/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if ESP_PLATFORM
#include "esp_log.h"

#define ESP_AMP_LOGE  ESP_LOGE
#define ESP_AMP_LOGW  ESP_LOGW
#define ESP_AMP_LOGI  ESP_LOGI
#define ESP_AMP_LOGD  ESP_LOGD
#define ESP_AMP_LOGV  ESP_LOGV

#define ESP_AMP_DRAM_LOGE ESP_DRAM_LOGE
#define ESP_AMP_DRAM_LOGW ESP_DRAM_LOGW
#define ESP_AMP_DRAM_LOGI ESP_DRAM_LOGI
#define ESP_AMP_DRAM_LOGD ESP_DRAM_LOGD
#define ESP_AMP_DRAM_LOGV ESP_DRAM_LOGV

#define ESP_AMP_LOG_BUFFER_HEXDUMP ESP_LOG_BUFFER_HEXDUMP

#define ESP_AMP_LOG_NONE ESP_LOG_NONE
#define ESP_AMP_LOG_ERROR ESP_LOG_ERROR
#define ESP_AMP_LOG_WARN ESP_LOG_WARN
#define ESP_AMP_LOG_INFO ESP_LOG_INFO
#define ESP_AMP_LOG_DEBUG ESP_LOG_DEBUG
#define ESP_AMP_LOG_VERBOSE ESP_LOG_VERBOSE
#endif

#ifdef __cplusplus
}
#endif
