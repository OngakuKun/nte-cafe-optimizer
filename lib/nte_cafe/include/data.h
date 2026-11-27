#ifndef NTE_CAFE_DATA_H
#define NTE_CAFE_DATA_H

#include "types.h"

/*
 * Game data is populated at startup by gamedata_init() from JSON.
 * The arrays are mutable so an updated dataset can replace them at runtime.
 */
extern Ingredient INGREDIENTS[];
extern size_t INGREDIENT_COUNT;

extern Dish DISHES[];
extern size_t DISH_COUNT;

extern Character CHARACTERS[];
extern size_t CHARACTER_COUNT;

#endif /* NTE_CAFE_DATA_H */
