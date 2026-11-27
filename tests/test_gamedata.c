#include "data.h"
#include "gamedata.h"
#include "types.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  assert(gamedata_init_embedded());

  assert(INGREDIENT_COUNT == 25);
  assert(DISH_COUNT == 18);
  assert(CHARACTER_COUNT == 16);
  assert(strcmp(gamedata_game_version(), "1.5") == 0);
  assert(strlen(gamedata_content_hash()) == 64);

  assert(strcmp(DISHES[0].name, "Coconut Latte") == 0);
  assert(strcmp(DISHES[0].id, "coconut_latte") == 0);
  assert(DISHES[0].ingredient_count == 2);
  assert(strcmp(INGREDIENTS[DISHES[0].ingredients[0]].name,
                "Coffee Beans") == 0);
  assert(strcmp(CHARACTERS[0].name, "Nanally") == 0);
  assert(CHARACTERS[0].skill_count == 2);

  assert(strcmp(gamedata_i18n("de", "trend.None"), "Ohne Trend") == 0);
  assert(strcmp(gamedata_i18n("en", "trend.None"), "No Trend") == 0);
  assert(strcmp(gamedata_i18n("de", "dish.Iced Mocha"), "Eis-Mokka") == 0);
  assert(gamedata_i18n_count() > 100);

  puts("All gamedata tests passed.");
  return 0;
}
