#include "trend.h"
#include "data.h"
#include "types.h"

#include <ctype.h>
#include <string.h>

float get_trend_bonus(TrendType type) {
  switch (type) {
  case TREND_NONE:
    return 0.0f;
  case TREND_TAG:
    return 1.0f;
  case TREND_CATEGORY:
    return 0.75f;
  case TREND_INGREDIENT:
    return 0.75f;
  }
  return -0.0f;
}

SkillTag get_trend_tag(Trend trend) {
  if (trend.name && strcmp(trend.name, "Main Dishes") == 0)
    return TAG_MAIN_DISHES;
  if (trend.name && strcmp(trend.name, "Desserts") == 0)
    return TAG_DESSERTS;
  if (trend.name && strcmp(trend.name, "Beverages") == 0)
    return TAG_BEVERAGES;
  return TAG_NONE;
}

static bool contains_ci(const char *haystack, const char *needle) {
  if (!haystack || !needle || !*needle)
    return false;

  for (const char *p = haystack; *p; ++p) {
    const char *a = p;
    const char *b = needle;

    while (*a && *b &&
           tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
      ++a;
      ++b;
    }

    if (!*b)
      return true;
  }

  return false;
}

static bool equals_ci(const char *a, const char *b) {
  if (!a || !b)
    return false;

  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return false;
    }

    ++a;
    ++b;
  }

  return *a == '\0' && *b == '\0';
}

/*
 * Equivalent to the JS:
 *
 * masterData.ingredients
 *   .filter(r => r.category.toLowerCase() === trendMatch)
 *   .map(r => r.name.toLowerCase())
 */
static bool ingredient_category_matches(const Dish *dish, const char *trend) {
  if (!dish || !trend || !*trend)
    return false;

  for (size_t i = 0; i < dish->ingredient_count; ++i) {
    int ingredient_index = dish->ingredients[i];

    if (ingredient_index < 0 || (size_t)ingredient_index >= INGREDIENT_COUNT) {
      continue;
    }

    const Ingredient *ingredient = &INGREDIENTS[ingredient_index];

    /*
     * JS:
     *
     * r.category.toLowerCase() === trendMatch
     *
     * Therefore this is equality, NOT substring matching.
     */
    if (!ingredient->category)
      continue;

    if (!equals_ci(ingredient->category, trend))
      continue;

    /*
     * The category itself matched.
     *
     * The ingredient belonging to that category is present
     * in this dish by definition.
     */
    return true;
  }

  return false;
}

static bool ingredient_name_matches(const Dish *dish, const char *trend) {
  if (!dish || !trend || !*trend)
    return false;

  for (size_t i = 0; i < dish->ingredient_count; ++i) {
    int ingredient_index = dish->ingredients[i];

    if (ingredient_index < 0 || (size_t)ingredient_index >= INGREDIENT_COUNT) {
      continue;
    }

    const Ingredient *ingredient = &INGREDIENTS[ingredient_index];

    if (!ingredient->name)
      continue;

    /*
     * JS:
     *
     * ingredients.includes(trendMatch)
     *
     * So this is substring matching.
     */
    if (contains_ci(ingredient->name, trend))
      return true;
  }

  return false;
}

bool dish_matches_trend(const Dish *dish, const CafeSettings *settings) {
  if (!dish || !settings || !settings->trend_name || !*settings->trend_name) {
    return false;
  }

  const char *trend = settings->trend_name;

  /*
   * app.js:
   *
   * d.type.toLowerCase().includes(trendMatch)
   */
  if (dish->type && contains_ci(dish->type, trend)) {
    return true;
  }

  /*
   * app.js:
   *
   * ingredients.includes(trendMatch)
   */
  if (ingredient_name_matches(dish, trend)) {
    return true;
  }

  /*
   * app.js:
   *
   * trendingSubItems =
   *   ingredients whose category === trendMatch
   *
   * then:
   *
   * ingredients.includes(sub)
   */
  if (ingredient_category_matches(dish, trend)) {
    return true;
  }

  return false;
}

double dish_trend_bonus(const Dish *dish, const CafeSettings *settings) {
  if (!settings || !dish_matches_trend(dish, settings))
    return 0.0;
  return settings->trend_bonus;
}

double dish_base_price(const Dish *dish, int level,
                       const CafeSettings *settings) {
  if (!dish)
    return 0.0;

  double price = (level == 2) ? dish->price_l2 : dish->price_l1;

  /* Kept in the displayed base price for ranking; evaluation removes it
   * again before applying character price multipliers (see evaluate.c). */
  price += dish_trend_bonus(dish, settings);

  return price;
}

static bool trend_option_exists(const TrendOptions *options, const char *name) {
  for (size_t i = 0; i < options->count; ++i) {
    if (strcmp(options->trends[i].name, name) == 0)
      return true;
  }

  return false;
}

TrendOptions get_trend_options(void) {
  TrendOptions result = {0};

  /*
   * Special option
   */
  result.trends[result.count].name = "None";
  result.trends[result.count++].type = TREND_NONE;

  /*
   * The three dish types / trend tags
   */
  result.trends[result.count].name = "Main Dishes";
  result.trends[result.count++].type = TREND_TAG;
  result.trends[result.count].name = "Desserts";
  result.trends[result.count++].type = TREND_TAG;
  result.trends[result.count].name = "Beverages";
  result.trends[result.count++].type = TREND_TAG;

  /*
   * Ingredient categories
   */
  for (size_t i = 0; i < INGREDIENT_COUNT; ++i) {
    const char *category = INGREDIENTS[i].category;

    if (!category || !*category)
      continue;

    if (trend_option_exists(&result, category))
      continue;

    result.trends[result.count].name = category;
    result.trends[result.count++].type = TREND_CATEGORY;
  }

  /*
   * Ingredients
   */
  for (size_t i = 0; i < INGREDIENT_COUNT; ++i) {
    const char *name = INGREDIENTS[i].name;

    if (!name || !*name)
      continue;

    if (trend_option_exists(&result, name))
      continue;

    result.trends[result.count].name = name;
    result.trends[result.count++].type = TREND_INGREDIENT;
  }

  return result;
}
