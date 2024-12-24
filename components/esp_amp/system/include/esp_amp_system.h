/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#if IS_MAIN_CORE
#include "stdbool.h"
#include "esp_err.h"
#include "esp_partition.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if IS_MAIN_CORE
/**
 * Load the program binary from partition
 *
 * @param sub_partition partition handle to partition where subcore firmware resides
 *
 * @retval ESP_OK on success
 * @retval ESP_FAIL if load fail
 */
esp_err_t esp_amp_load_sub_from_partition(const esp_partition_t* sub_partition);

/**
 * Load the program binary from the memory pointer
 *
 * @param sub_bin pointer to the subcore binary
 *
 * @retval ESP_OK on success
 * @retval ESP_FAIL if load fail
 */
esp_err_t esp_amp_load_sub(const void* sub_bin);

/**
 * Start subcore
 *
 * @retval 0 start subcore successfully
 * @retval -1 Failed to start subcore
 */
int esp_amp_start_subcore(void);

/**
 * Stop subcore
 */
void esp_amp_stop_subcore(void);

/**
 * Initialize esp amp panic
 *
 * @retval 0 if successful
 * @retval -1 if failed
 */
int esp_amp_system_panic_init(void);

/**
 * @brief Check if subcore panic
 *
 * @retval true if subcore panic
 * @retval false if not
 */
bool esp_amp_subcore_panic(void);

/**
 * @brief default handler for subcore panic
 */
void esp_amp_subcore_panic_handler_default(void);
#endif

/**
 * Initialize esp amp system
 *
 * @retval 0 if successful
 * @retval -1 if failed
 */
int esp_amp_system_init(void);

#ifdef __cplusplus
}
#endif
