/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>

#include "esp_amp_platform.h"
#include "esp_amp.h"

#include "event.h"


const int data = 0;
const char* string = "sub_main";

int main(void)
{
    printf("Hello!!\r\n");
    assert(esp_amp_init() == 0);
    printf("main function at %p, data at %p, string at %p\r\n", main, &data, string);
    printf("Bye!!\r\n");
    return 0;
}
