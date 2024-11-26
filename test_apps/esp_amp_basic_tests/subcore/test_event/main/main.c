#include <stdint.h>

#include "esp_amp.h"
#include "esp_bit_defs.h"
#include <stdio.h>

#define TAG "amp_init_test"

/* ESP-AMP event */
#define SYS_INFO_ID_EVENT_TEST_MAIN 0x0000
#define SYS_INFO_ID_EVENT_TEST_SUB 0x0001
#define EVENT_SUBCORE_READY  (BIT0 | BIT1 | BIT2 | BIT3)
#define EVENT_MAINCORE_READY (BIT0 | BIT1 | BIT2 | BIT3)

#define ESP_AMP_EVENT_1 (BIT0 | BIT4)
#define ESP_AMP_EVENT_2 (BIT1 | BIT5)
#define ESP_AMP_EVENT_3 (BIT2 | BIT6)
#define ESP_AMP_EVENT_4 (BIT3 | BIT7)

int main(void)
{
    printf("Hello!!\r\n");

    assert(esp_amp_init() == 0);

    /* wait for maincore ready */
    uint32_t event_bits = 0;
    event_bits = esp_amp_event_wait(~EVENT_MAINCORE_READY, false, true, 1000);
    assert(event_bits == EVENT_MAINCORE_READY);

    event_bits = esp_amp_event_wait(EVENT_MAINCORE_READY, false, true, 1000);
    if ((event_bits & EVENT_MAINCORE_READY) == EVENT_MAINCORE_READY) {
        uint32_t maincore_event_bit = esp_amp_event_clear_by_id(SYS_INFO_RESERVED_ID_EVENT_MAIN, EVENT_MAINCORE_READY);
        assert(maincore_event_bit == EVENT_MAINCORE_READY);
        esp_amp_event_notify(EVENT_SUBCORE_READY);
    } else {
        printf("timeout maincore ready\r\n");
        abort();
    }

    printf("wait for event 1\r\n");
    uint32_t event_bits_1 = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_MAIN, ESP_AMP_EVENT_1, true, true, 5000);
    if ((event_bits_1 & ESP_AMP_EVENT_1) == ESP_AMP_EVENT_1) {
        printf("recv event 1\r\n");
        esp_amp_event_notify_by_id(SYS_INFO_ID_EVENT_TEST_SUB, ESP_AMP_EVENT_1);
    } else {
        printf("timeout event 1: %p\r\n", (void *)event_bits_1);
    }

    printf("wait for event 2\r\n");
    uint32_t event_bits_2 = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_MAIN, ESP_AMP_EVENT_2, true, true, 5000);
    if ((event_bits_2 & ESP_AMP_EVENT_2) == ESP_AMP_EVENT_2) {
        printf("recv event 2\r\n");
        esp_amp_event_notify_by_id(SYS_INFO_ID_EVENT_TEST_SUB, ESP_AMP_EVENT_2);
    } else {
        printf("timeout event 2: %p\r\n", (void *)event_bits_2);
    }

    printf("wait for event 3\r\n");
    uint32_t event_bits_3 = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_MAIN, ESP_AMP_EVENT_3, true, true, 5000);
    if ((event_bits_3 & ESP_AMP_EVENT_3) == ESP_AMP_EVENT_3) {
        printf("recv event 3\r\n");
        esp_amp_event_notify_by_id(SYS_INFO_ID_EVENT_TEST_SUB, ESP_AMP_EVENT_3);
    } else {
        printf("timeout event 3: %p\r\n", (void *)event_bits_3);
    }

    printf("wait for event 4\r\n");
    uint32_t event_bits_4 = esp_amp_event_wait_by_id(SYS_INFO_ID_EVENT_TEST_MAIN, ESP_AMP_EVENT_4, true, true, 5000);
    if ((event_bits_4 & ESP_AMP_EVENT_4) == ESP_AMP_EVENT_4) {
        printf("recv event 4\r\n");
        esp_amp_event_notify_by_id(SYS_INFO_ID_EVENT_TEST_SUB, ESP_AMP_EVENT_4);
    } else {
        printf("timeout event 4: %p\r\n", (void *)event_bits_4);
    }

    printf("Bye!!\r\n");
    while (1);
}
