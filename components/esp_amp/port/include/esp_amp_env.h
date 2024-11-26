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
 */
void esp_amp_env_enter_critical();

/**
 * @brief Enter critical section in ISR
 */
void esp_amp_env_enter_critical_isr();

/**
 * @brief Exit critical section
 */
void esp_amp_env_exit_critical();

/**
 * @brief Exit critical section in ISR
 */
void esp_amp_env_exit_critical_isr();

#ifdef __cplusplus
}
#endif
