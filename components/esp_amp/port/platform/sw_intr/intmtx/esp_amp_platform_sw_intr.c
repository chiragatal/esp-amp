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
#include "esp_intr_alloc.h"
#endif

#include "esp_amp_platform.h"

#define ESP_AMP_MAIN_SW_INTR_REG    HP_SYSTEM_CPU_INT_FROM_CPU_2_REG
#define ESP_AMP_SUB_SW_INTR_REG     HP_SYSTEM_CPU_INT_FROM_CPU_3_REG

#define ESP_AMP_MAIN_SW_INTR        HP_SYSTEM_CPU_INT_FROM_CPU_2
#define ESP_AMP_SUB_SW_INTR         HP_SYSTEM_CPU_INT_FROM_CPU_3

#define ESP_AMP_MAIN_SW_INTR_SRC    ETS_FROM_CPU_INTR2_SOURCE
#define ESP_AMP_SUB_SW_INTR_SRC     ETS_FROM_CPU_INTR3_SOURCE

#define ESP_AMP_RESERVED_INTR_NO    (30)

extern void esp_amp_sw_intr_handler(void);

#if IS_MAIN_CORE
static intr_handle_t s_main_intr_handle = NULL;
#endif

static void IRAM_ATTR intmtx_sw_intr_handler(void *args)
{
#if IS_MAIN_CORE
    bool is_sw_intr = REG_READ(ESP_AMP_MAIN_SW_INTR_REG) & BIT(0);
#else
    bool is_sw_intr = REG_READ(ESP_AMP_SUB_SW_INTR_REG) & BIT(0);
#endif

    esp_amp_platform_sw_intr_clear();
    if (is_sw_intr) {
        esp_amp_sw_intr_handler();
    }
}

int esp_amp_platform_sw_intr_install(void)
{
#if IS_MAIN_CORE
    /* setup software interrupt on main-core and disable it by default */
    int ret = esp_intr_alloc(ESP_AMP_MAIN_SW_INTR_SRC, (ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_INTRDISABLED), intmtx_sw_intr_handler, NULL, &s_main_intr_handle);
    esp_intr_dump(NULL);
    return ret;
#else
    uint32_t core_id = esp_cpu_get_core_id();
    esp_cpu_intr_set_handler(ESP_AMP_RESERVED_INTR_NO, (esp_cpu_intr_handler_t)intmtx_sw_intr_handler, NULL);
    esp_rom_route_intr_matrix(core_id, ESP_AMP_SUB_SW_INTR_SRC, ESP_AMP_RESERVED_INTR_NO);
    int level = esp_intr_flags_to_level(ESP_INTR_FLAG_LEVEL1);
    esp_cpu_intr_set_priority(ESP_AMP_RESERVED_INTR_NO, level);
    return 0;
#endif
}

void esp_amp_platform_sw_intr_enable(void)
{
#if IS_MAIN_CORE
    esp_intr_enable(s_main_intr_handle);
#else
    esp_cpu_intr_enable(1 << ESP_AMP_RESERVED_INTR_NO);
#endif
}

void esp_amp_platform_sw_intr_disable(void)
{
#if IS_MAIN_CORE
    esp_intr_disable(s_main_intr_handle);
#else
    esp_cpu_intr_disable(1 << ESP_AMP_RESERVED_INTR_NO);
#endif
}

void esp_amp_platform_sw_intr_trigger(void)
{
#if IS_MAIN_CORE
    WRITE_PERI_REG(ESP_AMP_SUB_SW_INTR_REG, ESP_AMP_SUB_SW_INTR);
#else
    WRITE_PERI_REG(ESP_AMP_MAIN_SW_INTR_REG, ESP_AMP_MAIN_SW_INTR);
#endif
}

void esp_amp_platform_sw_intr_clear(void)
{
#if IS_MAIN_CORE
    WRITE_PERI_REG(ESP_AMP_MAIN_SW_INTR_REG, 0);
#else
    WRITE_PERI_REG(ESP_AMP_SUB_SW_INTR_REG, 0);
#endif
}
