/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_amp.h"

#include "sys_info.h"
#include "event.h"

#define TAG "app_main"

EventGroupHandle_t sub_core_event_group;

void task_wait_for_any(void *arg)
{
    while (1) {
        /* you can use xEventGroupWaitBits() or esp_amp_event_wait_by_id() to wait for events in FreeRTOS */
        uint32_t event_bits = xEventGroupWaitBits(sub_core_event_group, (EVENT_SUBCORE_EVENT_1 | EVENT_SUBCORE_EVENT_2), true, false, 10000);
        // uint32_t event_bits = esp_amp_event_wait_by_id(SYS_INFO_ID_SUBCORE_EVENT, (EVENT_SUBCORE_EVENT_1 | EVENT_SUBCORE_EVENT_2), true, false, 10000);

        if ((event_bits & EVENT_SUBCORE_EVENT_1) == EVENT_SUBCORE_EVENT_1) {
            ESP_LOGI(TAG, "task_wait_for_any: EVENT_SUBCORE_EVENT_1 received");
        } else if ((event_bits & EVENT_SUBCORE_EVENT_2) == EVENT_SUBCORE_EVENT_2) {
            ESP_LOGI(TAG, "task_wait_for_any: EVENT_SUBCORE_EVENT_2 received");
        } else {
            ESP_LOGE(TAG, "task_wait_for_any: timeout");
        }
    }
}

void app_main(void)
{
    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create event */
    int ret;
    ret = esp_amp_event_create(SYS_INFO_ID_MAINCORE_EVENT); /* maincore notify subcore */
    assert(ret == 0);
    ret = esp_amp_event_create(SYS_INFO_ID_SUBCORE_EVENT); /* subcore notify maincore */
    assert(ret == 0);

    /* create event group: enable interrupt handling */
    sub_core_event_group = xEventGroupCreate();
    assert(sub_core_event_group != NULL);

    /* bind event group to event */
    esp_amp_event_table_dump();
    assert(esp_amp_event_bind_handle(SYS_INFO_ID_SUBCORE_EVENT, sub_core_event_group) == 0);
    esp_amp_event_table_dump();

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);
    ESP_LOGI(TAG, "Sub core linked up");

    /* create tasks to wait for events */
    if (xTaskCreate(task_wait_for_any, "w_any", 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task_wait_for_any");
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    /* notify subcore*/
    while (1) {
        ESP_LOGI(TAG, "notify EVENT_MAINCORE_EVENT(%p) to subcore", (void *)EVENT_MAINCORE_EVENT);
        esp_amp_event_notify_by_id(SYS_INFO_ID_MAINCORE_EVENT, EVENT_MAINCORE_EVENT);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
