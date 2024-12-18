set(ulp_lp_core_srcs
    "${IDF_PATH}/components/ulp/lp_core/lp_core/port/${IDF_TARGET}/vector_table.S"
    "${IDF_PATH}/components/ulp/lp_core/shared/ulp_lp_core_memory_shared.c"
    "${IDF_PATH}/components/ulp/lp_core/shared/ulp_lp_core_lp_timer_shared.c"
    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_startup.c"
    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_utils.c"

    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_uart.c"
    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_print.c"
    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_interrupt.c"
    "${IDF_PATH}/components/ulp/lp_core/lp_core/lp_core_i2c.c"

    "${IDF_PATH}/components/hal/uart_hal_iram.c"
    "${IDF_PATH}/components/hal/uart_hal.c"

    "vector.S"
    "start.S"
)

set(ulp_lp_core_includes
    "${IDF_PATH}/components/ulp/lp_core/lp_core/include"
    "${IDF_PATH}/components/ulp/lp_core/include"
    "${IDF_PATH}/components/ulp/lp_core/shared/include"
    "${IDF_PATH}/components/ulp/ulp_common/include"
)

idf_component_register(
    SRCS ${ulp_lp_core_srcs}
    INCLUDE_DIRS ${ulp_lp_core_includes}
    REQUIRES esp_common hal soc riscv
)

if(CONFIG_ESP_ROM_HAS_LP_ROM)
    target_link_options(${esp_amp_app_name}
        PRIVATE SHELL:-T ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}lp.rom.ld)
    target_link_options(${esp_amp_app_name}
        PRIVATE SHELL:-T ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}lp.rom.newlib.ld)
    target_link_options(${esp_amp_app_name}
        PRIVATE SHELL:-T ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}lp.rom.version.ld)
    target_link_options(${esp_amp_app_name}
        PRIVATE SHELL:-T ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}lp.rom.api.ld)
endif()

# suppress warning: implicit declaration of function 'abort'
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-function-declaration)

# suppress warning: incompatible implicit declaration of built-in function 'abort'
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-builtin-declaration-mismatch)
