/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_amp.h"

/* params of rpc service: add */
typedef struct {
    int a;
    int b;
} add_params_in_t;

typedef struct {
    int ret;
} add_params_out_t;

typedef struct {

} say_hello_params_in_t;

typedef struct {

} say_hello_params_out_t;

typedef struct {
    int a;
    int b;
} timeout_params_in_t;

typedef struct {
    int ret;
} timeout_params_out_t;

/* generated rpc service types */
typedef enum {
    RPC_SERVICE_ADD,
    RPC_SERVICE_SAY_HELLO,
    RPC_SERVICE_TIMEOUT,
    RPC_SERVICE_INVALID,
} rpc_service_enum_t;


/* service handlers */
esp_amp_rpc_status_t rpc_service_add(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len);
esp_amp_rpc_status_t rpc_service_say_hello(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len);
esp_amp_rpc_status_t rpc_service_timeout(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len);
