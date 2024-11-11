cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")

if(DEFINED ENV{ESP_AMP_PATH})
  set(ESP_AMP_PATH $ENV{ESP_AMP_PATH})
else()
  set(ESP_AMP_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif(DEFINED ENV{ESP_AMP_PATH})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# SUBCORE_APP_NAME and SUBCORE_PROJECT_DIR are defined in subcore_config.cmake
# essential to include this file before project() in unified build mode
include(${CMAKE_CURRENT_LIST_DIR}/subcore/subcore_config.cmake)

set(EXTRA_COMPONENT_DIRS
    ${ESP_AMP_PATH}/components
    ${CMAKE_CURRENT_LIST_DIR}/maincore
    ${SUBCORE_COMPONENT_DIRS}
)

project(esp_amp_rpmsg_send_recv)
