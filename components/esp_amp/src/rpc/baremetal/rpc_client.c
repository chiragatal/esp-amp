/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "stdint.h"
#include "stdbool.h"
#include "limits.h"
#include "string.h"

#include "esp_amp_platform.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_rpc.h"
#include "esp_amp_log.h"

#define ESP_AMP_RPC_INVALID_REQ_ID 0x0

#define TAG "rpc_client"

typedef struct {
    uint16_t req_id; /* req_id & status still needed since pkt can be freed somewhere asynchronously */
    uint16_t status; /* status can be updated by timer */
    uint32_t start_time;
    uint32_t timeout_ms;
    esp_amp_rpc_req_cb_t cb;
    esp_amp_rpc_pkt_t *pkt;
} esp_amp_rpc_pending_req_t;

typedef struct {
    uint32_t service_id;
    uint32_t req_id;
    void *params;
    esp_amp_rpc_status_t status;
} esp_amp_rpc_resp_t;

typedef struct {
    esp_amp_rpc_pending_req_t *reqs[ESP_AMP_RPC_MAX_PENDING_REQ];
} esp_amp_rpc_pending_list_t;

typedef struct {
    uint16_t server_addr;
    uint16_t client_addr;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    esp_amp_rpc_pending_list_t pending_list;
} esp_amp_rpc_client_t;

static esp_amp_rpc_client_t esp_amp_rpc_client;
esp_amp_rpc_pending_req_t pending_reqs[ESP_AMP_RPC_MAX_PENDING_REQ];

static int esp_amp_rpc_client_poll(void *pkt_in_buf, uint16_t pkt_in_size, uint16_t src_addr, void* rx_cb_data);

static int esp_amp_rpc_pending_list_push(esp_amp_rpc_pending_req_t *req)
{
    int ret = -1;
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        if (esp_amp_rpc_client.pending_list.reqs[i] == NULL) {
            /* copy the filled slot to the empty one */
            esp_amp_rpc_client.pending_list.reqs[i] = req;
            ret = 0;
            break;
        }
    }
    return ret;
}

static int esp_amp_rpc_pending_list_pop(uint32_t req_id, esp_amp_rpc_pending_req_t **req_out)
{
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
    return ret;
}

static int esp_amp_rpc_pending_list_peek(uint32_t req_id, esp_amp_rpc_pending_req_t **req_out)
{
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
    return ret;
}


__attribute__((__unused__)) static void esp_amp_rpc_pending_list_dump(void)
{
    int list[ESP_AMP_RPC_MAX_PENDING_REQ];
    ESP_AMP_LOGD(TAG, "=== pending list ===");
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        if (esp_amp_rpc_client.pending_list.reqs[i] == NULL) {
            list[i] = 0;
        } else {
            list[i] = esp_amp_rpc_client.pending_list.reqs[i]->req_id;
        }
    }
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        ESP_AMP_LOGD(TAG, "%d\t%d", i, list[i]);
    }
    ESP_AMP_LOGD(TAG, "====================");
}

static uint16_t esp_amp_rpc_get_req_id(void)
{
    static uint16_t s_req_id = 0;
    if (s_req_id++ == SHRT_MAX) { /* wrap around */
        s_req_id = 1;
    }
    return s_req_id;
}

