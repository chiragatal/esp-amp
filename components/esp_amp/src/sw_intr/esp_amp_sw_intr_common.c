/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp_sw_intr_priv.h"
#include "esp_amp_priv.h"

sw_intr_handler_tbl_t sw_intr_handlers[ESP_AMP_SW_INTR_HANDLER_TABLE_LEN];

esp_amp_sw_intr_st_t *s_sw_intr_st;

int esp_amp_sw_intr_add_handler(esp_amp_sw_intr_id_t intr_id, esp_amp_sw_intr_handler_t handler, void *arg)
{
    ESP_AMP_ASSERT(intr_id <= SW_INTR_ID_MAX);

    int ret = 0;
    /* find an available slot */
    int avail_slot = ESP_AMP_SW_INTR_HANDLER_TABLE_LEN;
    for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
        if (sw_intr_handlers[i].handler == NULL) {
            avail_slot = i;
            break;
        }
    }

    /* add handler to this slot */
    if (avail_slot != ESP_AMP_SW_INTR_HANDLER_TABLE_LEN) {
        sw_intr_handlers[avail_slot].intr_id = intr_id;
        sw_intr_handlers[avail_slot].handler = handler;
        sw_intr_handlers[avail_slot].arg = arg;
    } else {
        ret = -1;
    }
    return ret;
}

void esp_amp_sw_intr_delete_handler(esp_amp_sw_intr_id_t intr_id, esp_amp_sw_intr_handler_t handler)
{
    ESP_AMP_ASSERT(intr_id <= SW_INTR_ID_MAX);

    for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
        if (sw_intr_handlers[i].intr_id == intr_id && sw_intr_handlers[i].handler == handler) {
            sw_intr_handlers[i].handler = NULL;
        }
    }
}

void esp_amp_sw_intr_handler_dump(void)
{
    ESP_AMP_LOGD(TAG, "== sw handlers ==");
    ESP_AMP_LOGD(TAG, "intr_id\thandler");
    for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
        if (sw_intr_handlers[i].handler) {
            ESP_AMP_LOGD(TAG, "%d\t%p", sw_intr_handlers[i].intr_id, sw_intr_handlers[i].handler);
        }
    }
    ESP_AMP_LOGD(TAG, "=================");
}

