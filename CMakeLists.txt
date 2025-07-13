# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the IDF_PATH if not already set
if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "/opt/esp/idf")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-crsfjr)