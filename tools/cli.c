#include "cli_output.h"
#include "data.h"
#include "optimizer.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  DishState dish_state[DISH_COUNT];
  CharacterState character_state[CHARACTER_COUNT];

  for (size_t i = 0; i < DISH_COUNT; ++i) {
    dish_state[i].owned = true;
    dish_state[i].level = 2;
  }

  for (size_t i = 0; i < CHARACTER_COUNT; ++i) {
    character_state[i].owned = true;
    character_state[i].level = 5;
  }

  CafeContext ctx = {DISHES,
                     DISH_COUNT,
                     CHARACTERS,
                     CHARACTER_COUNT,
                     dish_state,
                     character_state,
                     {5, TAG_MAIN_DISHES, "Main Dish", 1.00, 0.0, true}};

  OptimizationResult result;
  if (!optimize_cafe(&ctx, &result)) {
    fprintf(stderr, "No valid optimization result.\n");
    return 1;
  }

  print_optimization_result(&ctx, &result);

  return 0;
}
