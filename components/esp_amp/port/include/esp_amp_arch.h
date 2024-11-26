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
    return RV_READ_CSR(mhartid);
}

uint64_t esp_amp_arch_get_cpu_cycle(void);

static inline void esp_amp_arch_memory_barrier(void)
{
#ifdef __riscv
    asm volatile("fence" ::: "memory");
#endif
}

#ifdef __cplusplus
}
#endif
