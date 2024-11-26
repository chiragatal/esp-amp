set(includes
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/esp_rom/${IDF_TARGET}
)

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_EQUAL "5.3")
    list(APPEND includes
        ${IDF_PATH}/components/esp_rom/include/${IDF_TARGET}
    )
endif()

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER "5.3")
    list(APPEND includes
        ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/include
        ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/include/${IDF_TARGET}
    )
endif()

idf_component_register(INCLUDE_DIRS ${includes})

function(rom_linker_script name)
    target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}.rom.${name}.ld")
endfunction()

if(IDF_TARGET STREQUAL "esp32p4")
    target_linker_script(${COMPONENT_LIB} INTERFACE ${IDF_PATH}/components/esp_rom/${IDF_TARGET}/ld/${IDF_TARGET}.rom.ld)

    rom_linker_script("api")
    rom_linker_script("newlib")
    if(CONFIG_COMPILER_FLOAT_LIB_FROM_GCCLIB)
        rom_linker_script("libgcc")
    else()
        rom_linker_script("rvfp")
    endif()
endif()
