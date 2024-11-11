/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "stdint.h"
#include "stddef.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_amp_rpc.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_log.h"

#define TAG "rpc_server"

#define SERVER_EVENT_STOPPING ( 1 << 1 )
#define SERVER_EVENT_STOPPED ( 1 << 2 )

#define ESP_AMP_RPC_SERVICE_TABLE_LEN CONFIG_ESP_AMP_RPC_SERVICE_TABLE_LEN

typedef struct {
    SemaphoreHandle_t mutex;
    int len;
    esp_amp_rpc_service_t services[ESP_AMP_RPC_SERVICE_TABLE_LEN];
} esp_amp_rpc_service_tbl_t;

typedef enum {
    SERVER_INVALID,
    SERVER_READY,
    SERVER_RUNNING,
    SERVER_STOPPED,
} esp_amp_rpc_server_state_t;

typedef struct {
    uint16_t server_addr;
    uint16_t client_addr;
    int task_priority;
    int stack_size;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    esp_amp_rpc_service_tbl_t service_tbl;
    QueueHandle_t rx_q;
    EventGroupHandle_t event;
    esp_amp_rpc_server_state_t state;
} esp_amp_rpc_server_t;

static esp_amp_rpc_server_t esp_amp_rpc_server;

static int esp_amp_rpc_server_isr(void *pkt_in_buf, uint16_t size, uint16_t src_addr, void* rx_cb_data);

