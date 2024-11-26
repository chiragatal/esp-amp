/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "sdkconfig.h"
#include "stdint.h"
#include "stddef.h"
#include "string.h"

#include "esp_amp_rpc.h"
#include "esp_amp_rpmsg.h"
#include "esp_amp_log.h"

#define TAG "rpc_server"

#define ESP_AMP_RPC_SERVICE_TABLE_LEN CONFIG_ESP_AMP_RPC_SERVICE_TABLE_LEN

typedef struct {
    int len;
    esp_amp_rpc_service_t services[ESP_AMP_RPC_SERVICE_TABLE_LEN];
} esp_amp_rpc_service_tbl_t;

typedef struct {
    uint16_t server_addr;
    uint16_t client_addr;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    esp_amp_rpc_service_tbl_t service_tbl;
} esp_amp_rpc_server_t;

static esp_amp_rpc_server_t esp_amp_rpc_server;

static int esp_amp_rpc_server_poll(void *pkt_in_buf, uint16_t size, uint16_t src_addr, void* rx_cb_data);

esp_amp_rpc_status_t esp_amp_rpc_server_init(esp_amp_rpmsg_dev_t *rpmsg_dev, uint16_t client_addr, uint16_t server_addr)
{
    if (!rpmsg_dev) {
        ESP_AMP_LOGE(TAG, "Invalid rpmsg_dev");
        return ESP_AMP_RPC_STATUS_FAILED;
    }
    esp_amp_rpc_server.rpmsg_dev = rpmsg_dev;
    esp_amp_rpc_server.client_addr = client_addr;
    esp_amp_rpc_server.server_addr = server_addr;

    /* init service_tbl*/
    esp_amp_rpc_server.service_tbl.len = 0;

    /* register endpoint */
    if (esp_amp_rpmsg_create_endpoint(esp_amp_rpc_server.rpmsg_dev, server_addr, esp_amp_rpc_server_poll, NULL, &esp_amp_rpc_server.rpmsg_ept) == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to create ept");
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t esp_amp_rpc_server_add_service(esp_amp_rpc_service_id_t srv_id, esp_amp_rpc_service_func_t srv_func)
{
    int next_idx = esp_amp_rpc_server.service_tbl.len;
    if (next_idx == ESP_AMP_RPC_SERVICE_TABLE_LEN) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

    if (srv_func == NULL) {
        return ESP_AMP_RPC_STATUS_FAILED;
    }

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
    ESP_AMP_LOGD(TAG, "added srv(%u, %p) to tbl[%d]", srv_id, srv_func, next_idx);
    return ESP_AMP_RPC_STATUS_OK;
}


esp_amp_rpc_status_t esp_amp_rpc_server_deinit(void)
{
    return ESP_AMP_RPC_STATUS_OK;
}


static int esp_amp_rpc_server_poll(void *pkt_in_buf, uint16_t size, uint16_t src_addr, void* rx_cb_data)
{
    int ret = 0;
    esp_amp_rpc_pkt_t *pkt_in;
    esp_amp_rpc_pkt_t *pkt_out;
    uint32_t rpmsg_len = esp_amp_rpmsg_get_max_size(esp_amp_rpc_server.rpmsg_dev);

    if (size < sizeof(esp_amp_rpc_pkt_t)) {
        ESP_AMP_LOGE(TAG, "Incomplete pkt in");
        ret = -1;
    }

    /* decode */
    if (ret != -1) {
        pkt_in = (esp_amp_rpc_pkt_t *)pkt_in_buf;
        ESP_AMP_LOGD(TAG, "server(%u) recv req(pkt=%p, req_id=%u) from client(%u)", esp_amp_rpc_server.rpmsg_ept.addr,
                     pkt_in, pkt_in->req_id, src_addr);
        ESP_AMP_LOG_BUFFER_HEXDUMP(TAG, pkt_in, pkt_in->params_len + sizeof(esp_amp_rpc_pkt_t), ESP_AMP_LOG_DEBUG);

        /* alloc tx_buf (pkt_out) */
        pkt_out = (esp_amp_rpc_pkt_t *)esp_amp_rpmsg_create_message(esp_amp_rpc_server.rpmsg_dev, rpmsg_len, ESP_AMP_RPMSG_DATA_DEFAULT);
        if (pkt_out == NULL) {
            ESP_AMP_LOGE(TAG, "Failed to alloc tx buf for pkt_out");
            ret = -1;
        }

        /* copy from pkt_in to pkt_out */
        memcpy(pkt_out, pkt_in, sizeof(esp_amp_rpc_pkt_t));
        pkt_out->params_len = rpmsg_len;
        pkt_out->status = ESP_AMP_RPC_STATUS_NO_SERVICE;
    }

    /* if buffer out also ready */
    if (ret != -1) {
        ESP_AMP_LOGD(TAG, "Executing(req_id:%u, srv_id:%u, status:%u, param(%u):%p)", pkt_in->req_id, pkt_in->service_id, pkt_in->status, pkt_in->params_len, pkt_in->params);
        /* execute service */
        for (int i = 0; i < esp_amp_rpc_server.service_tbl.len; i++) {
            if (esp_amp_rpc_server.service_tbl.services[i].handler == NULL) {
                continue;
            }
            if (esp_amp_rpc_server.service_tbl.services[i].id == pkt_in->service_id) {
                int ret = esp_amp_rpc_server.service_tbl.services[i].handler(pkt_in->params, pkt_in->params_len, pkt_out->params, &pkt_out->params_len);
                if (ret == 0) {
                    pkt_out->status = ESP_AMP_RPC_STATUS_OK;
                } else {
                    pkt_out->status = ESP_AMP_RPC_STATUS_EXEC_FAILED;
                }
                break;
            }
        }

        /* release rx buffer (pkt_in) */
        esp_amp_rpmsg_destroy(esp_amp_rpc_server.rpmsg_dev, pkt_in_buf);

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
        ESP_AMP_LOGD(TAG, "server(%u) send rsp(pkt=%p, req_id=%u) to client(%u)", esp_amp_rpc_server.rpmsg_ept.addr,
                     pkt_out, pkt_out->req_id, src_addr);
        ESP_AMP_LOG_BUFFER_HEXDUMP(TAG, pkt_out, pkt_out->params_len + sizeof(esp_amp_rpc_pkt_t), ESP_AMP_LOG_DEBUG);

        esp_amp_rpmsg_send_nocopy(esp_amp_rpc_server.rpmsg_dev, &esp_amp_rpc_server.rpmsg_ept, esp_amp_rpc_server.client_addr,
                                  pkt_out, pkt_out->params_len + sizeof(esp_amp_rpc_pkt_t));
    }

    return 0;
}
