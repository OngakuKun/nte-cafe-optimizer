#include "data.h"
#include "optimizer.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static void assert_double_equal(double actual, double expected,
                                double epsilon) {
  assert(fabs(actual - expected) <= epsilon);
}

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
                     {5, TAG_NONE, "", 1.0, 0.0, true}};

  OptimizationResult result;
  if (!optimize_cafe(&ctx, &result)) {
    fprintf(stderr, "No valid optimization result.\n");
    return 1;
  }

  assert_double_equal(result.traffic, 2754.135, 1e-9);
  assert_double_equal(result.price_buff, 1.56, 1e-9);
  assert_double_equal(result.price_multiplier, 1.055, 1e-9);
  assert_double_equal(result.income, 2623.768019775, 1e-9);

  puts("Golden Test passed");

  return 0;
}
