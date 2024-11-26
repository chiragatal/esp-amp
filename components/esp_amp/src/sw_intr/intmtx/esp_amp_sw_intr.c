/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_cpu.h"
#include "soc/interrupt_core0_reg.h"
#include "soc/hp_system_reg.h"
#include "soc/hp_system_struct.h"
#include "soc/interrupts.h"
#include "esp_rom_sys.h"

#if IS_MAIN_CORE
#include "soc/interrupts.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const DRAM_ATTR char TAG[] = "sw_intr";

#include "esp_amp_log.h"
#include "esp_amp_sw_intr_priv.h"
#include "esp_amp_sys_info.h"

#define ESP_AMP_MAIN_SW_INTR_REG    HP_SYSTEM_CPU_INT_FROM_CPU_2_REG
#define ESP_AMP_SUB_SW_INTR_REG     HP_SYSTEM_CPU_INT_FROM_CPU_3_REG

#define ESP_AMP_MAIN_SW_INTR        HP_SYSTEM_CPU_INT_FROM_CPU_2
#define ESP_AMP_SUB_SW_INTR         HP_SYSTEM_CPU_INT_FROM_CPU_3

#define ESP_AMP_MAIN_SW_INTR_SRC    ETS_FROM_CPU_INTR2_SOURCE
#define ESP_AMP_SUB_SW_INTR_SRC     ETS_FROM_CPU_INTR3_SOURCE

#define ESP_AMP_RESERVED_INTR_NO    (30)


extern sw_intr_handler_tbl_t sw_intr_handlers[ESP_AMP_SW_INTR_HANDLER_TABLE_LEN];

extern esp_amp_sw_intr_st_t *s_sw_intr_st;

static void IRAM_ATTR intr_mat_sw_intr_handler(void *args)
{
#if !IS_ENV_BM
    bool need_yield = false;
#endif

    int unprocessed = 0;

#if IS_MAIN_CORE
    uint32_t intr_status_reg = REG_READ(ESP_AMP_MAIN_SW_INTR_REG);
#else
    uint32_t intr_status_reg = REG_READ(ESP_AMP_SUB_SW_INTR_REG);
#endif

    if (intr_status_reg & 1) {
        // clear software interrupt bit
#if IS_MAIN_CORE
        WRITE_PERI_REG(ESP_AMP_MAIN_SW_INTR_REG, 0);
#else
        WRITE_PERI_REG(ESP_AMP_SUB_SW_INTR_REG, 0);
#endif

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
    } else {
        ESP_AMP_DRAM_LOGD(TAG, "Unknown interrupt: 0x%p\n", (void*)(intr_status_reg));
    }

#if !IS_ENV_BM
    portYIELD_FROM_ISR(need_yield);
#endif

}

void esp_amp_sw_intr_trigger(esp_amp_sw_intr_id_t intr_id)
{
    ESP_AMP_LOGD(TAG, "intr_id:%d, SW_INTR_ID_MAX:%d", intr_id, SW_INTR_ID_MAX);
    assert((int)intr_id <= (int)SW_INTR_ID_MAX);

    /* must be initialized */
    assert(s_sw_intr_st != NULL);

    int core_id = esp_cpu_get_core_id();

    if (core_id == 0) {
        ESP_AMP_LOGD(TAG, "maincore trigger sw intr");
        atomic_fetch_or(&(s_sw_intr_st->sub_core_sw_intr_st), BIT(intr_id));
        WRITE_PERI_REG(ESP_AMP_SUB_SW_INTR_REG, ESP_AMP_SUB_SW_INTR);
    } else {
        ESP_AMP_LOGD(TAG, "subcore trigger sw intr");
        atomic_fetch_or(&(s_sw_intr_st->main_core_sw_intr_st), BIT(intr_id));
        WRITE_PERI_REG(ESP_AMP_MAIN_SW_INTR_REG, ESP_AMP_MAIN_SW_INTR);
    }
}

#if IS_MAIN_CORE
static int hp_core_sw_intr_setup(void)
{
    return esp_intr_alloc(ESP_AMP_MAIN_SW_INTR_SRC, ESP_INTR_FLAG_LEVEL2, intr_mat_sw_intr_handler, NULL, NULL);
}
#endif

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
    uint32_t core_id = esp_cpu_get_core_id();
    esp_cpu_intr_set_handler(ESP_AMP_RESERVED_INTR_NO, (esp_cpu_intr_handler_t)intr_mat_sw_intr_handler, NULL);
    esp_rom_route_intr_matrix(core_id, ESP_AMP_SUB_SW_INTR_SRC, ESP_AMP_RESERVED_INTR_NO);
    esp_cpu_intr_enable(1 << ESP_AMP_RESERVED_INTR_NO);
    int level = esp_intr_flags_to_level(ESP_INTR_FLAG_LEVEL2);
    esp_cpu_intr_set_priority(ESP_AMP_RESERVED_INTR_NO, level);
    ESP_AMP_LOGI(TAG, "Connected src %d to int %d (cpu %"PRIu32")", ESP_AMP_SUB_SW_INTR_SRC, ESP_AMP_RESERVED_INTR_NO, core_id);
    return 0;
#endif
}

/**
 * init software interrupt
 */
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

    return esp_amp_sw_intr_enable();

}
