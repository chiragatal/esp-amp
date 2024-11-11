/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "sdkconfig.h"
#include "soc/interrupts.h"
#include "soc/soc_caps.h"
#include "riscv/rv_utils.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "hal/interrupt_clic_ll.h"
#include "hal/crosscore_int_ll.h"


/* rom serial port number */
#define CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM 0

/* vector table */
extern int _vector_table;
extern int _mtvt_table;

/* extern main function */
extern void main();

static void core_intr_matrix_clear(void)
{
    uint32_t core_id = esp_cpu_get_core_id();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
#if SOC_INT_CLIC_SUPPORTED
        interrupt_clic_ll_route(core_id, i, ETS_INVALID_INUM);
#else
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
#endif  // SOC_INT_CLIC_SUPPORTED
    }

#if SOC_INT_CLIC_SUPPORTED
    for (int i = 0; i < 32; i++) {
        /* Set all the CPU interrupt lines to vectored by default, as it is on other RISC-V targets */
        esprv_int_set_vectored(i, true);
    }
#endif // SOC_INT_CLIC_SUPPORTED
}


void subcore_startup()
{

#if SOC_BRANCH_PREDICTOR_SUPPORTED
    esp_cpu_branch_prediction_enable();
#endif  //#if SOC_BRANCH_PREDICTOR_SUPPORTED

    // esp_cpu_intr_set_ivt_addr(&_vector_table);
    rv_utils_set_mtvec((uint32_t)&_vector_table);

#if SOC_INT_CLIC_SUPPORTED
    /* When hardware vectored interrupts are enabled in CLIC,
     * the CPU jumps to this base address + 4 * interrupt_id.
     */
    esp_cpu_intr_set_mtvt_addr(&_mtvt_table);
    rv_utils_set_mtvt((uint32_t)&_mtvt_table);
#endif

    ets_set_appcpu_boot_addr(0);


    // bootloader_init_mem();

#if CONFIG_ESP_CONSOLE_NONE
    esp_rom_install_channel_putc(1, NULL);
    esp_rom_install_channel_putc(2, NULL);
#elif !CONFIG_ESP_CONSOLE_USB_CDC
    esp_rom_install_uart_printf();
    esp_rom_output_set_as_console(CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM);
#endif

    // Clear interrupt matrix for APP CPU core
    // Also set all interrupt to be vectored
    core_intr_matrix_clear();

    /* call main function */
#if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU
    rv_utils_enable_fpu();
#endif
    main();

    /* exit */
    exit(0);
}
