cmake_minimum_required(VERSION 3.18)

project(nte_cafe_optimizer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(WEBUI_BUILD_EXAMPLES OFF)

add_subdirectory(lib/nte_cafe)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(lib/webui)
add_subdirectory(app)
