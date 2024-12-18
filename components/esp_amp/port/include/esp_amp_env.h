/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter critical section
 *
 * @note This function can be used to protect critical sections of code.
 * Nested critical section is allowed. The critical section will be
 * exited when the nesting count reaches 0.
 */
void esp_amp_env_enter_critical(void);

/**
 * @brief Exit critical section
 *
 * @note This function can be used to exit critical sections of code.
 * Ensure each call to esp_amp_env_enter_critical() is matched by a
 * call to esp_amp_env_exit_critical().
 */
void esp_amp_env_exit_critical(void);

/**
 * @brief Check if the current context is in interrupt service routine
 *
 * @retval 0 if the current context is not in interrupt service routine
 * @retval 1 if the current context is in interrupt service routine
 */
int esp_amp_env_in_isr(void);

#ifdef __cplusplus
}
#endif
