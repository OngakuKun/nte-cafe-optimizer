#ifndef NTE_CAFE_EVALUATE_H
#define NTE_CAFE_EVALUATE_H

#include "types.h"

OptimizationResult evaluate_candidate(const CandidateDish *dishes,
                                      size_t dish_count, MenuStats menu,
                                      const CandidateCharacter *characters,
                                      size_t character_count,
                                      const CafeSettings *settings);

/* Fast hot-path evaluation: returns income only, no logging/result struct. */
double evaluate_income_fast(const MenuStats *menu,
                            const CandidateCharacter *characters,
                            size_t character_count,
                            const CafeSettings *settings);

#endif
