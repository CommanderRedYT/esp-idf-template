cmake_minimum_required(VERSION 3.16.3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASIO_DISABLE_CONCEPTS=1)


set(EXTRA_COMPONENT_DIRS
    esp-protocols/components
)

set(expected_build_folder "${CMAKE_CURRENT_SOURCE_DIR}/build")

project(example-project)
