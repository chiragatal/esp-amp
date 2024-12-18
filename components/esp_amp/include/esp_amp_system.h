/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
