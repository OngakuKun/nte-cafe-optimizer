#ifndef NTE_CAFE_POPULARITY_H
#define NTE_CAFE_POPULARITY_H

#include "types.h"

/* Highest selectable buff level of a cafe (0 means no bonus at all). */
int popularity_max_level(int cafe_index);

/* Popularity required to reach a level (1-based), 0 when out of range. */
int popularity_threshold(int cafe_index, int level);

/* Income bonus of a level (1-based), 0.0 when out of range. */
double popularity_level_bonus(int cafe_index, int level);

/* Bonus of the level selected for one cafe. */
double popularity_cafe_bonus(const CafeSettings *settings, int cafe_index);

/* Sum over all owned cafes: the in-game "Gesamtmultiplikator fuer alle Laeden". */
double popularity_total_bonus(const CafeSettings *settings);

#endif
