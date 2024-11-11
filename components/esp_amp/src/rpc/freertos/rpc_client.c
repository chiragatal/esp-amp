/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "stdint.h"
#include "string.h"
#include "limits.h"
#include "stdatomic.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_amp_log.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_rpc.h"

#define CLIENT_EVENT_STOPPING ( 1 << 1 )
#define CLIENT_EVENT_RECV_STOPPED ( 1 << 2 )
#define CLIENT_EVENT_SEND_STOPPED (1 << 3)

#define ESP_AMP_RPC_INVALID_REQ_ID 0

#define TAG "rpc_client"

typedef struct {
    uint16_t req_id; /* req_id & status still needed since pkt can be freed somewhere asynchronously */
    uint16_t service_id;
    uint16_t status; /* status can be updated by timer */
    QueueHandle_t app_rsp_q;
    esp_amp_rpc_pkt_t *pkt;
} esp_amp_rpc_pending_req_t;

typedef struct {
    esp_amp_rpc_pending_req_t *reqs[ESP_AMP_RPC_MAX_PENDING_REQ];
    SemaphoreHandle_t mutex; /* lock for pending list */
} esp_amp_rpc_pending_list_t;

typedef struct {
    uint16_t val;
    SemaphoreHandle_t mutex; /* lock for req id */
} esp_amp_rpc_req_id_t;

typedef enum {
    CLIENT_INVALID,
    CLIENT_READY,
    CLIENT_RUNNING,
    CLIENT_STOPPED,
} esp_amp_rpc_client_state_t;

typedef struct {
    uint16_t server_addr;
    uint16_t client_addr;
    int task_priority;
    int stack_size;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    esp_amp_rpc_pending_list_t pending_list;
    esp_amp_rpc_req_id_t req_id;
    QueueHandle_t app_req_q; /* queue for req pkt from app layer */
    QueueHandle_t rx_q; /* queue for rx pkt from transport layer */
    EventGroupHandle_t event;
    esp_amp_rpc_client_state_t state;
} esp_amp_rpc_client_t;

static esp_amp_rpc_client_t esp_amp_rpc_client;


__attribute__((__unused__)) static void esp_amp_rpc_pending_list_squeeze(void)
{
    xSemaphoreTakeRecursive(esp_amp_rpc_client.pending_list.mutex, portMAX_DELAY);

    int next_avail = -1;
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        /* find empty slot */
        if (esp_amp_rpc_client.pending_list.reqs[i] == NULL) {
            if (next_avail == -1) {
                next_avail = i;    /* find an empty slot and point to it by next_avail */
            }
        }
        /* squeeze the filled slots */
        else {
            if (next_avail != -1) {
                /* copy the filled slot to the empty one */
                esp_amp_rpc_client.pending_list.reqs[next_avail] = esp_amp_rpc_client.pending_list.reqs[i];
                /* set reqs[i] as empty slot */
                esp_amp_rpc_client.pending_list.reqs[i] = NULL;
                /* proceed to next slot */
                next_avail++;
            }
        }
    }

    xSemaphoreGiveRecursive(esp_amp_rpc_client.pending_list.mutex);
}

static int esp_amp_rpc_pending_list_push(esp_amp_rpc_pending_req_t *req)
{
    int ret = -1;
    xSemaphoreTakeRecursive(esp_amp_rpc_client.pending_list.mutex, portMAX_DELAY);

    // esp_amp_rpc_pending_list_squeeze();
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        if (esp_amp_rpc_client.pending_list.reqs[i] == NULL) {
            /* copy the filled slot to the empty one */
            esp_amp_rpc_client.pending_list.reqs[i] = req;
            ret = 0;
            break;
        }
    }

    xSemaphoreGiveRecursive(esp_amp_rpc_client.pending_list.mutex);
    return ret;
}

