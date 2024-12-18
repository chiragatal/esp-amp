/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_amp_arch.h"

typedef union {
    struct {
        uint32_t rv_mcycle;
        uint32_t rv_mcycleh;
    };
    uint64_t rv_mcycle_comb;
} esp_cpu_rv_mcycle_t;

uint64_t esp_amp_arch_get_cpu_cycle(void)
{
    esp_cpu_rv_mcycle_t cpu_cycle;
    uint32_t mcycleh_snapshot;

    /* double-check mcycleh: to avoid inconsistency when (mcycleh, mcycle) */
    /* changes from (0x0, 0xffff_ffff) to (0x1, 0x0000_0000) */
    mcycleh_snapshot = RV_READ_CSR(mcycleh);

    do {
        cpu_cycle.rv_mcycle = RV_READ_CSR(mcycle);
        cpu_cycle.rv_mcycleh = RV_READ_CSR(mcycleh);
    } while (mcycleh_snapshot != cpu_cycle.rv_mcycleh);

    return cpu_cycle.rv_mcycle_comb;
}
