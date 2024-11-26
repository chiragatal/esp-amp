/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "sdkconfig.h"
#include "esp_amp_arch.h"
#include "esp_amp_platform.h"
#include "rom/ets_sys.h"
#include "esp_rom_sys.h"
#include "hal/cpu_utility_ll.h"
#include "soc/hp_sys_clkrst_reg.h"
#include "hal/cache_ll.h"
#include "hal/systimer_ll.h"
#include "riscv/rv_utils.h"

// TODO: fixme: P4 HP core freq can change if DFS enabled
#if CONFIG_IDF_TARGET_ESP32P4
#define HP_CORE_CPU_FREQ_HZ 360000000
#endif

#if IS_MAIN_CORE
uint32_t hp_subcore_boot_addr;
#endif

#if !IS_MAIN_CORE
void esp_amp_platform_delay_us(uint32_t time)
{
    esp_rom_delay_us(time);
    return;
}

void esp_amp_platform_delay_ms(uint32_t time)
{
    esp_rom_delay_us(time * 1000);
    return;
}

uint32_t esp_amp_platform_get_time_ms(void)
{
    uint64_t cpu_cycle_u64 = esp_amp_arch_get_cpu_cycle();
    return (uint32_t)(cpu_cycle_u64 / (HP_CORE_CPU_FREQ_HZ / 1000));
}

extern volatile unsigned long _bm_intr_level_count;
void esp_amp_platform_intr_enable(void)
{
    if (_bm_intr_level_count == 0) {
        /* In baremetal environment main-loop */
        /* currently, baremetal environment doesn't support nested interrupts */
        asm volatile("csrs mstatus, %0" : : "r"(1 << 3));
    } /* _bm_intr_level_count > 0, in ISR context */
}

void esp_amp_platform_intr_disable(void)
{
    asm volatile("csrc mstatus, %0" : : "r"(1 << 3));
}

#endif

#if IS_MAIN_CORE
int esp_amp_platform_start_subcore(void)
{
    cache_ll_writeback_all(CACHE_LL_LEVEL_INT_MEM, CACHE_TYPE_DATA, CACHE_LL_ID_ALL);
    cpu_utility_ll_unstall_cpu(1);
#if CONFIG_IDF_TARGET_ESP32P4
    /* reset cpu clk */
    if (!REG_GET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_CPU_CLK_EN)) {
        REG_SET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_CPU_CLK_EN);
    }
    if (REG_GET_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG, HP_SYS_CLKRST_REG_RST_EN_CORE1_GLOBAL)) {
        REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG, HP_SYS_CLKRST_REG_RST_EN_CORE1_GLOBAL);
    }
#endif

#if SOC_KEY_MANAGER_SUPPORTED
    // The following operation makes the Key Manager to use eFuse key for ECDSA and XTS-AES operation by default
    // This is to keep the default behavior same as the other chips
    // If the Key Manager configuration is already locked then following operation does not have any effect
    key_mgr_hal_set_key_usage(ESP_KEY_MGR_ECDSA_KEY, ESP_KEY_MGR_USE_EFUSE_KEY);
    key_mgr_hal_set_key_usage(ESP_KEY_MGR_XTS_AES_128_KEY, ESP_KEY_MGR_USE_EFUSE_KEY);
#endif

    if (hp_subcore_boot_addr == 0) {
        return -1;
    }

    ets_set_appcpu_boot_addr((uint32_t)hp_subcore_boot_addr);
    return 0;
}

void esp_amp_platform_stop_subcore(void)
{
// #warning "esp_amp_platform_stop_subcore() not implemented"
    return;
}
#endif
