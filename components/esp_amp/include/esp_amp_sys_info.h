/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYS_INFO_ID_RESERVED = 0xff00,
    SYS_INFO_ID_SW_INTR,    /* reserved for software interrupt */
    SYS_INFO_ID_EVENT_MAIN, /* reserved for main core event */
    SYS_INFO_ID_EVENT_SUB,  /* reserved for sub core event */
    SYS_INFO_ID_SHARED_MEM, /* reserved for shared memory */
    SYS_INFO_ID_VQUEUE_TX,  /* store shared queue (packed virt queue) data structure (main TX, sub RX) */
    SYS_INFO_ID_VQUEUE_RX,  /* store shared queue (packed virt queue) data structure (main RX, sub TX) */
    SYS_INFO_ID_VQUEUE_BUFFER,

    SYS_INFO_ID_MAX = 0xffff, /* max number of sys info */
} esp_amp_sys_info_id_t;

/**
 * @brief Allocate sys info
 *
 * This API is intended for maincore to create necessary background information for dual-core communication
 *
 * @param info_id identifier for sys info data (0x0000 ~ 0xff00). 0xff00 ~ 0xffff is reserved for internal use
 * @param size size of sys info data needed
 *
 * @retval NULL failed to alloc sys info
 * @retval pointer to allocated shared memory region for sys info data
 */
void *esp_amp_sys_info_alloc(uint16_t info_id, uint16_t size);

/**
 * @brief Get sys info
 *
 * This API is intended for subcore to get background information created by maincore
 *
 * @param info_id identifier for sys info data (0x0000 ~ 0xff00). 0xff00 ~ 0xffff is reserved for internal use
 * @param size size of sys info data allocated by maincore
 *
 * @retval NULL failed to get sys info
 * @retval pointer to allocated shared memory region for sys info data
 */
void *esp_amp_sys_info_get(uint16_t info_id, uint16_t *size);

/**
 * Init sys info manager
 *
 * @retval 0 if successful
 * @retval failed to init sys info manager
 */
int esp_amp_sys_info_init(void);

/**
 * Dump sys info data (for debug use)
 */
void esp_amp_sys_info_dump(void);

#ifdef __cplusplus
}
#endif
