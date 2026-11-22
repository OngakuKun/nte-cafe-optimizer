add_executable(nte-cafe-app
    src/main.c
)

target_link_libraries(nte-cafe-app
    PRIVATE
        nte_cafe
        webui
)

target_include_directories(nte-cafe-app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Keep UI files next to the executable for simple local runs.
add_custom_command(TARGET nte-cafe-app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
            $<TARGET_FILE_DIR:nte-cafe-app>/ui
)
