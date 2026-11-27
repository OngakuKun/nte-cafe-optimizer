#include "data.h"
#include "optimizer.h"
#include "trend.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void assert_double_equal(double actual, double expected, double epsilon) {
  if (fabs(actual - expected) > epsilon) {
    fprintf(stderr, "expected %.12f got %.12f (delta %.12f)\n", expected, actual, fabs(actual - expected));
  }
  assert(fabs(actual - expected) <= epsilon);
}

static bool str_eq(const char *a, const char *b) {
  return a && b && strcmp(a, b) == 0;
}

int main(void) {
  /* Baseline captured from WebUI /runOptimization on 2026-11-23.
   * Setup: all dishes owned level 2, all current characters owned level 5,
   * cafes 5, trend Desserts, hotori decor on. Only the character roster known
   * at capture time is enabled so future character additions don't change the
   * expected optimal result. */
  static const char *CURRENT_CHARS[] = {
      "Nanally", "Sakiri", "Skia", "Baicang", "Mint", "Haniel", "Adler",
      "Dafodil", "Edgar", "Aurelia", "Chiz", "Lacrimosa", "Shinku", "Iroi",
      "Linko", "Akane Rin",
  };
  static const char *EXPECTED_DISHES[] = {
      "Orange Hot Coffee",
      "Coconut Macchiato",
      "Matcha Latte with Milk Cap",
      "Tiramisu",
      "Caramel & Cocoa Mille-Feuille",
  };
  static const int EXPECTED_DISH_LEVELS[] = {2, 2, 2, 2, 2};
  static const double EXPECTED_DISH_FPH[] = {487.68, 517.13, 531.85, 574.49, 603.94};
  static const bool EXPECTED_DISH_TREND[] = {false, false, false, true, true};
  static const char *EXPECTED_CHARS[] = {
      "Sakiri", "Skia", "Haniel", "Edgar", "Aurelia",
      "Lacrimosa", "Shinku", "Iroi", "Linko", "Akane Rin",
  };

  DishState dish_state[DISH_COUNT];
  CharacterState character_state[CHARACTER_COUNT];

  for (size_t i = 0; i < DISH_COUNT; ++i) {
    dish_state[i].owned = true;
    dish_state[i].level = 2;
  }
  for (size_t i = 0; i < CHARACTER_COUNT; ++i) {
    bool known = false;
    for (size_t k = 0; k < sizeof(CURRENT_CHARS) / sizeof(CURRENT_CHARS[0]); ++k) {
      if (str_eq(CHARACTERS[i].name, CURRENT_CHARS[k])) { known = true; break; }
    }
    character_state[i].owned = known;
    character_state[i].level = 5;
  }

  Trend trend = {TREND_TAG, "Desserts"};
  CafeSettings settings = {
      .cafes_owned = 5,
      .trend_tag = get_trend_tag(trend),
      .trend_name = trend.name,
      .trend_bonus = get_trend_bonus(trend.type),
      .hotori_cat_decor = true,
      .popularity_bonus = 0.0,
  };

  CafeContext ctx = {
      .dishes = DISHES,
      .dish_count = DISH_COUNT,
      .characters = CHARACTERS,
      .character_count = CHARACTER_COUNT,
      .dish_state = dish_state,
      .character_state = character_state,
      .settings = settings,
  };

  OptimizationResult result;
  assert(optimize_cafe(&ctx, &result));

  assert_double_equal(result.income, 2715.09, 0.01);
  assert_double_equal(result.traffic, 2791.39, 0.01);
  assert_double_equal(result.price_buff, 1.56, 1e-9);
  assert_double_equal(result.price_multiplier, 1.055, 1e-9);

  assert(result.dish_count == 5);
  for (size_t i = 0; i < 5; ++i) {
    assert(result.dishes[i] != NULL);
    assert(str_eq(result.dishes[i]->name, EXPECTED_DISHES[i]));
    int level = 1;
    for (size_t d = 0; d < DISH_COUNT; ++d) {
      if (&DISHES[d] == result.dishes[i]) { level = dish_state[d].level; break; }
    }
    assert(level == EXPECTED_DISH_LEVELS[i]);
    double base_price = dish_base_price(result.dishes[i], level, &settings);
    double trend_bonus = EXPECTED_DISH_TREND[i] ? settings.trend_bonus : 0.0;
    double fph = ((base_price - trend_bonus + result.price_buff) * result.price_multiplier +
                  trend_bonus) * (result.traffic / 100.0);
    assert_double_equal(fph, EXPECTED_DISH_FPH[i], 0.01);
    assert(dish_matches_trend(result.dishes[i], &settings) == EXPECTED_DISH_TREND[i]);
  }

  assert(result.character_count == 10);
  for (size_t i = 0; i < 10; ++i) {
    assert(result.characters[i] != NULL);
    assert(str_eq(result.characters[i]->name, EXPECTED_CHARS[i]));
  }

  puts("Optimizer current-roster fixture test passed.");
  return 0;
}