esp_amp_rpc_status_t esp_amp_rpc_server_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr, int task_priority, int stack_size)
{
    if (!rpmsg_dev) {
        ESP_AMP_LOGE(TAG, "Invalid rpmsg dev");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    if (esp_amp_rpc_server.state == SERVER_READY) {
        ESP_AMP_LOGE(TAG, "RPC server aleady init");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_server.task_priority = task_priority <= 0 ? 5 : task_priority;
    esp_amp_rpc_server.stack_size = stack_size <= 0 ? 2048 : stack_size;

    esp_amp_rpc_server.rpmsg_dev = rpmsg_dev;
    esp_amp_rpc_server.client_addr = client_addr;
    esp_amp_rpc_server.server_addr = server_addr;

    /* register endpoint */
    if (esp_amp_rpmsg_create_ept(esp_amp_rpc_server.rpmsg_dev, server_addr, esp_amp_rpc_server_isr, NULL, &esp_amp_rpc_server.rpmsg_ept) == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create ept");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    /* create queue */
    esp_amp_rpc_server.rx_q = xQueueCreate(ESP_AMP_RPC_MAX_PENDING_REQ, sizeof(esp_amp_rpc_pkt_t *));
    if (esp_amp_rpc_server.rx_q == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create rx_q");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    /* init service table */
    esp_amp_rpc_server.service_tbl.mutex = xSemaphoreCreateRecursiveMutex();
    if (esp_amp_rpc_server.service_tbl.mutex == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create service lock");
        return ESP_AMP_RPC_STATUS_FAILED;
    }
    esp_amp_rpc_server.service_tbl.len = 0;

    /* init event group */
    esp_amp_rpc_server.event = xEventGroupCreate();
    if (esp_amp_rpc_server.event == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create event group");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_server.state = SERVER_READY;
    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t esp_amp_rpc_server_add_service(esp_amp_rpc_service_id_t srv_id, esp_amp_rpc_service_func_t srv_func)
{
    esp_amp_rpc_status_t ret = ESP_AMP_RPC_STATUS_OK;
    xSemaphoreTakeRecursive(esp_amp_rpc_server.service_tbl.mutex, portMAX_DELAY);

    if (srv_func == NULL) {
        ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    int next_idx = esp_amp_rpc_server.service_tbl.len;
    if (next_idx == ESP_AMP_RPC_SERVICE_TABLE_LEN) {
        ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    if (ret != ESP_AMP_RPC_STATUS_FAILED) {
        /* duplicated id checking: if a previous handler has the same srv_id, replace it with the new one */
        for (int i = 0; i < next_idx; i++) {
            if (esp_amp_rpc_server.service_tbl.services[i].id == srv_id) {
                next_idx = i;
                break;
            }
        }

        esp_amp_rpc_server.service_tbl.services[next_idx].handler = srv_func;
        esp_amp_rpc_server.service_tbl.services[next_idx].id = srv_id;

        /* if a new service is added to service table, increase the service table length */
        if (next_idx == esp_amp_rpc_server.service_tbl.len) {
            esp_amp_rpc_server.service_tbl.len++;
        }
    }

    xSemaphoreGiveRecursive(esp_amp_rpc_server.service_tbl.mutex);

    ESP_AMP_LOGD(TAG, "added srv(%u, %p) to tbl[%d]", srv_id, srv_func, next_idx);
    return ret;
}

esp_amp_rpc_status_t esp_amp_rpc_server_stop(void)
{
    if (esp_amp_rpc_server.state == SERVER_STOPPED) {
        return ESP_AMP_RPC_STATUS_OK;
    }

    if (esp_amp_rpc_server.state != SERVER_RUNNING) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_status_t ret = ESP_AMP_RPC_STATUS_OK;
    xEventGroupSetBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPING);
    EventBits_t event = xEventGroupWaitBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPED, false, false, pdMS_TO_TICKS(1000));
    if (!(event & SERVER_EVENT_STOPPED)) {
        ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    if (ret == ESP_AMP_RPC_STATUS_OK) {
        esp_amp_rpc_server.state = SERVER_STOPPED;
    }

    xEventGroupClearBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPED);
    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t esp_amp_rpc_server_deinit(void)
{
    esp_amp_rpc_status_t ret = ESP_AMP_RPC_STATUS_OK;
    if (esp_amp_rpc_server.state == SERVER_RUNNING) {
        ret = esp_amp_rpc_server_stop();
    }

    if (ret == ESP_AMP_RPC_STATUS_OK) {
        if (esp_amp_rpc_server.rpmsg_dev) {
            esp_amp_rpmsg_del_ept(esp_amp_rpc_server.rpmsg_dev, esp_amp_rpc_server.server_addr);
            esp_amp_rpc_server.rpmsg_dev = NULL;
        }
        if (esp_amp_rpc_server.event) {
            vEventGroupDelete(esp_amp_rpc_server.event);
            esp_amp_rpc_server.event = NULL;
        }
        if (esp_amp_rpc_server.rx_q) {
            vQueueDelete(esp_amp_rpc_server.rx_q);
            esp_amp_rpc_server.rx_q = NULL;
        }
        if (esp_amp_rpc_server.service_tbl.mutex) {
            vSemaphoreDelete(esp_amp_rpc_server.service_tbl.mutex);
            esp_amp_rpc_server.service_tbl.mutex = NULL;
        }
    }

    esp_amp_rpc_server.state = SERVER_INVALID;
    return ret;
}

static void esp_amp_rpc_server_task(void *args)
{
    (void) args;
    int ret = 0;
    esp_amp_rpc_pkt_t *pkt_in = NULL;
    esp_amp_rpc_pkt_t *pkt_out = NULL;
    uint32_t rpmsg_len = esp_amp_rpmsg_get_max_size(esp_amp_rpc_server.rpmsg_dev);

    while (true) {
        EventBits_t event = xEventGroupWaitBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPING, false, false, 0);
        if (event & SERVER_EVENT_STOPPING) {
            /* server stop as user required */
            xEventGroupClearBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPING);
            break;
        }

        /* recv from isr */
        if (xQueueReceive(esp_amp_rpc_server.rx_q, &pkt_in, pdMS_TO_TICKS(500)) != pdTRUE) {
            ret = -1;
        }

        /* alloc tx_buf (pkt_out) */
        if (ret != -1) {
            pkt_out = (esp_amp_rpc_pkt_t *)esp_amp_rpmsg_create_msg(esp_amp_rpc_server.rpmsg_dev, rpmsg_len, ESP_AMP_RPMSG_DATA_DEFAULT);
            if (pkt_out == NULL) {
                ESP_AMP_LOGE(TAG, "Failed to alloc tx buf for pkt_out");
                ret = -1;
            }
        }

        /* if buffer out also ready */
        if (ret != -1) {
            ESP_AMP_LOGD(TAG, "pkt_in at %p, pkt_out at %p", pkt_in, pkt_out);
            /* copy from pkt_in to pkt_out */
            memcpy(pkt_out, pkt_in, sizeof(esp_amp_rpc_pkt_t));
            pkt_out->params_len = rpmsg_len;
            pkt_out->status = ESP_AMP_RPC_STATUS_NO_SERVICE;

            ESP_AMP_LOGD(TAG, "Executing(req_id:%u, srv_id:%u, status:%u, param_len:%u)", pkt_in->req_id, pkt_in->service_id, pkt_in->status, pkt_in->params_len);
            /* execute service */
            esp_amp_rpc_service_func_t service_handler = NULL;

            xSemaphoreTakeRecursive(esp_amp_rpc_server.service_tbl.mutex, portMAX_DELAY);
            for (int i = 0; i < esp_amp_rpc_server.service_tbl.len; i++) {
                if (esp_amp_rpc_server.service_tbl.services[i].handler == NULL) {
                    continue;
                }
                if (esp_amp_rpc_server.service_tbl.services[i].id == pkt_in->service_id) {
                    service_handler = esp_amp_rpc_server.service_tbl.services[i].handler;
                    break;
                }
            }
            xSemaphoreGiveRecursive(esp_amp_rpc_server.service_tbl.mutex);

            if (service_handler != NULL) {
                if (service_handler((void **)pkt_in->params, pkt_in->params_len, (void **)pkt_out->params, &pkt_out->params_len) == 0) {
                    pkt_out->status = ESP_AMP_RPC_STATUS_OK;
                } else {
                    pkt_out->status = ESP_AMP_RPC_STATUS_EXEC_FAILED;
                }
            }

            /* release rx buffer (pkt_in) */
            esp_amp_rpmsg_destroy(esp_amp_rpc_server.rpmsg_dev, pkt_in);

            if (pkt_out->status == ESP_AMP_RPC_STATUS_OK) {
                ESP_AMP_LOGD(TAG, "Execd req(%u, %u)", pkt_out->req_id, pkt_out->service_id);
            }

            if (pkt_out->status == ESP_AMP_RPC_STATUS_NO_SERVICE) {
                ESP_AMP_LOGE(TAG, "Invalid srv id req(%u, %u)", pkt_out->req_id, pkt_out->service_id);
            }

            if (pkt_out->status == ESP_AMP_RPC_STATUS_EXEC_FAILED) {
                ESP_AMP_LOGE(TAG, "Failed to execute req(%u, %u)", pkt_out->req_id, pkt_out->service_id);
            }
        }

        /* as long as decode successfully, send back the result */
        if (ret != -1) {
            ESP_AMP_LOGD(TAG, "sending rsp(%u)", pkt_out->req_id);
            esp_amp_rpmsg_send_nocopy(esp_amp_rpc_server.rpmsg_dev, &esp_amp_rpc_server.rpmsg_ept, esp_amp_rpc_server.client_addr,
                                      pkt_out, pkt_out->params_len + sizeof(esp_amp_rpc_pkt_t));
        }
    }

    ESP_AMP_LOGD(TAG, "%s(): server task stopped", __func__);
    xEventGroupSetBits(esp_amp_rpc_server.event, SERVER_EVENT_STOPPED);
    vTaskDelete(NULL);
}

esp_amp_rpc_status_t esp_amp_rpc_server_run(void)
{
    esp_amp_rpc_status_t ret;

    switch (esp_amp_rpc_server.state) {
    case SERVER_READY:
    case SERVER_STOPPED:
        if (xTaskCreate(esp_amp_rpc_server_task, "rpc_server", esp_amp_rpc_server.stack_size, NULL, esp_amp_rpc_server.task_priority, NULL) != pdPASS) {
            ESP_AMP_LOGE(TAG, "Failed to create rpc server task");
            ret = ESP_AMP_RPC_STATUS_FAILED;
        } else {
            esp_amp_rpc_server.state = SERVER_RUNNING;
            ret = ESP_AMP_RPC_STATUS_OK;
        }
        break;
    case SERVER_RUNNING:
        ret = ESP_AMP_RPC_STATUS_OK;
        break;
    case SERVER_INVALID:
        ret = ESP_AMP_RPC_STATUS_FAILED;
        break;
    default:
        ret = ESP_AMP_RPC_STATUS_FAILED;
        break;
    }

    return ret;
}


static int esp_amp_rpc_server_isr(void *pkt_in_buf, uint16_t size, uint16_t src_addr, void* rx_cb_data)
{
    if (size < sizeof(esp_amp_rpc_pkt_t)) {
        ESP_AMP_DRAM_LOGE(TAG, "incomplete rx buf");
        esp_amp_rpmsg_destroy_from_isr(esp_amp_rpc_server.rpmsg_dev, pkt_in_buf);
        return 0;
    }

    BaseType_t need_yield;
    /* try to send to server */
    if (xQueueSendFromISR(esp_amp_rpc_server.rx_q, &pkt_in_buf, &need_yield) != pdTRUE) {
        esp_amp_rpmsg_destroy_from_isr(esp_amp_rpc_server.rpmsg_dev, pkt_in_buf);
    }

    portYIELD_FROM_ISR(need_yield);
    return 0;
}