esp_amp_rpc_status_t esp_amp_rpc_client_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr)
{
    if (rpmsg_dev == NULL) {
        ESP_AMP_LOGE(TAG, "Invalid rpmsg_dev");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_client.rpmsg_dev = rpmsg_dev;
    esp_amp_rpc_client.client_addr = client_addr;
    esp_amp_rpc_client.server_addr = server_addr;

    if (esp_amp_rpmsg_create_ept(esp_amp_rpc_client.rpmsg_dev, client_addr, esp_amp_rpc_client_poll, NULL, &esp_amp_rpc_client.rpmsg_ept) == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create ept");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    esp_amp_rpc_client.server_addr = server_addr;
    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t esp_amp_rpc_client_deinit(void)
{
    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_req_handle_t esp_amp_rpc_client_create_request(uint16_t service_id, void *params, uint16_t params_len)
{

    /* first, check if any space in pending list */
    int avail_idx = ESP_AMP_RPC_MAX_PENDING_REQ;
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        if (pending_reqs[i].req_id == ESP_AMP_RPC_INVALID_REQ_ID) {
            avail_idx = i;
            break;
        }
    }

    if (avail_idx == ESP_AMP_RPC_MAX_PENDING_REQ) {
        ESP_AMP_LOGE(TAG, "No space for pending request");
        return NULL;
    }

    esp_amp_rpc_pending_req_t *pending_req = &pending_reqs[avail_idx];
    if (esp_amp_rpc_pending_list_push(pending_req) == -1) {
        ESP_AMP_LOGE(TAG, "No space in pending list");
        pending_req->req_id = ESP_AMP_RPC_INVALID_REQ_ID;
        return NULL;
    }

    pending_req->req_id = esp_amp_rpc_get_req_id();
    pending_req->start_time = esp_amp_platform_get_time_ms();
    pending_req->status = ESP_AMP_RPC_STATUS_PENDING;

    // esp_amp_rpc_pending_list_dump();

    /* second, alloc tx buffer only when pending list is not full */
    int pkt_out_size = params_len + sizeof(esp_amp_rpc_pkt_t);
    esp_amp_rpc_pkt_t *pkt_out = (esp_amp_rpc_pkt_t *)esp_amp_rpmsg_create_msg(esp_amp_rpc_client.rpmsg_dev, pkt_out_size, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (pkt_out == NULL) {
        ESP_AMP_LOGE(TAG, "No space for rpc pkt");
        esp_amp_rpc_pending_list_pop(pending_req->req_id, NULL); /* pop out pending request */
        pending_req->req_id = ESP_AMP_RPC_INVALID_REQ_ID;
        return NULL;
    }

    /* third, create packet */
    memcpy(pkt_out->params, params, params_len);
    pkt_out->params_len = params_len;
    pkt_out->req_id = pending_req->req_id;
    pkt_out->service_id = service_id;
    pkt_out->status = ESP_AMP_RPC_STATUS_PENDING;

    /* attach pkt_out to pending req */
    pending_req->pkt = pkt_out;

    ESP_AMP_LOGD(TAG, "request(req_id=%lu, srv_id=%lu, param=%p, start_time=%lu)", pkt_out->req_id, pkt_out->service_id,
                 pkt_out->params, pending_req->start_time);

    return pending_req;
}

esp_amp_rpc_status_t esp_amp_rpc_client_execute_request_with_cb(esp_amp_rpc_req_handle_t req, esp_amp_rpc_req_cb_t cb, uint32_t timeout_ms)
{
    esp_amp_rpc_pending_req_t *pending_req = (esp_amp_rpc_pending_req_t *)req;
    pending_req->cb = cb;
    pending_req->timeout_ms = timeout_ms;

    /* finally, send out */
    if (!pending_req) {
        return ESP_AMP_RPC_STATUS_INVALID_ARG;
    }

    esp_amp_rpmsg_send_nocopy(esp_amp_rpc_client.rpmsg_dev, &esp_amp_rpc_client.rpmsg_ept, esp_amp_rpc_client.server_addr,
                              pending_req->pkt, pending_req->pkt->params_len + sizeof(esp_amp_rpc_pkt_t));
    return ESP_AMP_RPC_STATUS_OK;
}

void esp_amp_rpc_client_destroy_request(esp_amp_rpc_req_handle_t req)
{
    esp_amp_rpc_pending_req_t *pending_req = (esp_amp_rpc_pending_req_t *)req;
    esp_amp_rpc_pending_list_pop(pending_req->req_id, NULL);
    pending_req->req_id = ESP_AMP_RPC_INVALID_REQ_ID; /* set req_id to invalid */
}

/* check pending list periodically and pop out the timeout requests */
void esp_amp_rpc_client_complete_timeout_request(void)
{
    ESP_AMP_LOGD(TAG, "=== timeout request begin ===");
    for (int i = 0; i < ESP_AMP_RPC_MAX_PENDING_REQ; i++) {
        esp_amp_rpc_pending_req_t *pending_req = esp_amp_rpc_client.pending_list.reqs[i];
        if (!pending_req || pending_req->req_id == ESP_AMP_RPC_INVALID_REQ_ID) {
            continue;
        }
        ESP_AMP_LOGD(TAG, "req(%u): timeout=%lu, start=%lu, cur=%lu", pending_req->req_id, pending_req->timeout_ms, pending_req->start_time, esp_amp_platform_get_time_ms());

        /* if timeout, remove this request from pending list */
        if (esp_amp_platform_get_time_ms() - pending_req->start_time >= pending_req->timeout_ms) {
            if (pending_req->cb) {
                pending_req->cb(ESP_AMP_RPC_STATUS_TIMEOUT, NULL, 0);
            }
            esp_amp_rpc_pending_list_pop(pending_req->req_id, NULL);
            pending_req->req_id = ESP_AMP_RPC_INVALID_REQ_ID;
        }
    }
    ESP_AMP_LOGD(TAG, "=== timeout request end ===");
}

/**
 * rpmsg_poll() callback for rpc client
 * triggered when receiving an incoming transport packet from rpmsg
 */
static int esp_amp_rpc_client_poll(void *pkt_in_buf, uint16_t pkt_in_size, uint16_t src_addr, void* rx_cb_data)
{
    int ret = 0;
    esp_amp_rpc_pending_req_t *pending_req;
    esp_amp_rpc_pkt_t *pkt_in;

    if (pkt_in_size < sizeof(esp_amp_rpc_pkt_t)) {
        ESP_AMP_LOGE(TAG, "Incomplete pkt in");
        ret = -1;
    }

    if (ret != -1) {
        pkt_in = (esp_amp_rpc_pkt_t *)pkt_in_buf;
        /* find req from pending list and execute cb */
        ret = esp_amp_rpc_pending_list_peek(pkt_in->req_id, &pending_req);

        /* rsp may belong to a timeout req already moved out of pending list */
        if (ret == -1) {
            ESP_AMP_LOGD(TAG, "recv rsp for timeout req(%u)", pkt_in->req_id);
        } else {
            if (pending_req->cb) {
                ESP_AMP_LOGD(TAG, "calling req(%u)'s cb %p", pkt_in->req_id, pending_req->cb);
                pending_req->cb(pkt_in->status, pkt_in->params, pkt_in->params_len);
            }
        }
    }

    /* release rx buf */
    if (ret != -1) {
        esp_amp_rpc_pending_list_pop(pkt_in->req_id, NULL);
        pending_req->req_id = ESP_AMP_RPC_INVALID_REQ_ID;
    }
    esp_amp_rpmsg_destroy(esp_amp_rpc_client.rpmsg_dev, pkt_in_buf);
    return ret;
}
