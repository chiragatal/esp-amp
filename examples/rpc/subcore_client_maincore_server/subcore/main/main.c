/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "sys_info.h"
#include "event.h"
#include "rpc_service.h"

#define TAG "rpc_s2m_test"

/* used by endpoint */
static esp_amp_rpmsg_dev_t rpmsg_dev;

static void rpc_add_cb(esp_amp_rpc_status_t status, void *params, uint16_t params_len)
{
    add_params_out_t add_params_out;

    if (status != ESP_AMP_RPC_STATUS_OK) {
        printf("Failed to execute ADD, err_code=%x\r\n", status);
    }

    else {
        if (sizeof(add_params_out_t) != params_len) {
            printf("Incomplete add_out_params\r\n");
        }

        memcpy(&add_params_out, params, params_len);
        printf("recv ADD()->%d\r\n", add_params_out.ret);
    }
}

static int rpc_srv_add(int a, int b)
{
    int rpc_ret;

    /* simple encoder */
    add_params_in_t add_params_in = {
        .a = a,
        .b = b,
    };

    /* first create request */
    esp_amp_rpc_req_handle_t req = esp_amp_rpc_client_create_request(RPC_SERVICE_ADD, &add_params_in, sizeof(add_params_in));

    if (req == NULL) {
        printf("Failed to create request\r\n");
        return -1;
    }

    /* second try to execute request */
    printf("send rpc request: ADD(%d, %d)\r\n", a, b);
    rpc_ret = esp_amp_rpc_client_execute_request_with_cb(req, rpc_add_cb, -1);
    if (rpc_ret != 0) {
        printf("Failed to send rpc request: ADD(%d, %d)\r\n", a, b);
    }

    return 0;
}

static void rpc_say_hello_cb(esp_amp_rpc_status_t status, void *params, uint16_t params_len)
{
    say_hello_params_out_t say_hello_params_out;

    if (status != ESP_AMP_RPC_STATUS_OK) {
        printf("Failed to execute TIMEOUT, err_code=%x\r\n", status);
    }

    else {
        if (sizeof(say_hello_params_out_t) != params_len) {
            printf("Incomplete say_hello_params\r\n");
        }

        memcpy(&say_hello_params_out, params, params_len);
        printf("recv rpc response SAY_HELLO\r\n");
    }
}

static int rpc_srv_say_hello(void)
{
    int rpc_ret;

    say_hello_params_in_t say_hello_params_in = {

    };

    /* first create request */
    esp_amp_rpc_req_handle_t req = esp_amp_rpc_client_create_request(RPC_SERVICE_SAY_HELLO, &say_hello_params_in, sizeof(say_hello_params_in));

    if (req == NULL) {
        printf("Failed to create request\r\n");
        return -1;
    }

    /* second try to execute request */
    printf("send rpc request: SAY_HELLO()\r\n");
    rpc_ret = esp_amp_rpc_client_execute_request_with_cb(req, rpc_say_hello_cb, -1);
    if (rpc_ret != 0) {
        printf("Failed to send rpc request: SAY_HELLO\r\n");
    }

    return 0;

}

static void rpc_timeout_cb(esp_amp_rpc_status_t status, void *params, uint16_t params_len)
{
    timeout_params_out_t timeout_params_out;

    if (status != ESP_AMP_RPC_STATUS_OK) {
        printf("Failed to execute TIMEOUT, err_code=%x\r\n", status);
    }

    else {
        if (sizeof(timeout_params_out_t) != params_len) {
            printf("Incomplete add_out_params\r\n");
        }

        memcpy(&timeout_params_out, params, params_len);
        printf("recv TIMEOUT()->%d\r\n", timeout_params_out.ret);
    }
}

static int rpc_srv_timeout(int a, int b)
{
    int rpc_ret;

    /* simple encoder */
    timeout_params_in_t timeout_params_in = {
        .a = a,
        .b = b,
    };

    /* first create request */
    esp_amp_rpc_req_handle_t req = esp_amp_rpc_client_create_request(RPC_SERVICE_TIMEOUT, &timeout_params_in, sizeof(timeout_params_in));

    if (req == NULL) {
        printf("Failed to create TIMEOUT request\r\n");
        return -1;
    }

    /* second try to execute request */
    printf("send rpc request: TIMEOUT(%d, %d)\r\n", a, b);
    rpc_ret = esp_amp_rpc_client_execute_request_with_cb(req, rpc_timeout_cb, 100);
    if (rpc_ret != 0) {
        printf("Failed to send rpc request: TIMEOUT(%d, %d)\r\n", a, b);
    }

    return 0;
}

int main(void)
{
    printf("Hello from the Sub core!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);
    assert(esp_amp_rpc_client_init(&rpmsg_dev, RPC_SUB_CORE_CLIENT, RPC_MAIN_CORE_SERVER) == 0);

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    for (int i = 0; i < 10; i++) {
        rpc_srv_add(i, i + 1);

        /* to test the timeout feature, uncomment the line below */
        // rpc_srv_timeout(i, i+1);
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
        esp_amp_rpc_client_complete_timeout_request();
        esp_amp_platform_delay_us(100000);
    }

    /* wait for more response */
    while (true) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
        esp_amp_rpc_client_complete_timeout_request();
        esp_amp_platform_delay_us(100000);
    }

    printf("Bye from the Sub core!!\r\n");
    abort();
}
