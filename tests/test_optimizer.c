#include "evaluate.h"
#include "skills.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_additive_traffic_multiplier(void) {
  static const Dish dish = {"Test", "Beverage", 100.0, 100.0, NULL, 0};

  static const Skill skills[] = {
      {1, 0.01, 1, SKILL_TRAFFIC_MULTIPLY, TAG_NONE},
      {1, 0.02, 1, SKILL_TRAFFIC_MULTIPLY, TAG_NONE}};

  CandidateDish dishes[] = {{&dish, 100.0, 0.0}};

  CandidateCharacter team[2] = {{NULL, 1, {skills[0]}, 1},
                                {NULL, 1, {skills[1]}, 1}};

  CafeSettings settings = {.cafes_owned = 1,
                             .trend_tag = TAG_NONE,
                             .trend_name = ""};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, team, 2, &settings);

  /* 2400 + 2400*.01 + 2400*.02 = 2472 */
  assert(fabs(r.traffic - 2472.0) < 1e-9);
}

static void test_price_multiplier_is_additive(void) {
  static const Dish dish = {"Test", "Beverage", 100.0, 100.0, NULL, 0};

  static const Skill skills[] = {{1, 0.01, 1, SKILL_PRICE_MULTIPLY, TAG_NONE},
                                 {1, 0.05, 1, SKILL_PRICE_MULTIPLY, TAG_NONE}};

  CandidateDish dishes[] = {{&dish, 100.0, 0.0}};

  CandidateCharacter team[2] = {{NULL, 1, {skills[0]}, 1},
                                {NULL, 1, {skills[1]}, 1}};

  CafeSettings settings = {.cafes_owned = 1,
                             .trend_tag = TAG_NONE,
                             .trend_name = ""};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, team, 2, &settings);

  assert(fabs(r.price_multiplier - 1.06) < 1e-9);
}

static void test_hotori_is_one_percent_of_base_traffic(void) {
  static const Dish dish = {"Test", "Beverage", 100.0, 100.0, NULL, 0};

  CandidateDish dishes[] = {{&dish, 100.0, 0.0}};

  CafeSettings settings = {.cafes_owned = 1,
                             .trend_tag = TAG_NONE,
                             .trend_name = "",
                             .hotori_cat_decor = true};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, NULL, 0, &settings);

  assert(fabs(r.traffic - 2424.0) < 1e-9);
}

static void test_trend_bonus_after_price_multiplier(void) {
  static const Dish dish = {"Tiramisu", "Desserts", 17.0, 17.0, NULL, 0};
  static const Skill price_flat = {1, 1.56, 1, SKILL_PRICE_FLAT, TAG_NONE};
  static const Skill price_mult = {1, 0.055, 1, SKILL_PRICE_MULTIPLY, TAG_NONE};

  /* base_price includes the +0.75 trend bonus; that part must not be
   * multiplied by the character price multiplier. */
  CandidateDish dishes[] = {{&dish, 17.75, 0.75}};
  CandidateCharacter team[2] = {{NULL, 1, {price_flat}, 1},
                                {NULL, 1, {price_mult}, 1}};
  CafeSettings settings = {.cafes_owned = 1,
                             .trend_tag = TAG_NONE,
                             .trend_name = "Coffee Beans",
                             .trend_bonus = 0.75};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, team, 2, &settings);

  /* ((17 + 1.56) * 1.055 + 0.75) * 2400 / 100 */
  assert(fabs(r.income - 487.9392) < 1e-9);
}

int main(void) {
  test_additive_traffic_multiplier();
  test_price_multiplier_is_additive();
  test_hotori_is_one_percent_of_base_traffic();
  test_trend_bonus_after_price_multiplier();

  puts("All optimizer tests passed.");
  return 0;
}
