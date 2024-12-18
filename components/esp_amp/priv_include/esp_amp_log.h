/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !IS_ENV_BM
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

#else /* IS_ENV_BM */

#include "sdkconfig.h"
#include "stdio.h"

typedef enum {
    ESP_AMP_LOG_NONE,       /*!< No log output */
    ESP_AMP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    ESP_AMP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    ESP_AMP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    ESP_AMP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    ESP_AMP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} esp_amp_log_level_t;

#define AMP_LOG_LOCAL_LEVEL ( CONFIG_LOG_DEFAULT_LEVEL )

#define ESP_AMP_LOG_LEVEL(level, tag, format, ...) do { \
        if (level==ESP_AMP_LOG_ERROR )          { printf("E %s: " format "\r\n", tag, ##__VA_ARGS__); } \
        else if (level==ESP_AMP_LOG_WARN )      { printf("W %s: " format "\r\n", tag, ##__VA_ARGS__); } \
        else if (level==ESP_AMP_LOG_DEBUG )     { printf("D %s: " format "\r\n", tag, ##__VA_ARGS__); } \
        else if (level==ESP_AMP_LOG_VERBOSE )   { printf("V %s: " format "\r\n", tag, ##__VA_ARGS__); } \
        else                                    { printf("I %s: " format "\r\n", tag, ##__VA_ARGS__); } \
    } while(0)

#define ESP_AMP_LOG_LEVEL_LOCAL(level, tag, format, ...) do { \
        if ( AMP_LOG_LOCAL_LEVEL >= level ) ESP_AMP_LOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while(0)

#define ESP_AMP_LOGE( tag, format, ... )  ESP_AMP_LOG_LEVEL_LOCAL(ESP_AMP_LOG_ERROR, tag, format, ##__VA_ARGS__)
#define ESP_AMP_LOGW( tag, format, ... )  ESP_AMP_LOG_LEVEL_LOCAL(ESP_AMP_LOG_WARN, tag, format, ##__VA_ARGS__)
#define ESP_AMP_LOGI( tag, format, ... )  ESP_AMP_LOG_LEVEL_LOCAL(ESP_AMP_LOG_INFO, tag, format, ##__VA_ARGS__)
#define ESP_AMP_LOGD( tag, format, ... )  ESP_AMP_LOG_LEVEL_LOCAL(ESP_AMP_LOG_DEBUG, tag, format, ##__VA_ARGS__)
#define ESP_AMP_LOGV( tag, format, ... )  ESP_AMP_LOG_LEVEL_LOCAL(ESP_AMP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

/* XIP is not supported yet. strings are in dram only */
#define ESP_AMP_DRAM_LOGE ESP_AMP_LOGE
#define ESP_AMP_DRAM_LOGW ESP_AMP_LOGW
#define ESP_AMP_DRAM_LOGI ESP_AMP_LOGI
#define ESP_AMP_DRAM_LOGD ESP_AMP_LOGD
#define ESP_AMP_DRAM_LOGV ESP_AMP_LOGV

#define BYTES_PER_LINE 16

static inline void esp_amp_log_buffer_hex_internal(const char *tag, const void *buffer, int buff_len,
                                                   esp_amp_log_level_t log_level)
{
    if (AMP_LOG_LOCAL_LEVEL < log_level) {
        return;
    }
    if (buff_len == 0) {
        return;
    }
    const char *p = (const char *)buffer;
    for (int i = 0; i < buff_len; i++) {
        if (i && !(i % BYTES_PER_LINE)) {
            printf("\n");
        }
        printf("%02X ", (*(p + i) & 0xff));
    }
    printf("\n");
}

#define ESP_AMP_LOG_BUFFER_HEXDUMP( tag, buffer, buff_len, level ) \
    do {\
            esp_amp_log_buffer_hex_internal( tag, buffer, buff_len, level ); \
    } while(0)
#endif /* !IS_ENV_BM */

#ifdef __cplusplus
}
#endif
