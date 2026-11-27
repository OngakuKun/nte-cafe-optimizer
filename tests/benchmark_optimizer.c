#include "data.h"
#include "gamedata.h"
#include "optimizer.h"
#include "types.h"

#include <stdio.h>
#include <time.h>

int main(void) {
  if (!gamedata_init_embedded())
    return 1;

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
                     {5, TAG_NONE, "Fruit", 0.75, 0.0, true}};

  OptimizationResult result;

  clock_t start = clock();

  bool success = optimize_cafe(&ctx, &result);

  clock_t end = clock();

  if (!success) {
    fprintf(stderr, "Optimization failed.\n");
    return 1;
  }

  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  printf("Income: %.6f\n", result.income);
  printf("Traffic: %.6f\n", result.traffic);
  printf("Time: %.6f seconds\n", elapsed);

  return 0;
}
