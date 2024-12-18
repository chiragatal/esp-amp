/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp_log.h"
#include "esp_amp_sys_info.h"
#include "esp_amp_platform.h"
#include "esp_amp_sw_intr.h"
#include "esp_amp_sw_intr_priv.h"
#include "esp_amp_platform.h"

#if !IS_ENV_BM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const char *TAG = "sw_intr";

sw_intr_handler_tbl_t sw_intr_handlers[ESP_AMP_SW_INTR_HANDLER_TABLE_LEN];

esp_amp_sw_intr_st_t *s_sw_intr_st;

int esp_amp_sw_intr_add_handler(esp_amp_sw_intr_id_t intr_id, esp_amp_sw_intr_handler_t handler, void *arg)
{
    assert(intr_id <= SW_INTR_ID_MAX);

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
    assert(intr_id <= SW_INTR_ID_MAX);

    for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
        if (sw_intr_handlers[i].intr_id == intr_id && sw_intr_handlers[i].handler == handler) {
            sw_intr_handlers[i].handler = NULL;
        }
    }
}

void esp_amp_sw_intr_trigger(esp_amp_sw_intr_id_t intr_id)
{
    ESP_AMP_LOGD(TAG, "intr_id:%d, SW_INTR_ID_MAX:%d", intr_id, SW_INTR_ID_MAX);
    assert((int)intr_id <= (int)SW_INTR_ID_MAX);

    /* must be initialized */
    assert(s_sw_intr_st != NULL);

#if IS_MAIN_CORE
    ESP_AMP_LOGD(TAG, "maincore trigger sw intr");
    atomic_fetch_or(&(s_sw_intr_st->sub_core_sw_intr_st), BIT(intr_id));
#else
    ESP_AMP_LOGD(TAG, "subcore trigger sw intr");
    atomic_fetch_or(&(s_sw_intr_st->main_core_sw_intr_st), BIT(intr_id));
#endif
    esp_amp_platform_sw_intr_trigger();
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

int esp_amp_sw_intr_init(void)
{
#if IS_MAIN_CORE
    /* initialize software interrupt status */
    s_sw_intr_st = (esp_amp_sw_intr_st_t *) esp_amp_sys_info_alloc(SYS_INFO_RESERVED_ID_SW_INTR, sizeof(esp_amp_sw_intr_st_t));
    if (s_sw_intr_st == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to alloc sw_intr_st in sys info");
        return -1;
    }

    atomic_init(&s_sw_intr_st->main_core_sw_intr_st, 0);
    atomic_init(&s_sw_intr_st->sub_core_sw_intr_st, 0);
#else
    s_sw_intr_st = (esp_amp_sw_intr_st_t *) esp_amp_sys_info_get(SYS_INFO_RESERVED_ID_SW_INTR, NULL);
    if (s_sw_intr_st == NULL) {
        return -1;
    }
#endif

    int ret = esp_amp_platform_sw_intr_install();
    if (ret == 0) {
        esp_amp_platform_sw_intr_enable();
#if IS_ENV_BM
        esp_amp_platform_intr_enable(); /* enable global interrupt */
#endif
    }
    return ret;
}

void esp_amp_sw_intr_handler(void)
{
#if !IS_ENV_BM
    bool need_yield = false;
#endif
    int unprocessed = 0;

#if IS_MAIN_CORE
    ESP_AMP_DRAM_LOGD(TAG, "Received software interrupt from subcore\n");
    while (!atomic_compare_exchange_weak(&s_sw_intr_st->main_core_sw_intr_st, &unprocessed, 0));
#else
    ESP_AMP_DRAM_LOGD(TAG, "Received software interrupt from maincore\n");
    while (!atomic_compare_exchange_weak(&s_sw_intr_st->sub_core_sw_intr_st, &unprocessed, 0));
#endif
    ESP_AMP_DRAM_LOGD(TAG, "sw_intr_st at %p, unprocessed=0x%x\n", s_sw_intr_st, (unsigned)unprocessed);

    while (unprocessed) {
        for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
            /* intr_id matches unprocessed bit */
            if (unprocessed & BIT(sw_intr_handlers[i].intr_id)) {
                if (sw_intr_handlers[i].handler) {
                    ESP_AMP_DRAM_LOGD(TAG, "executing handler(%p)", sw_intr_handlers[i].handler);
#if !IS_ENV_BM
                    need_yield |= sw_intr_handlers[i].handler(sw_intr_handlers[i].arg);
#else
                    sw_intr_handlers[i].handler(sw_intr_handlers[i].arg);
#endif
                }
            }
        }
        /* clear all interrupt bit */
        unprocessed = 0;
#if IS_MAIN_CORE
        while (!atomic_compare_exchange_weak(&s_sw_intr_st->main_core_sw_intr_st, &unprocessed, 0));
#else
        while (!atomic_compare_exchange_weak(&s_sw_intr_st->sub_core_sw_intr_st, &unprocessed, 0));
#endif
    }

#if !IS_ENV_BM
    portYIELD_FROM_ISR(need_yield);
#endif
}
