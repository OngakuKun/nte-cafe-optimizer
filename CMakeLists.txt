cmake_minimum_required(VERSION 3.16)

project(nte_cafe_optimizer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib/nte_cafe)
add_subdirectory(tools)
add_subdirectory(tests)
