set(srcs
    "panic_handler.c"
    "syscalls.c"
)

if(IDF_TARGET STREQUAL "esp32p4")
    list(APPEND srcs
        "esp32p4/portasm.S"
        "esp32p4/start.S"
        "esp32p4/startup.c"

        "${IDF_PATH}/components/riscv/interrupt_clic.c"
        "${IDF_PATH}/components/riscv/interrupt.c"
        "${IDF_PATH}/components/riscv/vectors_clic.S"
        "${IDF_PATH}/components/riscv/vectors.S"
        "${IDF_PATH}/components/esp_system/port/arch/riscv/esp_ipc_isr_handler.S"
    )
endif()

idf_component_register(
    SRCS ${srcs}
    REQUIRES hal riscv esp_rom)

target_link_libraries(${COMPONENT_LIB} INTERFACE m)
