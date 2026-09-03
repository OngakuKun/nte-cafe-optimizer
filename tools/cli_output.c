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
  switch (cafeContext->settings.trend_tag) {
  case TAG_NONE:
    printf("Trend: %s\n", cafeContext->settings.trend_name);
    break;
  case TAG_ANY:
    printf("Trend: %s\n", cafeContext->settings.trend_name);
    break;
  case TAG_MAIN_DISHES:
    printf("Trend: Main Dishes\n");
    break;
  case TAG_BEVERAGES:
    printf("Trend: Beverages\n");
    break;
  case TAG_DESSERTS:
    printf("Trend: Desserts\n");
    break;
  }

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
  for (size_t i = 0; i < result->character_count; ++i) {
    printf("  - %-*s", 20, result->characters[i]->name);

    double pFlatSum = 0.0;
    double pMultiSum = 0.0;
    double tFlatSum = 0.0;
    double tMultiSum = 0.0;

    for (int j = 0; j < (int)result->characters[i]->skill_count; ++j) {
      Skill skill = result->characters[i]->skills[j];

      switch (skill.type) {
      case SKILL_PRICE_FLAT:
        pFlatSum += skill.value;
        break;

      case SKILL_PRICE_MULTIPLY:
        pMultiSum += skill.value;
        break;

      case SKILL_TRAFFIC_FLAT:
        tFlatSum += skill.value;
        break;

      case SKILL_TRAFFIC_MULTIPLY:
        tMultiSum += skill.value;
        break;
      }
    }

    bool first = true;

    if (pFlatSum != 0.0) {
      printf("%s%+g price", first ? "" : " & ", pFlatSum);
      first = false;
    }

    if (pMultiSum != 0.0) {
      printf("%s+%.2f%% price", first ? "" : " & ", pMultiSum * 100);
      first = false;
    }

    if (tFlatSum != 0.0) {
      printf("%s%+g traffic", first ? "" : " & ", tFlatSum);
      first = false;
    }

    if (tMultiSum != 0.0) {
      printf("%s+%.2f%% traffic", first ? "" : " & ", tMultiSum * 100);
      first = false;
    }

    printf("\n");
  }
  if (cafeContext->settings.hotori_cat_decor) {
    printf("Buffs:\n");
    printf("  - %-*s+1%% traffic\n", 20, "Hotori Cat Decor");
  }

  printf("====================================================================="
         "===========\n");
}
