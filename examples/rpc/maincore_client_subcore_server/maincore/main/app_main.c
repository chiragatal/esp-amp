/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>

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
static esp_amp_rpmsg_dev_t rpmsg_dev;

static int rpc_srv_add(int a, int b)
{
    esp_amp_rpc_status_t rpc_ret = ESP_AMP_RPC_STATUS_OK;

    /* simple encoder */
    add_params_in_t add_params_in = {
        .a = a,
        .b = b,
    };

    add_params_out_t *add_params_out_buf;
    add_params_out_t add_params_out;
    int add_params_out_buf_size;

    /* first create request */
    esp_amp_rpc_req_handle_t req = esp_amp_rpc_client_create_request(RPC_SERVICE_ADD, &add_params_in, sizeof(add_params_in));
    if (req == NULL) {
        ESP_LOGE(TAG, "Failed to create request");
        rpc_ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    /* execute rpc request */
    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        ESP_LOGI(TAG, "%s send rpc request: ADD(%d, %d)", pcTaskGetName(NULL), a, b);
        rpc_ret = esp_amp_rpc_client_execute_request(req, (void**)(&add_params_out_buf), &add_params_out_buf_size, -1);
    }

    /* get result */
    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        if (add_params_out_buf_size != sizeof(add_params_out_t)) {
            ESP_LOGE(TAG, "Incomplete add_params_out");
            rpc_ret = ESP_AMP_RPC_STATUS_BAD_PACKET;
        }
    }

    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        memcpy(&add_params_out, add_params_out_buf, add_params_out_buf_size);
        ESP_LOGI(TAG, "%s recv rpc response ADD(%d,%d)->%d", pcTaskGetName(NULL), a, b, add_params_out.ret);
    } else {
        ESP_LOGE(TAG, "%s failed to execute rpc call ADD. Err=%d", pcTaskGetName(NULL), rpc_ret);
    }

    /* finally destroy request */
    esp_amp_rpc_client_destroy_request(req);
    return rpc_ret;
}

static int rpc_srv_timeout(int a, int b)
{
    esp_amp_rpc_status_t rpc_ret = ESP_AMP_RPC_STATUS_OK;;

    /* simple encoder */
    timeout_params_in_t timeout_params_in = {
        .a = a,
        .b = b
    };

    timeout_params_out_t *timeout_params_out_buf;
    timeout_params_out_t timeout_params_out;
    int timeout_params_out_buf_size;

    /* first create request */
    esp_amp_rpc_req_handle_t req = esp_amp_rpc_client_create_request(RPC_SERVICE_TIMEOUT, &timeout_params_in, sizeof(timeout_params_in));
    if (req == NULL) {
        ESP_LOGE(TAG, "%s failed to create request", pcTaskGetName(NULL));
        rpc_ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    /* execute rpc request */
    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        ESP_LOGI(TAG, "%s send rpc request: TIMEOUT", pcTaskGetName(NULL));
        rpc_ret = esp_amp_rpc_client_execute_request(req, (void**)(&timeout_params_out_buf), &timeout_params_out_buf_size, 100);
    }

    /* get result */
    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        if (timeout_params_out_buf_size != sizeof(timeout_params_out_t)) {
            ESP_LOGE(TAG, "Incomplete timeout_params_out");
            rpc_ret = ESP_AMP_RPC_STATUS_BAD_PACKET;
        }
    }

    if (rpc_ret == ESP_AMP_RPC_STATUS_OK) {
        memcpy(&timeout_params_out, timeout_params_out_buf, timeout_params_out_buf_size);
        ESP_LOGI(TAG, "%s recv rpc response TIMEOUT", pcTaskGetName(NULL));
    } else {
        ESP_LOGE(TAG, "%s failed to execute rpc call TIMEOUT. Err=%d", pcTaskGetName(NULL), rpc_ret);
    }
    /* finally destroy request */
    esp_amp_rpc_client_destroy_request(req);
    return rpc_ret;
}

static void client(void *args)
{
    int client_id = (int)args;

    for (int i = 0; i < 10; i++) {
        rpc_srv_add(i + client_id * 10000, client_id * 10000 + i + 1);
        vTaskDelay(pdMS_TO_TICKS(50));

        /* to test timeout feature, uncomment the following lines */
        // rpc_srv_timeout(i+client_id*10000, client_id*10000+i+1);
        // vTaskDelay(pdMS_TO_TICKS(50));
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    int ret = 0;

    /* init esp amp component */
    assert(esp_amp_init() == 0);

    /* create endpoint */
    assert(esp_amp_rpmsg_main_init(&rpmsg_dev, 32, 128, false, false) == 0);
    assert(esp_amp_rpc_client_init(&rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_SUB_CORE_SERVER, 5, 2048) == 0);
    esp_amp_rpmsg_intr_enable(&rpmsg_dev);

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    ret = esp_amp_rpc_client_run();
    if (ret == -1) {
        ESP_LOGE(TAG, "Failed to run rpc client");
    }

    if (xTaskCreate(client, "c1", 2048, (void *)1, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task c1");
    }

    if (xTaskCreate(client, "c2", 2048, (void *)2, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task c2");
    }

    if (xTaskCreate(client, "c3", 2048, (void *)3, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task c3");
    }
}
