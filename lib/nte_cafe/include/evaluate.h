#ifndef NTE_CAFE_EVALUATE_H
#define NTE_CAFE_EVALUATE_H

#include "types.h"

OptimizationResult evaluate_candidate(const CandidateDish *dishes,
                                      size_t dish_count, MenuStats menu,
                                      const CandidateCharacter *characters,
                                      size_t character_count,
                                      const CafeSettings *settings);

#endif
