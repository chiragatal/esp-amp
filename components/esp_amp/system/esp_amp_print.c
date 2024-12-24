/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include <stdio.h>
#include <stdarg.h>

#if !IS_MAIN_CORE
#include <string.h>
#include "esp_amp_platform.h"
#include "esp_amp_service.h"

#if CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE
#include "ulp_lp_core_print.h"
#else
#include "esp_rom_uart.h"
#endif /* CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE */

#define is_digit(c) ((c >= '0') && (c <= '9'))

static int _cvt(unsigned long long val, char *buf, long radix, const char *digits)
{
    char temp[64];
    char *cp = temp;
    int length = 0;

    if (val == 0) {
        /* Special case */
        *cp++ = '0';
    } else {
        while (val) {
            *cp++ = digits[val % radix];
            val /= radix;
        }
    }
    while (cp != temp) {
        *buf++ = *--cp;
        length++;
    }
    *buf = '\0';
    return (length);
}

static int subcore_vprintf(void (*putc)(char c), const char *fmt, va_list ap)
{
    char buf[sizeof(long long) * 8];

    char c, sign, *cp = buf;
    int left_prec, right_prec, zero_fill, pad, pad_on_right,
        islong, islonglong, i;
    long long val = 0;
    int res = 0, length = 0;

    while ((c = *fmt++) != '\0') {
        if (c == '%') {
            c = *fmt++;
            left_prec = right_prec = pad_on_right = islong = islonglong = 0;
            if (c == '-') {
                c = *fmt++;
                pad_on_right++;
            }
            if (c == '0') {
                zero_fill = true;
                c = *fmt++;
            } else {
                zero_fill = false;
            }
            while (is_digit(c)) {
                left_prec = (left_prec * 10) + (c - '0');
                c = *fmt++;
            }
            if (c == '.') {
                c = *fmt++;
                zero_fill = false;
                while (is_digit(c)) {
                    right_prec = (right_prec * 10) + (c - '0');
                    c = *fmt++;
                }
            }
            sign = '\0';
            if (c == 'l') {
                c = *fmt++;
                islong = 1;
                if (c == 'l') {
                    c = *fmt++;
                    islonglong = 1;
                    islong = 0;
                }
            }
            switch (c) {
            case 'p':
                islong = 1;
            /* fall through */
            case 'd':
            case 'D':
            case 'x':
            case 'X':
            case 'u':
            case 'U':
            case 'b':
            case 'B':
                if (islonglong) {
                    val = va_arg(ap, long long);
                } else if (islong) {
                    val = (long long)va_arg(ap, long);
                } else {
                    val = (long long)va_arg(ap, int);
                }

                if ((c == 'd') || (c == 'D')) {
                    if (val < 0) {
                        sign = '-';
                        val = -val;
                    }
                } else {
                    if (islonglong) {
                        ;
                    } else if (islong) {
                        val &= ((long long)1 << (sizeof(long) * 8)) - 1;
                    } else {
                        val &= ((long long)1 << (sizeof(int) * 8)) - 1;
                    }
                }
                break;
            default:
                break;
            }

            switch (c) {
            case 'p':
                (*putc)('0');
                (*putc)('x');
                zero_fill = true;
                left_prec = sizeof(unsigned long) * 2;
            /* fall through */
            case 'd':
            case 'D':
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                switch (c) {
                case 'd':
                case 'D':
                case 'u':
                case 'U':
                    length = _cvt(val, buf, 10, "0123456789");
                    break;
                case 'p':
                case 'x':
                    length = _cvt(val, buf, 16, "0123456789abcdef");
                    break;
                case 'X':
                    length = _cvt(val, buf, 16, "0123456789ABCDEF");
                    break;
                }
                cp = buf;
                break;
            case 's':
            case 'S':
                cp = va_arg(ap, char *);
                if (cp == NULL)  {
                    cp = (char *)"<null>";
                }
                length = 0;
                while (cp[length] != '\0') {
                    length++;
                }
                if (right_prec) {
                    length = MIN(right_prec, length);
                    right_prec = 0;
                }
                break;
            case 'c':
            case 'C':
                c = va_arg(ap, int /*char*/);
                (*putc)(c);
                res++;
                continue;
            case 'b':
            case 'B':
                length = left_prec;
                if (left_prec == 0) {
                    if (islonglong) {
                        length = sizeof(long long) * 8;
                    } else if (islong) {
                        length = sizeof(long) * 8;
                    } else {
                        length = sizeof(int) * 8;
                    }
                }
                for (i = 0;  i < length - 1;  i++) {
                    buf[i] = ((val & ((long long)1 << i)) ? '1' : '.');
                }
                cp = buf;
                break;
            case '%':
                (*putc)('%');
                break;
            default:
                (*putc)('%');
                (*putc)(c);
                res += 2;
            }
            pad = left_prec - length;
            right_prec = right_prec - length;
            if (right_prec > 0) {
                pad -= right_prec;
            }
            if (sign != '\0') {
                pad--;
            }
            if (zero_fill) {
                c = '0';
                if (sign != '\0') {
                    (*putc)(sign);
                    res++;
                    sign = '\0';
                }
            } else {
                c = ' ';
            }
            if (!pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(c);
                    res++;
                }
            }
            if (sign != '\0') {
                (*putc)(sign);
                res++;
            }
            while (right_prec-- > 0) {
                (*putc)('0');
                res++;
            }
            while (length-- > 0) {
                c = *cp++;
                (*putc)(c);
                res++;
            }
            if (pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(' ');
                    res++;
                }
            }
        } else {
            (*putc)(c);
            res++;
        }
    }
    return (res);
}