static int esp_amp_rpc_pending_list_pop(uint32_t req_id, esp_amp_rpc_pending_req_t **req_out)
{
    xSemaphoreTakeRecursive(esp_amp_rpc_client.pending_list.mutex, portMAX_DELAY);

    int ret = -1;
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        esp_amp_rpc_pending_req_t *req = esp_amp_rpc_client.pending_list.reqs[i];
        if (req && req->req_id == req_id) {
            /* copy to req_out */
            if (req_out != NULL) {
                *req_out = esp_amp_rpc_client.pending_list.reqs[i];
            }
            /* set to empty slot */
            esp_amp_rpc_client.pending_list.reqs[i] = NULL;
            ret = 0;
            break;
        }
    }

    xSemaphoreGiveRecursive(esp_amp_rpc_client.pending_list.mutex);
    return ret;
}

static int esp_amp_rpc_pending_list_peek(uint32_t req_id, esp_amp_rpc_pending_req_t **req_out)
{
    xSemaphoreTakeRecursive(esp_amp_rpc_client.pending_list.mutex, portMAX_DELAY);

    int ret = -1;
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        esp_amp_rpc_pending_req_t *req = esp_amp_rpc_client.pending_list.reqs[i];
        if (req && req->req_id == req_id) {
            /* copy to req_out */
            if (req_out != NULL) {
                *req_out = esp_amp_rpc_client.pending_list.reqs[i];
            }
            ret = 0;
            break;
        }
    }

    xSemaphoreGiveRecursive(esp_amp_rpc_client.pending_list.mutex);
    return ret;
}

static void esp_amp_rpc_pending_list_dump(void)
{
    int list[ESP_AMP_RPC_MAX_PENDING_REQ];
    ESP_AMP_LOGD(TAG, "=== pending list ===");
    xSemaphoreTakeRecursive(esp_amp_rpc_client.pending_list.mutex, portMAX_DELAY);

    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        if (esp_amp_rpc_client.pending_list.reqs[i] == NULL) {
            list[i] = 0;
        } else {
            list[i] = esp_amp_rpc_client.pending_list.reqs[i]->req_id;
        }
    }
    xSemaphoreGiveRecursive(esp_amp_rpc_client.pending_list.mutex);

    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        ESP_AMP_LOGD(TAG, "%d\t%d", i, list[i]);
    }
    ESP_AMP_LOGD(TAG, "====================");
}

static int esp_amp_rpc_client_isr(void *pkt_in_buf, uint16_t pkt_in_size, uint16_t src_addr, void* rx_cb_data)
{
    BaseType_t need_yield = 0;

    if (pkt_in_size < sizeof(esp_amp_rpc_pkt_t)) {
        ESP_AMP_DRAM_LOGE(TAG, "incomplete pkt");
        esp_amp_rpmsg_destroy_from_isr(esp_amp_rpc_client.rpmsg_dev, pkt_in_buf);
        return 0;
    }

    esp_amp_rpc_pkt_t *pkt_in = (esp_amp_rpc_pkt_t *)pkt_in_buf;
    if (xQueueSendFromISR(esp_amp_rpc_client.rx_q, &pkt_in, &need_yield) != pdTRUE) {
        esp_amp_rpmsg_destroy_from_isr(esp_amp_rpc_client.rpmsg_dev, pkt_in_buf);
        ESP_AMP_DRAM_LOGE(TAG, "rx_q full. drop pkt(%u)", pkt_in->req_id);
    }

    portYIELD_FROM_ISR(need_yield);
    return 0;
}

