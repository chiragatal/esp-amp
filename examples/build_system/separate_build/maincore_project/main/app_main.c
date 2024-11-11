/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_amp.h"

#include "event.h"

#define TAG "app_main"

void app_main(void)
{
    assert(esp_amp_init() == 0);

    /* Load firmware & start subcore */
    ESP_LOGI(TAG, "Loading subcore firmware from partition");
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* Wait for subcore ready */
    uint32_t bit_mask = esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 5000);
    assert((bit_mask & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);
    ESP_LOGI(TAG, "Subcore started successfully");
}
