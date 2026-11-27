add_executable(test_optimizer
    test_optimizer.c
)

target_link_libraries(test_optimizer
    PRIVATE
        nte_cafe
)

add_test(NAME optimizer COMMAND test_optimizer)


add_executable(test_trend
    test_trend.c
)

target_link_libraries(test_trend
    PRIVATE
        nte_cafe
)

add_test(NAME trend COMMAND test_trend)


add_executable(test_full_dataset
    test_full_dataset.c
)

target_link_libraries(test_full_dataset
    PRIVATE
        nte_cafe
)

add_test(NAME full_dataset COMMAND test_full_dataset)


add_executable(test_optimizer_current
    test_optimizer_current.c
)

target_link_libraries(test_optimizer_current
    PRIVATE
        nte_cafe
)

add_test(NAME optimizer_current COMMAND test_optimizer_current)


add_executable(test_popularity
    test_popularity.c
)

target_link_libraries(test_popularity
    PRIVATE
        nte_cafe
)

add_test(NAME popularity COMMAND test_popularity)


add_executable(benchmark_optimizer
    benchmark_optimizer.c
)

target_link_libraries(benchmark_optimizer
    PRIVATE
        nte_cafe
)

add_test(NAME benchmark COMMAND benchmark_optimizer)