void subcore_uart_putchar(char c)
{
#if CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE
    lp_core_print_char(c);
#else
    esp_rom_output_putc(c);
#endif
}

int esp_amp_subcore_early_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int prt_bytes;
    prt_bytes = subcore_vprintf(subcore_uart_putchar, format, args);
    va_end(args);

    return prt_bytes;
}

#if CONFIG_ESP_AMP_ROUTE_SUBCORE_PRINT
void __attribute__((alias("virtqueue_send_char"))) subcore_putchar(char c);
static void virtqueue_send_char(char c)
{
    static char* buf = NULL;
    static uint16_t buf_len = 0;
    static uint16_t pos = 0;

    // ignore '\0'
    if (c == '\0') {
        return;
    }

    if (buf == NULL) {
        if (!esp_amp_system_service_is_ready()) {
            subcore_uart_putchar(c);
            return;
        }

        /* create buffer, try maximum 10ms */
        if (esp_amp_system_service_create_request((void **)&buf, &buf_len) != 0) {
            uint32_t cur_time = esp_amp_platform_get_time_ms();
            while (esp_amp_platform_get_time_ms() - cur_time < 10) {
                if (esp_amp_system_service_create_request((void **)&buf, &buf_len) == 0) {
                    break;
                }
            }
        }

        /* if still failed, drop the log */
        if (buf == NULL) {
            return;
        }
    }

    buf[pos++] = c;
    if ((pos == buf_len - 1) || (c == '\n')) {
        buf[pos] = '\0';
        if (pos >= 1 && buf[pos - 1] == '\n') {
            buf[pos - 1] = '\0';
        } else if (pos >= 2 && buf[pos - 2] == '\r' && buf[pos - 1] == '\n') {
            buf[pos - 2] = '\0';
        }
        esp_amp_system_service_send_request(SYSTEM_SERVICE_ID_PRINT, buf, pos + 1);
        // release the sent buf
        buf = NULL;
        pos = 0;
    }
}
#else
#if CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE

void __attribute__((alias("lp_subcore_send_char"))) subcore_putchar(char c);
static void lp_subcore_send_char(char c)
{
    lp_core_print_char(c);
}
#else
void __attribute__((alias("hp_subcore_send_char"))) subcore_putchar(char c);
static void hp_subcore_send_char(char c)
{
    esp_rom_output_putc(c);
}
#endif /* CONFIG_ESP_AMP_SUBCORE_TYPE_LP_CORE */
#endif /* CONFIG_ESP_AMP_ROUTE_SUBCORE_PRINT */

int esp_amp_subcore_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int prt_bytes;
    prt_bytes = subcore_vprintf(subcore_putchar, format, args);
    va_end(args);

    return prt_bytes;
}

int esp_amp_subcore_puts(const char *s)
{
    while (*s != '\0') {
        subcore_putchar(*s);
        s++;
    }
    return 0;
}

int esp_amp_subcore_putchar(int ch)
{
    subcore_putchar(ch);
    return ch;
}

#else /* !IS_MAIN_CORE */
#if CONFIG_ESP_AMP_ROUTE_SUBCORE_PRINT
#include "stdio.h"

void esp_amp_system_service_print(void *param, uint16_t param_len)
{
    puts((char *)param);
}

#endif /* CONFIG_ESP_AMP_ROUTE_SUBCORE_PRINT */
#endif /* IS_MAIN_CORE */
