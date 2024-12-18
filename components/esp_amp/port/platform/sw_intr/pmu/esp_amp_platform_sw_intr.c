/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "soc/pmu_struct.h"

#if IS_MAIN_CORE
#include "soc/interrupts.h"
#include "esp_intr_alloc.h"
#else
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_interrupts.h"
#endif

#include "esp_amp_platform.h"

extern void esp_amp_sw_intr_handler(void);

#if IS_MAIN_CORE
static void IRAM_ATTR pmu_sw_intr_handler(void *args)
{
    bool is_sw_intr = PMU.hp_ext.int_st.sw;
    esp_amp_platform_sw_intr_clear();

    if (is_sw_intr) {
        esp_amp_sw_intr_handler();
    }
}
#else
void LP_CORE_ISR_ATTR ulp_lp_core_lp_pmu_intr_handler(void)
{
    esp_amp_platform_sw_intr_clear();
    esp_amp_sw_intr_handler();
}
#endif /* IS_MAIN_CORE */

int esp_amp_platform_sw_intr_install(void)
{
    int ret = 0;
#if IS_MAIN_CORE
#if CONFIG_IDF_TARGET_ESP32C6
    ret |= esp_intr_alloc(ETS_PMU_INTR_SOURCE, ESP_INTR_FLAG_LEVEL2, pmu_sw_intr_handler, NULL, NULL);
#elif CONFIG_IDF_TARGET_ESP32P4
    ret |= esp_intr_alloc(ETS_PMU_0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL2, pmu_sw_intr_handler, NULL, NULL);
#endif
    esp_intr_dump(NULL);
#endif
    return ret;
}

void esp_amp_platform_sw_intr_enable(void)
{
#if IS_MAIN_CORE
    /* enable PMU SW interrupt */
    PMU.hp_ext.int_ena.sw = 1;
#else
    ulp_lp_core_sw_intr_enable(true);
#endif
}

void esp_amp_platform_sw_intr_disable(void)
{
#if IS_MAIN_CORE
    /* disable PMU SW interrupt */
    PMU.hp_ext.int_ena.sw = 0;
#else
    ulp_lp_core_sw_intr_enable(false);
#endif
}

void esp_amp_platform_sw_intr_trigger(void)
{
#if IS_MAIN_CORE
    PMU.hp_lp_cpu_comm.hp_trigger_lp = 1;
#else
    PMU.hp_lp_cpu_comm.lp_trigger_hp = 1;
#endif /* IS_MAIN_CORE */
}

void esp_amp_platform_sw_intr_clear(void)
{
#if IS_MAIN_CORE
    PMU.hp_ext.int_clr.sw = 1;
#else
    ulp_lp_core_sw_intr_clear();
#endif /* IS_MAIN_CORE */
}
