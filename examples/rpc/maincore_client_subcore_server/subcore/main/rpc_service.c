/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "stdio.h"
#include "rpc_service.h"
#include "esp_amp_platform.h"

/* actual function definition */
int add(int a, int b)
{
    printf("executing add(%d, %d)\r\n", a, b);
    return a + b;
}

void say_hello(void)
{
    printf("executing say_hello\r\n");
    printf("hello\r\n");
}

int timeout(int a, int b)
{
    printf("executing timeout(%d, %d)\r\n", a, b);
    esp_amp_platform_delay_us(1000000);
    return a + b;
}

esp_amp_rpc_status_t rpc_service_add(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    if (params_in_len < sizeof(add_params_in_t)) {
        printf("params_in size doesnot match\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    if (*params_out_len < sizeof(add_params_out_t)) {
        printf("params_out cannot fit in buffer\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    add_params_in_t *add_params_in = (add_params_in_t *)params_in;
    add_params_out_t *add_params_out = (add_params_out_t *)params_out;

    printf("ADD param_in(%p): a(%p)=%d, b(%p)=%d\r\n",
           add_params_in, &add_params_in->a, add_params_in->a, &add_params_in->b, add_params_in->b);
    add_params_out->ret = add(add_params_in->a, add_params_in->b);
    *params_out_len = sizeof(add_params_out_t);

    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t rpc_service_say_hello(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    if (params_in_len < sizeof(say_hello_params_in_t)) {
        printf("params_in size doesnot match\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    if (*params_out_len < sizeof(say_hello_params_out_t)) {
        printf("params_out cannot fit in buffer\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    say_hello_params_in_t *say_hello_params_in = (say_hello_params_in_t *)params_in;
    say_hello();

    return ESP_AMP_RPC_STATUS_OK;
}

esp_amp_rpc_status_t rpc_service_timeout(void *params_in, uint16_t params_in_len, void *params_out, uint16_t *params_out_len)
{
    if (params_in_len < sizeof(timeout_params_in_t)) {
        printf("params_in size doesnot match\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    if (*params_out_len < sizeof(timeout_params_out_t)) {
        printf("params_out cannot fit in buffer\r\n");
        return ESP_AMP_RPC_STATUS_BAD_PACKET;
    }

    timeout_params_in_t *timeout_params_in = (timeout_params_in_t *)params_in;
    timeout_params_out_t *timeout_params_out = (timeout_params_out_t *)params_out;

    printf("TIMEOUT param_in(%p): a(%p)=%d, b(%p)=%d\r\n",
           timeout_params_in, &timeout_params_in->a, timeout_params_in->a, &timeout_params_in->b, timeout_params_in->b);
    timeout_params_out->ret = timeout(timeout_params_in->a, timeout_params_in->b);
    *params_out_len = sizeof(timeout_params_out_t);

    return ESP_AMP_RPC_STATUS_OK;
}
