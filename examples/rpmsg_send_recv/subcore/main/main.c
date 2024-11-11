/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <sdkconfig.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "event.h"

#define TAG "amp_init_test"

esp_amp_rpmsg_dev_t rpmsg_dev;
esp_amp_rpmsg_ept_t rpmsg_ept[3];

int ept0_cb(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    printf("[EPT%d]: %s\r\n", (int)(src_addr), (char*)msg_data);
    esp_amp_rpmsg_destroy_from_isr(&rpmsg_dev, msg_data);
    static int count = 0;
    void* msg = esp_amp_rpmsg_create_msg_from_isr(&rpmsg_dev, 30, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        snprintf(msg, 30, "Extra Msg: %d", count++);
        assert(esp_amp_rpmsg_send_nocopy_from_isr(&rpmsg_dev, &rpmsg_ept[0], 0, msg, 30) == 0);
    } else {
        printf("Failed to send extra msg!\r\n");
    }

    return 0;
}

int ept1_cb(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    printf("[RPC1]: Receive RPC(add) request from HP\r\n");
    int a = ((int*)(msg_data))[0];
    int b = ((int*)(msg_data))[1];
    int c = a + b;
    printf("Result: %d + %d = %d\r\n", a, b, c);
    esp_amp_rpmsg_destroy_from_isr(&rpmsg_dev, msg_data);

    void* msg = esp_amp_rpmsg_create_msg_from_isr(&rpmsg_dev, sizeof(int), ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        *((int*)(msg)) = c;
        assert(esp_amp_rpmsg_send_nocopy_from_isr(&rpmsg_dev, &rpmsg_ept[1], src_addr, msg, 1) == 0);
        printf("[RPC1]: Results sent\r\n");
    } else {
        printf("[RPC1]: FATAL! Failed to send results\r\n");
    }

    return 0;
}

int ept2_cb(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    printf("[RPC2]: Receive RPC(multiply) request from HP\r\n");
    int a = ((int*)(msg_data))[0];
    int b = ((int*)(msg_data))[1];
    int c = a * b;
    printf("Result: %d x %d = %d\r\n", a, b, c);
    esp_amp_rpmsg_destroy_from_isr(&rpmsg_dev, msg_data);

    void* msg = esp_amp_rpmsg_create_msg_from_isr(&rpmsg_dev, sizeof(int), ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        *((int*)(msg)) = c;
        assert(esp_amp_rpmsg_send_nocopy_from_isr(&rpmsg_dev, &rpmsg_ept[2], src_addr, msg, 1) == 0);
        printf("[RPC2]: Results sent\r\n");
    } else {
        printf("[RPC2]: FATAL! Failed to send results\r\n");
    }

    return 0;
}

void send_normal_msg(void)
{
    static int count = 0;
    static bool flip = true;
    void *msg = esp_amp_rpmsg_create_msg(&rpmsg_dev, 48, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        snprintf(msg, 48, "Normal Msg: %d", count++);
        assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[0], flip ? 0 : 1, msg, 48) == 0);
        flip = !flip;
    } else {
        printf("Failed to send msg!\r\n");
    }
}

int main(void)
{
    printf("Hello!!\r\n");

#ifdef CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE
    printf("Demonstrating interrupt-based RPMsg handling on subcore\r\n");
#else
    printf("Demonstrating polling-based RPMsg handling on subcore\r\n");
#endif /* CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE */

    assert(esp_amp_init() == 0);
#ifdef CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE
    /* interrupt */
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, false) == 0);
    assert(esp_amp_rpmsg_intr_enable(&rpmsg_dev) == 0);
#else
    /* polling */
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);
#endif /* CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE */

    esp_amp_rpmsg_create_ept(&rpmsg_dev, 0, ept0_cb, NULL, &rpmsg_ept[0]);
    esp_amp_rpmsg_create_ept(&rpmsg_dev, 1, ept1_cb, NULL, &rpmsg_ept[1]);
    esp_amp_rpmsg_create_ept(&rpmsg_dev, 2, ept2_cb, NULL, &rpmsg_ept[2]);

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    for (;;) {
#if !CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE
        /* needed by polling only */
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
#endif /* !CONFIG_EXAMPLE_RPMSG_ENABLE_INTERRUPT_ON_SUBCORE */
        send_normal_msg();
        esp_amp_platform_delay_us(1000000);
    }

    printf("Bye!!\r\n");
}
