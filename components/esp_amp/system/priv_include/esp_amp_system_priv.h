/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if IS_MAIN_CORE
/**
 * Initialize esp amp panic
 *
 * @retval 0 if successful
 * @retval -1 if failed
 */
int esp_amp_system_panic_init(void);

#endif

#ifdef __cplusplus
}
#endif
