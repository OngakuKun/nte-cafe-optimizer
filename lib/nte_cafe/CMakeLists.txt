add_library(nte_cafe
    src/data.c
    src/combinations.c
    src/trend.c
    src/popularity.c
    src/skills.c
    src/evaluate.c
    src/optimizer.c
)

target_include_directories(nte_cafe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(nte_cafe PRIVATE /W4)
else()
    target_compile_options(nte_cafe PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
