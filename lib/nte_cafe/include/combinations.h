#ifndef NTE_CAFE_COMBINATIONS_H
#define NTE_CAFE_COMBINATIONS_H

#include <stdbool.h>
#include <stddef.h>

typedef bool (*CombinationCallback)(const size_t *indices, size_t count, void *userdata);

bool enumerate_combinations(
    size_t n,
    size_t k,
    CombinationCallback callback,
    void *userdata
);

#endif
