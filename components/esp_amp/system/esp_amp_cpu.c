/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "esp_amp_system.h"

#if CONFIG_ESP_AMP_SUBCORE_TYPE_HP_CORE
#include "rom/ets_sys.h"
#include "hal/cache_ll.h"
#include "hal/cpu_utility_ll.h"
#include "soc/hp_sys_clkrst_reg.h"

uint32_t hp_subcore_boot_addr;

int esp_amp_start_subcore(void)
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

void esp_amp_stop_subcore(void)
{
    return;
}

#endif /* CONFIG_ESP_AMP_SUBCORE_TYPE_HP_CORE */

#if CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE

#include "ulp_lp_core.h"
#include "lp_core_uart.h"

static void lp_uart_init(void)
{
    lp_core_uart_cfg_t cfg = LP_CORE_UART_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(lp_core_uart_init(&cfg));
    printf("LP UART initialized successfully\n");
}

int esp_amp_start_subcore(void)
{
    lp_uart_init();

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

void esp_amp_stop_subcore(void)
{
    ulp_lp_core_stop();
}

#endif /* CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE */
