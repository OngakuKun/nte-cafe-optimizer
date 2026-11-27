#include "data.h"
#include "types.h"

/*
 * Storage for the dataset loaded at startup. The previous hardcoded arrays
 * were moved to data/gamedata.json; see gamedata.c for the loader.
 */
Ingredient INGREDIENTS[MAX_INGREDIENTS];
size_t INGREDIENT_COUNT = 0;

Dish DISHES[MAX_DISH_POOL];
size_t DISH_COUNT = 0;

Character CHARACTERS[MAX_CHARACTER_POOL];
size_t CHARACTER_COUNT = 0;
