/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "sdkconfig.h"

#if IS_MAIN_CORE
uint16_t get_aligned_size(uint16_t size)
{
    typeof(size) size_in_word = size >> 2;
    if (size & 0x3) {
        size_in_word += 1;
    }
    return (size_in_word << 2);
}

uint16_t get_power_len(uint16_t len)
{
    typeof(len) max_bit = (typeof(len))((sizeof(len) << 3));
    typeof(len) max_len = (typeof(len))((1 << (max_bit - 1)));
    if ((len == 0) || (len > max_len)) {
        /* must fullfill 0 < `len` <= 2^(15) */
        return 0;
    }
    /* in case that the `len` is already power of 2 */
    len -= 1;
    return 1 << ((sizeof(unsigned int) << 3) - (__builtin_clz((unsigned int)(len))));
}
#endif /* IS_MAIN_CORE */
