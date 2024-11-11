/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_amp.h"

#include "sys_info.h"
#include "event.h"
#include "rpc_service.h"

#define TAG "app_main"

/* used by endpoint */
extern esp_amp_rpc_service_t services[3];
static esp_amp_rpmsg_dev_t rpmsg_dev;

void app_main(void)
{

    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create endpoint */
    assert(esp_amp_rpmsg_main_init(&rpmsg_dev, 32, 128, false, false) == 0);
    assert(esp_amp_rpc_server_init(&rpmsg_dev, RPC_SUB_CORE_CLIENT, RPC_MAIN_CORE_SERVER, 5, 2048) == 0);
    esp_amp_rpc_server_add_service(RPC_SERVICE_ADD, rpc_service_add);
    esp_amp_rpc_server_add_service(RPC_SERVICE_SAY_HELLO, rpc_service_say_hello);
    esp_amp_rpc_server_add_service(RPC_SERVICE_TIMEOUT, rpc_service_timeout);
    ESP_LOGI(TAG, "rpc server init successfully");

    esp_amp_rpmsg_intr_enable(&rpmsg_dev);

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    if (esp_amp_rpc_server_run() == -1) {
        ESP_LOGE(TAG, "Failed to run rpc server");
    }

}
