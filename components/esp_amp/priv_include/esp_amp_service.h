/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_SERVICE_ID_PRINT 0x0001

#ifdef __cplusplus
extern "C" {
#endif

int esp_amp_system_service_init(void);

#if !IS_MAIN_CORE
/**
 * @brief Create a system service request
 * @note can only be called in subcore
 *
 * @param buf pointer to the buffer to store request data
 * @param max_len The maximum length of the buffer
 */
int esp_amp_system_service_create_request(void **buf, uint16_t *max_len);

/**
 * @brief Send request to maincore
 * @note can only be called in subcore
 *
 * @param id service id
 * @param param buffer pointer to the request data
 * @param param_len The length of the request data
 */
int esp_amp_system_service_send_request(uint16_t id, void* param, uint16_t param_len);

#else

/**
 * @brief Receive request from subcore
 * @note can only be called in maincore
 *
 * @param id service id
 * @param param buffer pointer to the request data
 * @param param_len The length of the request data
 */
int esp_amp_system_service_recv_request(uint16_t *id, void** param, uint16_t *param_len);

/**
 * @brief Destroy a request
 * @note can only be called in maincore
 *
 * @param buf buffer pointer to the request data
 */
int esp_amp_system_service_destroy_request(void* buf);


/**
 * @brief Get task handle of esp amp subcore supplicant
 *
 * @note can only be called in maincore
 */
void *esp_amp_system_get_supplicant(void);

#endif

/**
 * @brief Check if the system service is ready
 *
 * @retval true if the system service is ready
 * @retval false if not
 */
bool esp_amp_system_service_is_ready(void);

#ifdef __cplusplus
}
#endif
