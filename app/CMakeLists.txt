# Server build: serves the WebUI over HTTP (used by the Docker deployment).
add_executable(nte-cafe-app
    src/main.c
)

# Desktop build: same source, UI and config.json, but opens a native WebView
# window instead of starting an HTTP server.
add_executable(nte-cafe-desktop
    src/main.c
)

foreach(target nte-cafe-app nte-cafe-desktop)
    target_link_libraries(${target}
        PRIVATE
            nte_cafe
            webui
    )

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Keep UI files and game data next to the executable for simple local runs.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
                $<TARGET_FILE_DIR:${target}>/ui
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${target}>/data
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/data/gamedata.json
                $<TARGET_FILE_DIR:${target}>/data/gamedata.json
    )
endforeach()

target_compile_definitions(nte-cafe-desktop PRIVATE NTE_CAFE_DESKTOP=1)
