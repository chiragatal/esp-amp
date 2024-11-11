/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "stdint.h"
#include "esp_amp_arch.h"
#include "esp_amp_platform_log.h"

/**
 * Get cpu core id by reading register
 *
 * On RISC-V platform, this is done by reading the mhartid CSR
 *
 * @retval CORE_ID
 */
static inline int esp_amp_platform_get_core_id(void)
{
    return esp_amp_arch_get_core_id();
}

/**
 * Busy-looping delay for milli-second
 *
 * @param time delay duration (ms)
 */
void esp_amp_platform_delay_ms(uint32_t time);


/**
 * Busy-looping delay for micro-second
 *
 * @param time delay duration (us)
 */
void esp_amp_platform_delay_us(uint32_t time);


/**
 * Get current cpu cycle as time
 *
 * On RISC-V platform, this is done by reading mcycle & mcycleh CSR
 *
 * @retval current cpu cycle
 */
uint32_t esp_amp_platform_get_time_ms(void);


/**
 * Disable all interrupts on the current core
 *
 * On RISC-V platform, this is done by clearing MIE bit of MSTATUS
 */
void esp_amp_platform_intr_disable(void);

/**
 * Enable interrupts on the current core
 *
 * On RISC-V platform, this is done by setting MIE bit of MSTATUS
 */
void esp_amp_platform_intr_enable(void);

#if IS_MAIN_CORE

/**
 * Start subcore
 *
 * @retval 0 start subcore successfully
 * @retval -1 failed to start subcore
 */
int esp_amp_platform_start_subcore(void);


/**
 * Stop subcore
 */
void esp_amp_platform_stop_subcore(void);

#endif /* IS_MAIN_CORE */


#if IS_ULP_COCPU
#undef assert
#define assert(_stat) if(!(_stat)) { printf("%s:%d assertion failed\r\n", __FILE__, __LINE__); asm volatile ("ebreak\n"); }
#endif