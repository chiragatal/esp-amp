# skip building component in maincore project, only trigger sdkconfig generation
if(NOT SUBCORE_BUILD)
    idf_component_register()
    return()
endif()
