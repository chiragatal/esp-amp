/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "limits.h"
#include "riscv/rv_utils.h"

#if IS_MAIN_CORE
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#endif

#if IS_ENV_BM
#include "ulp_lp_core_print.h"
#include "ulp_lp_core_utils.h"
#endif

#include "esp_amp_arch.h"
#include "esp_amp_platform.h"
#include "esp_amp.h"

#define LP_CORE_CPU_FREQ_HZ 16000000

#if !IS_MAIN_CORE
void esp_amp_platform_delay_us(uint32_t time)
{
    ulp_lp_core_delay_us(time);
}

void esp_amp_platform_delay_ms(uint32_t time)
{
    if (time >= INT_MAX / 1000) { /* if overflow */
        ulp_lp_core_delay_us(INT_MAX);
    } else {
        ulp_lp_core_delay_us(time * 1000);
    }
}

uint32_t esp_amp_platform_get_time_ms(void)
{
    uint64_t cpu_cycle_u64 = esp_amp_arch_get_cpu_cycle();
    return (uint32_t)(cpu_cycle_u64 / (LP_CORE_CPU_FREQ_HZ / 1000));
}

void esp_amp_platform_intr_enable(void)
{
    asm volatile("csrs mstatus, %0" : : "r"(1 << 3));
}

void esp_amp_platform_intr_disable(void)
{
    asm volatile("csrc mstatus, %0" : : "r"(1 << 3));
}
#endif /* !IS_MAIN_CORE */

#if IS_MAIN_CORE
static void lp_uart_init(void)
{
    lp_core_uart_cfg_t cfg = LP_CORE_UART_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(lp_core_uart_init(&cfg));
    printf("LP UART initialized successfully\n");
}

int esp_amp_platform_start_subcore(void)
{
    lp_uart_init();
    /* Set LP core wakeup source as the HP CPU */
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
    };

    /* Run LP core */
    esp_err_t ret = ulp_lp_core_run(&cfg);
    if (ret != ESP_OK) {
        return -1;
    }
    return 0;
}

void esp_amp_platform_stop_subcore(void)
{
    ulp_lp_core_stop();
}

#endif /* IS_MAIN_CORE */
