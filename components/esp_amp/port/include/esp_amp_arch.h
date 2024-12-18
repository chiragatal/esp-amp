/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "stdint.h"
#include "riscv/rv_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int esp_amp_arch_get_core_id(void)
{
#ifdef __riscv
    return RV_READ_CSR(mhartid);
#endif
}

static inline void esp_amp_arch_memory_barrier(void)
{
#ifdef __riscv
    asm volatile("fence" ::: "memory");
#endif
}

static inline void esp_amp_arch_intr_enable(void)
{
#ifdef __riscv
    RV_SET_CSR(mstatus, MSTATUS_MIE);
#endif
}

static inline void esp_amp_arch_intr_disable(void)
{
#ifdef __riscv
    RV_CLEAR_CSR(mstatus, MSTATUS_MIE);
#endif
}

uint64_t esp_amp_arch_get_cpu_cycle(void);

#ifdef __cplusplus
}
#endif
