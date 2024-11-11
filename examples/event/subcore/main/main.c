/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "event.h"
#include "sys_info.h"

#define TAG "amp_init_test"

int main(void)
{
    printf("SUBCORE: Hello!!\r\n");

    assert(esp_amp_init() == 0);

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    int cnt = 0;
    while (1) {
        int ret;
        printf("SUBCORE: polling EVENT_MAINCORE_EVENT\r\n");
        ret = esp_amp_event_poll_by_id(SYS_INFO_ID_MAINCORE_EVENT, EVENT_MAINCORE_EVENT, true, true);
        if ((ret & EVENT_MAINCORE_EVENT) == EVENT_MAINCORE_EVENT) {
            printf("SUBCORE: recv EVENT_MAINCORE_EVENT\r\n");
        }

        /* each time notify different event */
        if (cnt % 2 == 0) {
            printf("SUBCORE: notifying EVENT_SUBCORE_EVENT_1\r\n");
            esp_amp_event_notify_by_id(SYS_INFO_ID_SUBCORE_EVENT, EVENT_SUBCORE_EVENT_1);
        } else {
            printf("SUBCORE: notifying EVENT_SUBCORE_EVENT_2\r\n");
            esp_amp_event_notify_by_id(SYS_INFO_ID_SUBCORE_EVENT, EVENT_SUBCORE_EVENT_2);
        }
        cnt++;

        esp_amp_platform_delay_us(1000000);
        printf("SUBCORE: running...\r\n");
    }

    printf("SUBCORE: Bye!!\r\n");
    return 0;
}