esp_amp_rpc_status_t esp_amp_rpc_client_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr, int task_priority, int stack_size)
{
    if (!rpmsg_dev) {
        ESP_AMP_LOGE(TAG, "Invalid rpmsg_dev");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    if (esp_amp_rpc_client.state >= CLIENT_READY) {
        ESP_AMP_LOGE(TAG, "RPC client aleady initialized");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_client.task_priority = task_priority <= 0 ? 5 : task_priority;
    esp_amp_rpc_client.stack_size = stack_size <= 0 ? 2048 : stack_size;

    esp_amp_rpc_client.rpmsg_dev = rpmsg_dev;
    esp_amp_rpc_client.client_addr = client_addr;
    esp_amp_rpc_client.server_addr = server_addr;

    /* init rpmsg endpoint */
    if (esp_amp_rpmsg_create_ept(esp_amp_rpc_client.rpmsg_dev, client_addr, esp_amp_rpc_client_isr, NULL, &esp_amp_rpc_client.rpmsg_ept) == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create ept");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    /* init the pending list */
    esp_amp_rpc_client.pending_list.mutex = xSemaphoreCreateRecursiveMutex();
    if (!esp_amp_rpc_client.pending_list.mutex) {
        ESP_AMP_LOGE(TAG, "Failed to create pending list mutex");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        esp_amp_rpc_client.pending_list.reqs[i] = NULL;
    }

    /* init the req id */
    esp_amp_rpc_client.req_id.mutex = xSemaphoreCreateRecursiveMutex();
    if (!esp_amp_rpc_client.req_id.mutex) {
        ESP_AMP_LOGE(TAG, "Failed to create req id mutex");
        return ESP_AMP_RPC_STATUS_FAILED;
    }
    esp_amp_rpc_client.req_id.val = 1;

    /* request queue to accept pkt from user app */
    esp_amp_rpc_client.app_req_q = xQueueCreate(ESP_AMP_RPC_MAX_PENDING_REQ, sizeof(esp_amp_rpc_pending_req_t *));
    if (!esp_amp_rpc_client.app_req_q) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    /* response queue to recv pkt from server */
    esp_amp_rpc_client.rx_q = xQueueCreate(ESP_AMP_RPC_MAX_PENDING_REQ, sizeof(esp_amp_rpc_pkt_t *));
    if (!esp_amp_rpc_client.rx_q) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    /* create event */
    esp_amp_rpc_client.event = xEventGroupCreate();
    if (!esp_amp_rpc_client.event) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_client.state = CLIENT_READY;
    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t esp_amp_rpc_client_stop(void)
{
    if (esp_amp_rpc_client.state == CLIENT_STOPPED) {
        return ESP_AMP_RPC_STATUS_OK;
    }

    if (esp_amp_rpc_client.state != CLIENT_RUNNING) {
        ESP_AMP_LOGE(TAG, "Trying to stop a client not running");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_status_t ret = ESP_AMP_RPC_STATUS_OK;
    xEventGroupSetBits(esp_amp_rpc_client.event, CLIENT_EVENT_STOPPING);
    EventBits_t event = xEventGroupWaitBits(esp_amp_rpc_client.event, (CLIENT_EVENT_RECV_STOPPED | CLIENT_EVENT_SEND_STOPPED), false, true, portMAX_DELAY);

    if ((event & (CLIENT_EVENT_SEND_STOPPED | CLIENT_EVENT_RECV_STOPPED)) != (CLIENT_EVENT_SEND_STOPPED | CLIENT_EVENT_RECV_STOPPED)) {
        ret = ESP_AMP_RPC_STATUS_FAILED;
    }

    if (ret == ESP_AMP_RPC_STATUS_OK) {
        esp_amp_rpc_client.state = CLIENT_STOPPED;
    }

    xEventGroupClearBits(esp_amp_rpc_client.event, (CLIENT_EVENT_STOPPING | CLIENT_EVENT_SEND_STOPPED | CLIENT_EVENT_RECV_STOPPED));
    return ret;
}

esp_amp_rpc_status_t esp_amp_rpc_client_deinit(void)
{
    esp_amp_rpc_status_t ret = ESP_AMP_RPC_STATUS_OK;
    if (esp_amp_rpc_client.state == CLIENT_RUNNING) {
        ret = esp_amp_rpc_client_stop();
    }

    if (ret == ESP_AMP_RPC_STATUS_OK) {
        if (esp_amp_rpc_client.rpmsg_dev) {
            esp_amp_rpmsg_del_ept(esp_amp_rpc_client.rpmsg_dev, esp_amp_rpc_client.client_addr);
            esp_amp_rpc_client.rpmsg_dev = NULL;
        }
        if (esp_amp_rpc_client.pending_list.mutex) {
            vSemaphoreDelete(esp_amp_rpc_client.pending_list.mutex);
            esp_amp_rpc_client.pending_list.mutex = NULL;
        }
        if (esp_amp_rpc_client.rx_q) {
            vQueueDelete(esp_amp_rpc_client.rx_q);
            esp_amp_rpc_client.rx_q = NULL;
        }
        if (esp_amp_rpc_client.app_req_q) {
            vQueueDelete(esp_amp_rpc_client.app_req_q);
            esp_amp_rpc_client.app_req_q = NULL;
        }
        if (esp_amp_rpc_client.req_id.mutex) {
            vSemaphoreDelete(esp_amp_rpc_client.req_id.mutex);
            esp_amp_rpc_client.req_id.mutex = NULL;
        }
        if (esp_amp_rpc_client.event) {
            vEventGroupDelete(esp_amp_rpc_client.event);
            esp_amp_rpc_client.event = NULL;
        }
    }

    esp_amp_rpc_client.state = CLIENT_INVALID;
    return ESP_AMP_RPC_STATUS_OK;
}

static uint16_t esp_amp_rpc_get_req_id(void)
{
    uint16_t req_id = 0;
    xSemaphoreTakeRecursive(esp_amp_rpc_client.req_id.mutex, portMAX_DELAY);
    if (esp_amp_rpc_client.req_id.val == SHRT_MAX) { /* wrap around */
        esp_amp_rpc_client.req_id.val = 1;
    }
    req_id = esp_amp_rpc_client.req_id.val;
    esp_amp_rpc_client.req_id.val += 1;
    xSemaphoreGiveRecursive(esp_amp_rpc_client.req_id.mutex);
    return req_id;
}

esp_amp_rpc_req_handle_t esp_amp_rpc_client_create_request(uint16_t service_id, void *params, uint16_t params_len)
{
    QueueHandle_t rsp_q = xQueueCreate(1, sizeof(esp_amp_rpc_pkt_t *));
    if (rsp_q == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create rsp_q");
        return NULL;
    }

    /* construct the pending req */
    esp_amp_rpc_pending_req_t *pending_req = malloc(sizeof(esp_amp_rpc_pending_req_t));
    if (pending_req == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create pending req");
        return NULL;
    }

    pending_req->status = ESP_AMP_RPC_STATUS_PENDING;
    pending_req->app_rsp_q = rsp_q;
    pending_req->req_id = esp_amp_rpc_get_req_id();
    pending_req->service_id = service_id;

    if (esp_amp_rpc_pending_list_push(pending_req) == -1) {
        ESP_AMP_LOGE(TAG, "Failed to push to pending list");
        free(pending_req);
        return NULL;
    }

    esp_amp_rpc_pending_list_dump();

    esp_amp_rpc_pkt_t *pkt_out = (esp_amp_rpc_pkt_t *)esp_amp_rpmsg_create_msg(esp_amp_rpc_client.rpmsg_dev, sizeof(esp_amp_rpc_pkt_t) + params_len, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (pkt_out == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to alloc msg buf");
        free(pending_req);
        return NULL;
    }

    memcpy(pkt_out->params, params, params_len);
    pkt_out->params_len = params_len;
    pkt_out->req_id = pending_req->req_id;
    pkt_out->service_id = service_id;
    pkt_out->status = ESP_AMP_RPC_STATUS_PENDING;

    /* attach to pending_req */
    pending_req->pkt = pkt_out;
    return pending_req;
}

esp_amp_rpc_status_t esp_amp_rpc_client_execute_request(esp_amp_rpc_req_handle_t req, void **param_out, int *param_out_len, uint32_t timeout_ms)
{
    esp_amp_rpc_pending_req_t *pending_req = (esp_amp_rpc_pending_req_t *)req;

    if (!pending_req || pending_req->req_id == ESP_AMP_RPC_INVALID_REQ_ID || !pending_req->pkt) {
        ESP_AMP_LOGE(TAG, "Invalid req");
        return ESP_AMP_RPC_STATUS_INVALID_ARG;
    }

    if (pending_req->app_rsp_q == NULL) {
        ESP_AMP_LOGE(TAG, "Invalid app_rsp_q");
        return ESP_AMP_RPC_STATUS_INVALID_ARG;
    }

    /* send rpc request */
    ESP_AMP_LOGD(TAG, "send pending_req[%p](%u, %u) to send task", pending_req, pending_req->req_id, pending_req->service_id);
    xQueueSend(esp_amp_rpc_client.app_req_q, &pending_req, portMAX_DELAY);

    /* recv rpc response */
    uint32_t timeout_tick = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ms == -1) {
        timeout_tick = portMAX_DELAY;
    }
    if (xQueueReceive(pending_req->app_rsp_q, &pending_req->pkt, timeout_tick) != pdTRUE) {
        ESP_AMP_LOGE(TAG, "Timeout req(%u, %u)", pending_req->req_id, pending_req->service_id);
        *param_out = NULL;
        *param_out_len = 0;
        return ESP_AMP_RPC_STATUS_TIMEOUT;
    }

    /* if req_id not match */
    if (pending_req->pkt->req_id != pending_req->req_id) {
        ESP_AMP_LOGE(TAG, "Unmatched incoming rsp(%u) with req(%u, %u)", pending_req->pkt->req_id, pending_req->req_id, pending_req->service_id);
        return ESP_AMP_RPC_STATUS_EXEC_FAILED;
    }

    /* forward to params, no copy */
    *param_out = pending_req->pkt->params;
    *param_out_len = pending_req->pkt->params_len;
    return pending_req->pkt->status;
}

void esp_amp_rpc_client_destroy_request(esp_amp_rpc_req_handle_t req)
{
    esp_amp_rpc_pending_req_t *pending_req = (esp_amp_rpc_pending_req_t *)req;
    if (!pending_req) {
        ESP_AMP_LOGE(TAG, "Invalid req");
        return;
    }

    esp_amp_rpmsg_destroy(esp_amp_rpc_client.rpmsg_dev, pending_req->pkt);
    esp_amp_rpc_pending_list_pop(pending_req->req_id, NULL);
    vQueueDelete(pending_req->app_rsp_q);
    free(pending_req);
}

static void esp_amp_rpc_client_send_once(void)
{
    esp_amp_rpc_pending_req_t *pending_req;

    if (xQueueReceive(esp_amp_rpc_client.app_req_q, &pending_req, pdMS_TO_TICKS(500)) == pdTRUE) {
        ESP_AMP_LOGD(TAG, "Executing(req_id:%u, srv_id:%u, param(%u):%p",
                     pending_req->pkt->req_id, pending_req->pkt->service_id,
                     pending_req->pkt->params_len, pending_req->pkt->params);
        ESP_AMP_LOGD(TAG, "client(%u) send req(pkt=%p, req_id=%u) to server(%u)", esp_amp_rpc_client.rpmsg_ept.addr,
                     pending_req->pkt, pending_req->pkt->req_id, esp_amp_rpc_client.server_addr);
        ESP_AMP_LOG_BUFFER_HEXDUMP(TAG, pending_req->pkt, pending_req->pkt->params_len + sizeof(esp_amp_rpc_pkt_t), ESP_AMP_LOG_DEBUG);

        esp_amp_rpmsg_send_nocopy(esp_amp_rpc_client.rpmsg_dev, &esp_amp_rpc_client.rpmsg_ept, esp_amp_rpc_client.server_addr,
                                  pending_req->pkt, pending_req->pkt->params_len + sizeof(esp_amp_rpc_pkt_t));
    }
}

void esp_amp_rpc_client_recv_once(void)
{
    esp_amp_rpc_pkt_t *pkt_in;

    if (xQueueReceive(esp_amp_rpc_client.rx_q, &pkt_in, pdMS_TO_TICKS(500)) == pdTRUE) {
        esp_amp_rpc_pending_req_t *pending_req;

        /* timeout req will be removed from pending_list */
        if (esp_amp_rpc_pending_list_peek(pkt_in->req_id, &pending_req) == -1) {
            esp_amp_rpmsg_destroy(esp_amp_rpc_client.rpmsg_dev, pkt_in);
            return;
        }

        /* app_rsp_q will also be deleted */
        if (!pending_req->app_rsp_q) {
            esp_amp_rpmsg_destroy(esp_amp_rpc_client.rpmsg_dev, pkt_in);
            return;
        }

        /* send pkt_in to app_rsp_q without copy */
        if (xQueueSend(pending_req->app_rsp_q, &pkt_in, 0) != pdTRUE) {
            ESP_AMP_LOGE(TAG, "Failed to send pkt to app_rsp_q");
            esp_amp_rpmsg_destroy(esp_amp_rpc_client.rpmsg_dev, pkt_in);
        }
    }
}

static void esp_amp_rpc_client_send_task(void *args)
{
    while (true) {
        EventBits_t event = xEventGroupWaitBits(esp_amp_rpc_client.event, CLIENT_EVENT_STOPPING, false, false, 0);
        if (event & CLIENT_EVENT_STOPPING) {
            /* stop client as user requested */
            break;
        }
        esp_amp_rpc_client_send_once();
    }
    xEventGroupSetBits(esp_amp_rpc_client.event, CLIENT_EVENT_SEND_STOPPED);
    vTaskDelete(NULL);
}

static void esp_amp_rpc_client_recv_task(void *args)
{
    while (true) {
        EventBits_t event = xEventGroupWaitBits(esp_amp_rpc_client.event, CLIENT_EVENT_STOPPING, false, false, 0);
        if (event & CLIENT_EVENT_STOPPING) {
            /* stop client as user requested */
            break;
        }
        esp_amp_rpc_client_recv_once();
    }
    xEventGroupSetBits(esp_amp_rpc_client.event, CLIENT_EVENT_RECV_STOPPED);
    vTaskDelete(NULL);
}

esp_amp_rpc_status_t esp_amp_rpc_client_run(void)
{
    esp_amp_rpc_status_t ret;

    switch (esp_amp_rpc_client.state) {
    case CLIENT_RUNNING:
        ret = ESP_AMP_RPC_STATUS_OK;
        break;
    case CLIENT_READY:
    case CLIENT_STOPPED:
        if (xTaskCreate(esp_amp_rpc_client_send_task, "rpc_send", esp_amp_rpc_client.stack_size, NULL, esp_amp_rpc_client.task_priority, NULL) != pdPASS) {
            ESP_AMP_LOGE(TAG, "Failed to create rpc_send_task");
            ret = ESP_AMP_RPC_STATUS_FAILED;
            break;
        }
        if (xTaskCreate(esp_amp_rpc_client_recv_task, "rpc_recv", esp_amp_rpc_client.stack_size, NULL, esp_amp_rpc_client.task_priority, NULL) != pdPASS) {
            ESP_AMP_LOGE(TAG, "Failed to create rpc_recv_task");
            ret = ESP_AMP_RPC_STATUS_FAILED;
            break;
        }
        ret = ESP_AMP_RPC_STATUS_OK;
        esp_amp_rpc_client.state = CLIENT_RUNNING;
        break;
    default:
        ret = ESP_AMP_RPC_STATUS_FAILED;
        break;
    }
    return ret;
}
