/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>

#include "esp_amp.h"
#include "esp_attr.h"
#include "event.h"
#include "sys_info.h"
#include "esp_amp_platform.h"

int notify_func(void* args)
{
    esp_amp_sw_intr_trigger(SW_INTR_RESERVED_ID_VQUEUE);
    return ESP_OK;
}

int main(void)
{
    assert(esp_amp_init() == 0);
    esp_amp_queue_t vq;

    assert(esp_amp_queue_sub_init(&vq, notify_func, NULL, true, SYS_INFO_ID_VQUEUE_EXAMPLE) == 0);

    esp_amp_event_notify(EVENT_SUBCORE_READY);
    int idx = 0;

    while (1) {
        char *msg;
        int ret;
        printf("Alloc status: %x\n", ret = esp_amp_queue_alloc_try(&vq, (void**)(&msg), 32));
        printf("msg buffer ==> %p\n", msg);
        assert(msg != NULL && ret == ESP_OK);
        sprintf(msg, "message ==> %d\n", idx++);
        printf("Send status: %d\n", ret = esp_amp_queue_send_try(&vq, (void*)(msg), 32));
        assert(ret == ESP_OK);
        esp_amp_platform_delay_us(1000000);
    }

    printf("Bye!!\r\n");
    return 0;
}
