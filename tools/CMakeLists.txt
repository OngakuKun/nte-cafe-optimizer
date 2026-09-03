add_executable(nte-cafe-cli
    cli.c
    cli_output.c
)

target_link_libraries(nte-cafe-cli
    PRIVATE
        nte_cafe
)

target_include_directories(nte-cafe-cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
