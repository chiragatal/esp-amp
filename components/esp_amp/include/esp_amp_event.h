/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send an event to notify peer core
 *
 * @param sysinfo_id sysinfo id of esp-amp event
 * @param bit_mask event to notify
 * @retval bit mask before notify
 */
uint32_t esp_amp_event_notify_by_id(uint16_t sysinfo_id, uint32_t bit_mask);

/**
 * Send an event to notify peer core
 *
 * @note a shortcut for default esp-amp event
 *
 * @param bit_mask bit_mask indicating certain event to notify
 */
#if IS_MAIN_CORE
#define esp_amp_event_notify(bit_mask) \
    esp_amp_event_notify_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, bit_mask)
#else
#define esp_amp_event_notify(bit_mask) \
    esp_amp_event_notify_by_id(SYS_INFO_RESERVED_ID_EVENT_SUB, bit_mask)
#endif /* IS_MAIN_CORE */

/**
 * Wait for event triggered by peer core
 *
 * @param sysinfo_id sysinfo id of esp-amp event
 * @param bit_mask bit mask indicating certain event to wait for
 * @param clear_on_exit clear event after exit or not
 * @param wait_for_all wait for all events or any event
 * @param timeout maximum wait time in millisecond before return -1
 * @retval event bitmask set by peer core
 */
uint32_t esp_amp_event_wait_by_id(uint16_t sysinfo_id, uint32_t bit_mask, bool clear_on_exit, bool wait_for_all, uint32_t timeout);

/**
 * Wait for an event triggered by peer core
 *
 * @note a shortcut for default esp-amp event
 *
 * @param bit_mask bit_mask indicating certain event to wait for
 * @param clear_on_exit clear event after exit or not
 * @param wait_for_all wait for all events or any event
 * @param timeout maximum wait time in millisecond before return -1
 * @retval event bits set by peer core
 */
#if IS_MAIN_CORE
#define esp_amp_event_wait(bit_mask, clear_on_exit, wait_for_all, timeout) \
    esp_amp_event_wait_by_id(SYS_INFO_RESERVED_ID_EVENT_SUB, bit_mask, clear_on_exit, wait_for_all, timeout)
#else
#define esp_amp_event_wait(bit_mask, clear_on_exit, wait_for_all, timeout) \
    esp_amp_event_wait_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, bit_mask, clear_on_exit, wait_for_all, timeout)
#endif /* IS_MAIN_CORE */

#if IS_ENV_BM
#define esp_amp_event_poll_by_id(sysinfo_id, bit_mask, clear_on_exit, wait_for_all) \
    esp_amp_event_wait_by_id(sysinfo_id, bit_mask, clear_on_exit, wait_for_all, 0)

#define esp_amp_event_poll(bit_mask, clear_on_exit, wait_for_all) \
    esp_amp_event_wait_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, bit_mask, clear_on_exit, wait_for_all, 0)
#endif /* IS_ENV_BM */


/**
 * Clear event bit mask
 *
 * @note be careful when using this API to clear bit mask, it may lead to missing event
 * @note use on waiting side only
 *
 * @param sysinfo_id sysinfo id of esp-amp event
 * @param bit_mask bit mask indicating certain event to clear
 * @retval bit mast before clear
 */
uint32_t esp_amp_event_clear_by_id(uint16_t sysinfo_id, uint32_t bit_mask);

#if !IS_ENV_BM
/**
 * Bind esp-amp event to an event object
 *
 * @note only use in freertos environment
 * @note different from baremetal environment where the caller of wait() polls for event bits
 * in a tight-loop to check if an event is set. In OS environment, the event bits can be set
 * bind esp-amp event to a OS-specific event object.
 *
 * @param sysinfo_id sysinfo id of esp-amp event
 * @param event_handle handle of event object
 * @retval 0 on success
 * @retval -1 on failure
 */
int esp_amp_event_bind_handle(uint16_t sysinfo_id, void *event_handle);
#endif //* IS_ENV_BM */


/**
 * Unbind esp-amp event from an event object
 *
 * @note cannot be called in baremetal
 *
 * @param sysinfo_id sysinfo id of esp-amp event
 */
#if !IS_ENV_BM
void esp_amp_event_unbind_handle(uint16_t sysinfo_id);
#endif //* IS_ENV_BM */

/**
 * Create event
 *
 * @note can only be called by maincore
 *
 * @param sysinfo_id sysinfo id to indicate new esp-amp event
 */
#if IS_MAIN_CORE
int esp_amp_event_create(uint16_t sysinfo_id);
#endif /* IS_MAIN_CORE */

/**
 * Dump event table entries
 *
 * @note for debug use only. cannot be called in baremetal
 */
#if !IS_ENV_BM
void esp_amp_event_table_dump(void);
#endif /* IS_ENV_BM */

/**
 * Init default esp amp event for internal use
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int esp_amp_event_init(void);

#ifdef __cplusplus
}
#endif
