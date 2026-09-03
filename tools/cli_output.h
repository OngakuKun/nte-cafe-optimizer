#ifndef NTE_CAFE_RESULT_H
#define NTE_CAFE_RESULT_H

#include "types.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int nCr(int n, int r);
void print_optimization_result(CafeContext *cafeContext,
                               const OptimizationResult *result);

#endif
