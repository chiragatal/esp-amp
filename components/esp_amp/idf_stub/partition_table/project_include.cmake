function(partition_table_add_check_bootloader_size_target target_name)
    add_custom_target(${target_name}
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping size check for subcore"
        COMMENT "Skipping size check for subcore"
        DEPENDS ${CMD_DEPENDS}
    )
endfunction()
