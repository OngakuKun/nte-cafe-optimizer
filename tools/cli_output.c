#include "cli_output.h"
#include "types.h"
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int nCr(int n, int r) {
  if (r > n || r < 0)
    return 0;
  if (r == 0 || r == n)
    return 1;
  r = MIN(r, n - r);
  int result = 1;
  for (int i = 0; i < r; i++)
    result = result * (n - i) / (i + 1);

  return result;
}

void print_optimization_result(CafeContext *cafeContext,
                               const OptimizationResult *result) {
  if (!result)
    return;

  int cafes = cafeContext->settings.cafes_owned;
  int maxD = cafes;
  int maxC = cafes * 2;

  int ownedD = 0;
  int ownedC = 0;

  for (size_t i = 0; i < cafeContext->dish_count; ++i) {
    if (cafeContext->dish_state[i].owned)
      ++ownedD;
  }

  for (size_t i = 0; i < cafeContext->character_count; ++i) {
    if (cafeContext->character_state[i].owned)
      ++ownedC;
  }

  printf("====================================================================="
         "===========\n");

  printf("Dishes: %d owned / %d slots\n", ownedD, maxD);
  printf("Characters: %d owned / %d slots\n", ownedC, maxC);
  printf("Dish Combo: %d\n", nCr(ownedD, MIN(ownedD, maxD)));
  printf("Char Combo: %d\n", nCr(ownedC, MIN(ownedC, maxC)));
  printf("Trend: %s\n", cafeContext->settings.trend_name
                            ? cafeContext->settings.trend_name
                            : "None");

  printf("====================================================================="
         "===========\n");

  printf("Best income: %.2f\n", result->income);
  printf("Traffic: %.2f\n", result->traffic);
  printf("Price buff: %.2f\n", result->price_buff);
  printf("Price multiplier: %.3f\n", result->price_multiplier);

  printf("====================================================================="
         "===========\n");

  printf("Dishes:\n");
  for (size_t i = 0; i < result->dish_count; ++i)
    printf("  - %s (%.2f)\n", result->dishes[i]->name,
           result->dishes[i]->price_l2);

  printf("Characters:\n");
  for (size_t i = 0; i < result->character_count; ++i)
    printf("  - %s\n", result->characters[i]->name);

  /* The buff log contains only skills that really applied to this menu. */
  if (result->log.count > 0) {
    printf("Buffs:\n");
    for (size_t i = 0; i < result->log.count; ++i)
      printf("  - %-*s%+g\n", 20, result->log.entries[i].text,
             result->log.entries[i].value);
  }

  printf("====================================================================="
         "===========\n");
}
