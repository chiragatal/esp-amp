# subcore project CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

if(NOT SUBCORE_BUILD)
    message(STATUS "not subcore build, skipping event project")
    return()
endif()

message(STATUS "subcore build, processing event project")
include(${ESP_AMP_PATH}/components/esp_amp/cmake/subcore_project.cmake)

# SUBCORE_APP_NAME is defined in subcore_config.cmake
set(PROJECT_VER "1.0")
project(${SUBCORE_APP_NAME})
