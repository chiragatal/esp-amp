cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")

if(DEFINED ENV{ESP_AMP_PATH})
  set(ESP_AMP_PATH $ENV{ESP_AMP_PATH})
else()
  set(ESP_AMP_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../..)
endif(DEFINED ENV{ESP_AMP_PATH})

set(EXTRA_COMPONENT_DIRS
    ${ESP_AMP_PATH}/components
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_amp_separate_build)
