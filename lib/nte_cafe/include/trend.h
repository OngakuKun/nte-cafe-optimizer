#ifndef NTE_CAFE_TREND_H
#define NTE_CAFE_TREND_H

#include "types.h"

bool dish_matches_trend(const Dish *dish, const CafeSettings *settings);
double dish_trend_bonus(const Dish *dish, const CafeSettings *settings);
double dish_base_price(const Dish *dish, int level, const CafeSettings *settings);

float get_trend_bonus(TrendType type);
SkillTag get_trend_tag(Trend trend);
TrendOptions get_trend_options(void);

#endif
