/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "sys_info.h"
#include "event.h"
#include "rpc_service.h"

#define TAG "main"

extern esp_amp_rpc_service_t services[3];
static esp_amp_rpmsg_dev_t rpmsg_dev;

int main(void)
{
    printf("Hello from the Sub-core!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);
    assert(esp_amp_rpc_server_init(&rpmsg_dev, RPC_MAIN_CORE_CLIENT, RPC_SUB_CORE_SERVER) == 0);
    assert(esp_amp_rpc_server_add_service(RPC_SERVICE_ADD, rpc_service_add) == 0);
    assert(esp_amp_rpc_server_add_service(RPC_SERVICE_SAY_HELLO, rpc_service_say_hello) == 0);
    assert(esp_amp_rpc_server_add_service(RPC_SERVICE_TIMEOUT, rpc_service_timeout) == 0);

    printf("rpc server init successfully\r\n");

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    int i = 0;
    while (true) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);

        if (i % 1000 == 999) {
            printf("running...\r\n");
        }
        esp_amp_platform_delay_us(1000);
        i++;
    }

    printf("Bye from the Sub core!!\r\n");
    abort();
}
