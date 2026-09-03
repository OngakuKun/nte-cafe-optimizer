#include "data.h"
#include "trend.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_trend_fruit(void) {
  CafeSettings settings = {.cafes_owned = 5,
                           .trend_tag = TAG_NONE,
                           .trend_name = "Fruit",
                           .trend_bonus = 1.0,
                           .popularity_bonus = 0.0,
                           .hotori_cat_decor = false};

  /*
   * Coconut Latte contains Coconut -> Fruit.
   */
  assert(dish_matches_trend(&DISHES[0], &settings));

  /*
   * Apple Pie contains Apple -> Fruit.
   */
  assert(dish_matches_trend(&DISHES[2], &settings));

  /*
   * Ice Mocha contains Coffee Beans + Cocoa Powder,
   * neither belongs to Fruit.
   */
  assert(!dish_matches_trend(&DISHES[1], &settings));
}

static void test_trend_price(void) {
  CafeSettings settings = {.cafes_owned = 5,
                           .trend_tag = TAG_NONE,
                           .trend_name = "Fruit",
                           .trend_bonus = 1.0,
                           .popularity_bonus = 0.0,
                           .hotori_cat_decor = false};

  double price = dish_base_price(&DISHES[0], 2, &settings);

  assert(fabs(price - 11.2) < 1e-9);
}

int main(void) {
  test_trend_fruit();
  test_trend_price();

  puts("All optimizer tests passed.");
  return 0;
}
