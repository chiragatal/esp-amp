/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SW_INTR_ID_0 = 0,
    SW_INTR_ID_1,
    SW_INTR_ID_2,
    SW_INTR_ID_3,
    SW_INTR_ID_4,
    SW_INTR_ID_5,
    SW_INTR_ID_6,
    SW_INTR_ID_7,
    SW_INTR_ID_8,
    SW_INTR_ID_9,
    SW_INTR_ID_10,
    SW_INTR_ID_11,
    SW_INTR_ID_12,
    SW_INTR_ID_13,
    SW_INTR_ID_14,
    SW_INTR_ID_15,
    SW_INTR_RESERVED_ID_16,
    SW_INTR_RESERVED_ID_17,
    SW_INTR_RESERVED_ID_18,
    SW_INTR_RESERVED_ID_19,
    SW_INTR_RESERVED_ID_20,
    SW_INTR_RESERVED_ID_21,
    SW_INTR_RESERVED_ID_22,
    SW_INTR_RESERVED_ID_23,
    SW_INTR_RESERVED_ID_24,
    SW_INTR_RESERVED_ID_25,
    SW_INTR_RESERVED_ID_26,
    SW_INTR_RESERVED_ID_27,
    SW_INTR_RESERVED_ID_28,
    SW_INTR_RESERVED_ID_VQUEUE,
    SW_INTR_RESERVED_ID_EVENT,
    SW_INTR_ID_MAX = 31,
} esp_amp_sw_intr_id_t;

/**
 * Software Interrupt Handler
 *
 * @param[in] arg argument registered with handler when calling esp_amp_sw_intr_add_handler()
 *
 * @retval 1 high priority tasks woken. Software interrupt calls portYield_from_ISR() if at least one handler's retval is 1.
 * @retval 0 no high priority tasks woken.
 */
typedef int (* esp_amp_sw_intr_handler_t)(void *arg);

/**
 * Register a software interrupt handler for a certain interrupt
 *
 * @param[in] intr_id identifier of the software interrupt
 * @param[in] handler interrupt handler for the software interrupt
 * @param[in] arg arg to be passed to handler. Users must ensure the validity of arg when it is used by handler
 */
int esp_amp_sw_intr_add_handler(esp_amp_sw_intr_id_t intr_id, esp_amp_sw_intr_handler_t handler, void *arg);

/**
 * Delete the software interrupt handler for a certain interrupt
 *
 * @param[in] intr_id identifier of the software interrupt
 * @param[in] handler interrupt handler for the software interrupt
 */
void esp_amp_sw_intr_delete_handler(esp_amp_sw_intr_id_t intr_id, esp_amp_sw_intr_handler_t handler);

/**
 * Trigger an software interrupt on peer core
 *
 * @param[in] intr_id identifier of the software interrupt
 */
void esp_amp_sw_intr_trigger(esp_amp_sw_intr_id_t intr_id);

/**
 * Dump the software interrupt handler table (for debug use)
 */
void esp_amp_sw_intr_handler_dump(void);


/**
 * Init software interrupt manager
 *
 * @retval 0 if successful
 * @retval -1 failed to init softare interrupt manager
 */
int esp_amp_sw_intr_init(void);

#ifdef __cplusplus
}
#endif
