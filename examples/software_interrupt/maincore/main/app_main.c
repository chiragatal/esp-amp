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

static const DRAM_ATTR char TAG[] = "sw_intr";

static IRAM_ATTR int sw_intr_id0_handler_1(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id0_handler_1() called");
    return 0;
}

static IRAM_ATTR int sw_intr_id0_handler_2(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id0_handler_2() called");
    return 0;
}

static IRAM_ATTR int sw_intr_id1_handler_1(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id1_handler_1() called\n");
    return 0;
}

static IRAM_ATTR int sw_intr_id1_handler_2(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id1_handler_2() called\n");
    return 0;
}

static IRAM_ATTR int sw_intr_id2_handler_1(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id2_handler_1() called\n");
    return 0;
}

static IRAM_ATTR int sw_intr_id2_handler_2(void *arg)
{
    ESP_DRAM_LOGI(TAG, "sw_intr_id2_handler_2() called\n");
    return 0;
}

void app_main(void)
{
    assert(esp_amp_init() == 0);

    esp_amp_sw_intr_handler_dump();

    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler_2, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler_2, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler_2, NULL) == 0);

    esp_amp_sw_intr_handler_dump();

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);
    ESP_LOGI(TAG, "subcore linked up");

    while (1) {
        ESP_LOGI(TAG, "running...");
        printf("trigger intr 0...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        printf("trigger intr 1...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        printf("trigger intr 2...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_2);
        vTaskDelay(pdMS_TO_TICKS(1000));

        printf("trigger intr 3...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_3);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
