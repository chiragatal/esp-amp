function(esp_amp_add_subcore_project subcore_app_name subcore_project_dir)

    set(options EMBED PARTITION)
    set(single_value TYPE SUBTYPE)
    set(multi_value )
    cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

    idf_build_get_property(idf_path IDF_PATH)
    idf_build_get_property(idf_target IDF_TARGET)
    idf_build_get_property(build_dir BUILD_DIR)
    idf_build_get_property(sdkconfig SDKCONFIG)
    idf_build_get_property(python PYTHON)
    idf_build_get_property(extra_cmake_args EXTRA_CMAKE_ARGS)
    idf_build_get_property(config_dir CONFIG_DIR)

    get_filename_component(subcore_binary_dir ${subcore_project_dir} NAME)
    set(SUBCORE_BUILD_DIR "${build_dir}/${subcore_binary_dir}")
    set(subcore_binary_files
        "${SUBCORE_BUILD_DIR}/${subcore_app_name}.elf"
        "${SUBCORE_BUILD_DIR}/${subcore_app_name}.bin"
        "${SUBCORE_BUILD_DIR}/${subcore_app_name}.map"
        )

    externalproject_add(${subcore_app_name}
        SOURCE_DIR "${subcore_project_dir}"
        BINARY_DIR "${SUBCORE_BUILD_DIR}"
        CMAKE_ARGS  -DSDKCONFIG=${sdkconfig} -DIDF_PATH=${idf_path} -DIDF_TARGET=${idf_target}
        -DCONFIG_DIR=${config_dir} -DSUBCORE_BUILD=ON -DSUBCORE_APP_NAME=${subcore_app_name}
        -DESP_AMP_PATH=${ESP_AMP_PATH} -DUNIFIED_BUILD=ON
        ${extra_cmake_args}
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1  # no easy way around this...
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        BUILD_BYPRODUCTS ${subcore_binary_files}
        )

    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES
    ${subcore_binary_files})

    if(__EMBED)
        if(NOT CMAKE_BUILD_EARLY_EXPANSION)
            add_dependencies(${COMPONENT_LIB} ${subcore_app_name})
            target_add_binary_data(${COMPONENT_LIB} "${SUBCORE_BUILD_DIR}/${subcore_app_name}.bin" BINARY)
        endif()
    endif(__EMBED)
    
    if(__PARTITION)
        if(__TYPE STREQUAL "" OR __SUBTYPE STREQUAL "")
            message(FATAL_ERROR "partition type and subtype must be provided")
        endif()

        set(partition_type ${__TYPE})
        set(partition_subtype ${__SUBTYPE})

        if(NOT CMAKE_BUILD_EARLY_EXPANSION)
            # Verify whether the subcore partition has sufficient space to accommodate the subcore binary
            partition_table_add_check_size_target(app_check_subcore_size
                DEPENDS ${subcore_app_name}
                BINARY_PATH "${SUBCORE_BUILD_DIR}/${subcore_app_name}.bin"
                PARTITION_TYPE ${partition_type} PARTITION_SUBTYPE ${partition_subtype})
            add_dependencies(app app_check_subcore_size)

            add_dependencies(${subcore_app_name} partition_table_bin)
            add_dependencies(flash ${subcore_app_name})
            partition_table_get_partition_info(partition "--partition-type ${partition_type} --partition-subtype ${partition_subtype}" "name")
            set(image_file ${SUBCORE_BUILD_DIR}/${subcore_app_name}.bin)
            partition_table_get_partition_info(offset "--partition-name ${partition}" "offset")
            esptool_py_flash_target_image(flash "${partition}" "${offset}" "${image_file}")
        endif()
    endif(__PARTITION)

endfunction()
