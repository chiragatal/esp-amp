if(NOT DEFINED IDF_PATH)
    set(IDF_PATH $ENV{IDF_PATH})
endif()

## set bootloader build components
set(COMPONENTS bootloader esptool_py esp_hw_support esp_system freertos hal partition_table soc bootloader_support log spi_flash micro-ecc main efuse esp_system newlib CACHE STRING "" FORCE)
set(common_req log esp_rom esp_common esp_hw_support hal CACHE STRING "" FORCE)

list(APPEND EXTRA_COMPONENT_DIRS
    ${IDF_PATH}/components/bootloader/subproject/components
    ${ESP_AMP_PATH}/components/esp_amp/idf_stub
    ${ESP_AMP_PATH}/components/esp_amp
)

# Include sdkconfig.h derived from the parent build.
include_directories(${CONFIG_DIR})

## enable idf bootloader build with customized toolchain flag
if(IDF_TARGET STREQUAL "esp32c6")
    set(TOOLCHAIN_FLAG_SUFFIX "esp-lp-rv32")
elseif(IDF_TARGET STREQUAL "esp32p4")
    set(TOOLCHAIN_FLAG_SUFFIX "esp-rv32")
endif()

set(TOOLCHAIN_FLAG ${ESP_AMP_PATH}/components/esp_amp/cmake/${IDF_TARGET}/toolchain-${TOOLCHAIN_FLAG_SUFFIX}.cmake CACHE PATH "" FORCE)
set(CMAKE_TOOLCHAIN_FILE ${TOOLCHAIN_FLAG} CACHE PATH "" FORCE)
set(BOOTLOADER_BUILD 1 CACHE BOOL "" FORCE)
include("${IDF_PATH}/tools/cmake/project.cmake")

## set build properties
# disable subcore sdkconfig output in unified build mode
# otherwise maincore sdkconfig will be overwritten by subcore build
if(UNIFIED_BUILD)
    message(STATUS "unified build mode, disable subcore sdkconfig output")
    idf_build_set_property(__OUTPUT_SDKCONFIG 0)
endif()
# NOTE: Helps to analyse the components built for the subcore binary by CMake Graphviz
idf_build_set_property(__BUILD_COMPONENT_DEPGRAPH_ENABLED 1)

## set compilation flags
idf_build_set_property(__COMPONENT_REQUIRES_COMMON "${common_req}")
idf_build_set_property(LINK_OPTIONS "-nostartfiles" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--no-warn-rwx-segments" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--gc-sections" APPEND)

# wrapper functions for printf
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=printf" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=putc" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=fprintf" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=vprintf" APPEND)
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=puts" APPEND)

# esp-amp related flags
idf_build_set_property(COMPILE_DEFINITIONS "-DIS_ENV_BM" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "BOOTLOADER_BUILD=1" APPEND)

idf_build_set_property(C_COMPILE_OPTIONS "-Wno-pointer-sign" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-discarded-qualifiers" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-implicit-fallthrough" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=implicit-function-declaration" APPEND)

## set target specific flags
if(IDF_TARGET STREQUAL "esp32c6")
    idf_build_set_property(COMPILE_DEFINITIONS "-DIS_ULP_COCPU" APPEND)
endif()

if(IDF_TARGET STREQUAL "esp32c6")
    idf_build_set_property(LINK_OPTIONS "-Wl,--defsym=__wrap_printf=lp_core_printf" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--defsym=__wrap_putc=lp_core_print_char" APPEND)
endif()

if(IDF_TARGET STREQUAL "esp32p4")
    idf_build_set_property(LINK_OPTIONS "-Wl,--defsym=__wrap_printf=esp_rom_printf" APPEND)
endif()

# override the __target_set_toolchain function in idf build system
function(__target_set_toolchain)
endfunction()
