cmake_minimum_required(VERSION 3.16)
project(configparser)

include_directories(include)

add_library(configparser STATIC
        src/config_file_parser.cpp
        src/parse_file_exception.cpp
        src/parse_exception.cpp)

target_include_directories(configparser PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
if (NOT MSVC)
    set_target_properties(configparser PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
else()
    target_compile_definitions(configparser PUBLIC _USE_MATH_DEFINES)
endif()