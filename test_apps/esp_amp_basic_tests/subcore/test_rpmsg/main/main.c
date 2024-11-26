/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>

#include "esp_amp.h"

esp_amp_rpmsg_dev_t rpmsg_dev;
esp_amp_rpmsg_ept_t rpmsg_ept[3];

int ept0_cb(void* msg_data, uint16_t data_len, uint16_t src_addr, void* rx_cb_data)
{
    printf("[EPT%d]: %s\r\n", (int)(src_addr), (char*)msg_data);
    esp_amp_rpmsg_destroy(&rpmsg_dev, msg_data);
    static int count = 0;
    void* msg = esp_amp_rpmsg_create_message(&rpmsg_dev, 30, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        snprintf(msg, 30, "Extra Msg: %d", count++);
        assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[0], 0, msg, 30) == 0);
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
    esp_amp_rpmsg_destroy(&rpmsg_dev, msg_data);

    printf("Result: %d + %d = %d\r\n", a, b, c);
    void* msg = esp_amp_rpmsg_create_message(&rpmsg_dev, sizeof(int), ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        *((int*)(msg)) = c;
        assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[1], src_addr, msg, 1) == 0);
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
    esp_amp_rpmsg_destroy(&rpmsg_dev, msg_data);

    printf("Result: %d x %d = %d\r\n", a, b, c);

    void* msg = esp_amp_rpmsg_create_message(&rpmsg_dev, sizeof(int), ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        *((int*)(msg)) = c;
        assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[2], src_addr, msg, 1) == 0);
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
    void *msg = esp_amp_rpmsg_create_message(&rpmsg_dev, 48, ESP_AMP_RPMSG_DATA_DEFAULT);
    if (msg != NULL) {
        snprintf(msg, 48, "Normal Msg: %d", count++);
        // printf("sending msg ==> %s\r\n", msg);
        assert(esp_amp_rpmsg_send_nocopy(&rpmsg_dev, &rpmsg_ept[0], 1, msg, 48) == 0);
        flip = !flip;
    } else {
        printf("Failed to send msg!\r\n");
    }
}

int main(void)
{
    printf("Hello!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);

    esp_amp_rpmsg_create_endpoint(&rpmsg_dev, 0, ept0_cb, NULL, &rpmsg_ept[0]);
    esp_amp_rpmsg_create_endpoint(&rpmsg_dev, 1, ept1_cb, NULL, &rpmsg_ept[1]);
    esp_amp_rpmsg_create_endpoint(&rpmsg_dev, 2, ept2_cb, NULL, &rpmsg_ept[2]);
    printf("Sub started!\r\n");
    send_normal_msg();
    for (;;) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
    }

    abort();
}
