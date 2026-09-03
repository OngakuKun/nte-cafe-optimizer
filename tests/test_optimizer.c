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

  CandidateDish dishes[] = {{&dish, 100.0}};

  CandidateCharacter team[2] = {{NULL, 1, {skills[0]}, 1},
                                {NULL, 1, {skills[1]}, 1}};

  CafeSettings settings = {1, TAG_NONE, "", 0.0, 0.0, false};

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

  CandidateDish dishes[] = {{&dish, 100.0}};

  CandidateCharacter team[2] = {{NULL, 1, {skills[0]}, 1},
                                {NULL, 1, {skills[1]}, 1}};

  CafeSettings settings = {1, TAG_NONE, "", 0.0, 0.0, false};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, team, 2, &settings);

  assert(fabs(r.price_multiplier - 1.06) < 1e-9);
}

static void test_hotori_is_one_percent_of_base_traffic(void) {
  static const Dish dish = {"Test", "Beverage", 100.0, 100.0, NULL, 0};

  CandidateDish dishes[] = {{&dish, 100.0}};

  CafeSettings settings = {1, TAG_NONE, "", 0.0, 0.0, true};

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, NULL, 0, &settings);

  assert(fabs(r.traffic - 2424.0) < 1e-9);
}

int main(void) {
  test_additive_traffic_multiplier();
  test_price_multiplier_is_additive();
  test_hotori_is_one_percent_of_base_traffic();

  puts("All optimizer tests passed.");
  return 0;
}
