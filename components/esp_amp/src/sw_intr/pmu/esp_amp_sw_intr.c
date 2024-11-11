/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "soc/pmu_struct.h"

#if IS_MAIN_CORE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/interrupts.h"
#include "esp_intr_alloc.h"
#else
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_interrupts.h"
#endif

#include "esp_amp_sys_info.h"
#include "esp_amp_sw_intr_priv.h"
#include "esp_amp_priv.h"

extern sw_intr_handler_tbl_t sw_intr_handlers[ESP_AMP_SW_INTR_HANDLER_TABLE_LEN];

extern esp_amp_sw_intr_st_t *s_sw_intr_st;

void esp_amp_sw_intr_trigger(esp_amp_sw_intr_id_t intr_id)
{
    ESP_AMP_LOGD(TAG, "intr_id:%d, SW_INTR_ID_MAX:%d", intr_id, SW_INTR_ID_MAX);
    ESP_AMP_ASSERT((int)intr_id <= (int)SW_INTR_ID_MAX);

    /* must be initialized */
    ESP_AMP_ASSERT(s_sw_intr_st != NULL);

#if IS_ULP_COCPU
    ESP_AMP_LOGD(TAG, "subcore trigger sw intr");
    atomic_fetch_or(&(s_sw_intr_st->main_core_sw_intr_st), BIT(intr_id));
    PMU.hp_lp_cpu_comm.lp_trigger_hp = 1;
#else
    ESP_AMP_LOGD(TAG, "maincore trigger sw intr");
    atomic_fetch_or(&(s_sw_intr_st->sub_core_sw_intr_st), BIT(intr_id));
    PMU.hp_lp_cpu_comm.hp_trigger_lp = 1;
#endif /* IS_ULP_COCPU */
}

#if !IS_ULP_COCPU
static void IRAM_ATTR pmu_sw_intr_handler(void *args)
{
    bool need_yield = false;
    int unprocessed = 0;

    if (PMU.hp_ext.int_st.sw) {
        PMU.hp_ext.int_clr.sw = 1; /* clear software interrupt bit */
        ESP_AMP_DRAM_LOGD(TAG, "%s() called. Received software interrupt from LP\n", __func__);

        while (!atomic_compare_exchange_weak(&s_sw_intr_st->main_core_sw_intr_st, &unprocessed, 0));
        ESP_AMP_DRAM_LOGD(TAG, "sw_intr_st at %p, unprocessed=0x%x\n", s_sw_intr_st, (unsigned)unprocessed);

        while (unprocessed) {
            for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
                /* intr_id matches unprocessed bit */
                if (unprocessed & BIT(sw_intr_handlers[i].intr_id)) {
                    if (sw_intr_handlers[i].handler) {
                        ESP_AMP_DRAM_LOGD(TAG, "executing handler(%p)", sw_intr_handlers[i].handler);
                        need_yield |= sw_intr_handlers[i].handler(sw_intr_handlers[i].arg);
                    }
                }
            }
            /* clear all interrupt bit */
            unprocessed = 0;
            while (!atomic_compare_exchange_weak(&s_sw_intr_st->main_core_sw_intr_st, &unprocessed, 0));
        }
    } else {
        ESP_AMP_DRAM_LOGD(TAG, "%s() called. Unknown interrupt: 0x%08x\n", __func__, PMU.hp_ext.int_st.val);
    }

    portYIELD_FROM_ISR(need_yield);
}

static int hp_core_sw_intr_setup(void)
{
    int ret = 0;
#if CONFIG_IDF_TARGET_ESP32C6
    ret |= esp_intr_alloc(ETS_PMU_INTR_SOURCE, ESP_INTR_FLAG_LEVEL2, pmu_sw_intr_handler, NULL, NULL);
#elif CONFIG_IDF_TARGET_ESP32P4
    ret |= esp_intr_alloc(ETS_PMU_0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL2, pmu_sw_intr_handler, NULL, NULL);
#endif

    /* enable PMU SW interrupt */
    PMU.hp_ext.int_ena.sw = 1;
    return ret;
}

#endif /* !IS_ULP_COCPU */


#if IS_ULP_COCPU
void LP_CORE_ISR_ATTR ulp_lp_core_lp_pmu_intr_handler(void)
{
    /* clear cross core interrupt immediately to avoid lost wakeup problem */
    ulp_lp_core_sw_intr_clear();

    /* atomically load data from sw_intr_st to unprocessed and clear it */
    uint32_t unprocessed = 0;
    while (!atomic_compare_exchange_weak(&s_sw_intr_st->sub_core_sw_intr_st, &unprocessed, 0));
    ESP_AMP_DRAM_LOGD(TAG, "%s: unprocessed=0x%08x\r\n", __func__, unprocessed);

    while (unprocessed) {
        for (int i = 0; i < ESP_AMP_SW_INTR_HANDLER_TABLE_LEN; i++) {
            /* intr_id matches unprocessed bit */
            if (unprocessed & BIT(sw_intr_handlers[i].intr_id)) {
                if (sw_intr_handlers[i].handler) {
                    sw_intr_handlers[i].handler(sw_intr_handlers[i].arg);
                }
            }
        }
        /* clear all interrupt bit */
        unprocessed = 0;
        while (!atomic_compare_exchange_weak(&s_sw_intr_st->main_core_sw_intr_st, &unprocessed, 0));
    }
}
#endif /* IS_ULP_COCPU */


/**
 * enable software interrupt
 */
static int esp_amp_sw_intr_enable(void)
{
#if IS_MAIN_CORE
    /* setup software interrupt on main-core */
    int ret = hp_core_sw_intr_setup();
    esp_intr_dump(NULL);
    return ret;
#else
    ulp_lp_core_intr_enable();
    ulp_lp_core_sw_intr_enable(true);
    return 0;
#endif
}


/**
 * init software interrupt
 */
int esp_amp_sw_intr_init(void)
{
#if !IS_MAIN_CORE
    s_sw_intr_st = (esp_amp_sw_intr_st_t *) esp_amp_sys_info_get(SYS_INFO_ID_SW_INTR, NULL);
    if (s_sw_intr_st == NULL) {
        return -1;
    }
#else
    /* initialize software interrupt status */
    s_sw_intr_st = (esp_amp_sw_intr_st_t *) esp_amp_sys_info_alloc(SYS_INFO_ID_SW_INTR, sizeof(esp_amp_sw_intr_st_t));
    if (s_sw_intr_st == NULL) {
        ESP_AMP_LOGE(TAG, "Failed to alloc sw_intr_st in sys info");
        return -1;
    }

    atomic_init(&s_sw_intr_st->main_core_sw_intr_st, 0);
    atomic_init(&s_sw_intr_st->sub_core_sw_intr_st, 0);
#endif /* IS_ULP_COCPU */

    return esp_amp_sw_intr_enable();
}
