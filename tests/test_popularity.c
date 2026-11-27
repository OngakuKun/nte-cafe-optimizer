#include "evaluate.h"
#include "popularity.h"
#include "skills.h"
#include "types.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void assert_close(double actual, double expected, double epsilon) {
  if (fabs(actual - expected) > epsilon)
    fprintf(stderr, "expected %.12f got %.12f (delta %.12f)\n", expected,
            actual, fabs(actual - expected));
  assert(fabs(actual - expected) <= epsilon);
}

int main(void) {
  assert(popularity_max_level(0) == 3);
  assert(popularity_max_level(1) == 5);
  assert(popularity_max_level(2) == 5);
  assert(popularity_max_level(3) == 5);
  assert(popularity_max_level(4) == 5);
  assert(popularity_max_level(5) == 0);

  assert(popularity_threshold(0, 3) == 150);
  assert(popularity_threshold(1, 5) == 500);
  assert(popularity_threshold(3, 5) == 750);
  assert_close(popularity_level_bonus(0, 3), 0.010, 1e-12);
  assert_close(popularity_level_bonus(1, 5), 0.015, 1e-12);
  assert_close(popularity_level_bonus(3, 5), 0.025, 1e-12);

  CafeSettings settings;
  memset(&settings, 0, sizeof(settings));
  settings.cafes_owned = 5;

  /* Only the reached level counts - lower levels are replaced, not added. */
  settings.cafe_popularity[0] = 3;
  assert_close(popularity_cafe_bonus(&settings, 0), 0.010, 1e-12);
  settings.cafe_popularity[0] = 2;
  assert_close(popularity_cafe_bonus(&settings, 0), 0.007, 1e-12);

  /* zuzug == 0.090: every cafe on its highest level. */
  for (int c = 0; c < MAX_CAFES; ++c)
    settings.cafe_popularity[c] = popularity_max_level(c);
  assert_close(popularity_total_bonus(&settings), 0.090, 1e-12);

  /* Without the third tier in the first cafe the in-game display shows 0.087. */
  settings.cafe_popularity[0] = 2;
  assert_close(popularity_total_bonus(&settings), 0.087, 1e-12);

  /* Cafes that are not owned do not contribute. */
  settings.cafes_owned = 2;
  assert_close(popularity_total_bonus(&settings), 0.022, 1e-12);

  /* The bonus is applied to the total income, not to a single dish. */
  static const Dish dish = {"Test", "Beverage", 100.0, 100.0, NULL, 0};
  CandidateDish dishes[] = {{&dish, 100.0, 0.0}};
  CandidateCharacter team[1];
  memset(team, 0, sizeof(team));
  team[0].level = 1;
  CafeSettings income_settings;
  memset(&income_settings, 0, sizeof(income_settings));
  income_settings.cafes_owned = 1;
  income_settings.cafe_popularity[0] = 1; /* +0.005 */

  MenuStats menu;
  calculate_menu_stats(dishes, 1, &menu);
  OptimizationResult r =
      evaluate_candidate(dishes, 1, menu, team, 1, &income_settings);
  assert_close(r.popularity_bonus, 0.005, 1e-12);
  assert_close(r.income, 2400.0 * 1.005, 1e-9);

  printf("Popularity tests passed.\n");
  return 0;
}
