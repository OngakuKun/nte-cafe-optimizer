#include "config.h"
#include "data.h"
#include "gamedata.h"
#include "types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *path = "/tmp/nte-cafe-test-config.json";

static int index_of_character(const char *id) {
  for (size_t i = 0; i < CHARACTER_COUNT; ++i)
    if (CHARACTERS[i].id && strcmp(CHARACTERS[i].id, id) == 0)
      return (int)i;
  return -1;
}

static int index_of_dish(const char *id) {
  for (size_t i = 0; i < DISH_COUNT; ++i)
    if (DISHES[i].id && strcmp(DISHES[i].id, id) == 0)
      return (int)i;
  return -1;
}

int main(void) {
  assert(gamedata_init_embedded());

  CafeConfig cfg;
  char warn[512];

  /* Default config: everything at the dataset's minimum. */
  config_init_defaults(&cfg);
  assert(cfg.cafes_owned == 1);
  assert(strcmp(cfg.trend_name, "None") == 0);
  assert(!cfg.decor);
  for (size_t i = 0; i < CHARACTER_COUNT; ++i) {
    assert(!cfg.character_state[i].owned);
    assert(cfg.character_state[i].level == gamedata_character_min_level());
  }
  for (size_t i = 0; i < DISH_COUNT; ++i) {
    assert(!cfg.dish_state[i].owned);
    assert(cfg.dish_state[i].level == gamedata_dish_min_level());
  }

  assert(config_save_file(path, &cfg));
  CafeConfig loaded;
  assert(config_load_file(path, &loaded, warn, sizeof(warn)));
  assert(warn[0] == '\0');
  assert(loaded.cafes_owned == 1);
  assert(strcmp(loaded.trend_name, "None") == 0);
  assert(loaded.dish_state[0].level == gamedata_dish_min_level());

  /* The string API used by WebUI export/import must round-trip too. */
  char json[CONFIG_JSON_MAX];
  assert(config_to_json(&cfg, json, sizeof(json)));
  CafeConfig from_json;
  char json_warn[512];
  assert(config_from_json(json, strlen(json), &from_json, json_warn,
                          sizeof(json_warn)));
  assert(json_warn[0] == '\0');
  assert(from_json.cafes_owned == cfg.cafes_owned);
  assert(strcmp(from_json.trend_name, cfg.trend_name) == 0);
  assert(from_json.decor == cfg.decor);
  assert(from_json.dish_state[0].level == cfg.dish_state[0].level);
  assert(from_json.character_state[0].level == cfg.character_state[0].level);

  /* Round-trip real settings, keyed by stable IDs (trend stays None). */
  int nanally = index_of_character("nanally");
  int latte = index_of_dish("coconut_latte");
  assert(nanally >= 0 && latte >= 0);

  cfg.character_state[nanally].owned = true;
  cfg.character_state[nanally].level = 5;
  cfg.dish_state[latte].owned = true;
  cfg.dish_state[latte].level = 2;
  cfg.cafes_owned = 3;
  cfg.cafe_popularity[0] = 2;
  cfg.trend_name = "Coffee Beans"; /* session-only: must not reach the file */
  cfg.decor = true;
  assert(config_to_json(&cfg, json, sizeof(json)));
  assert(strstr(json, "\"trend\": \"None\"") != NULL);
  assert(strstr(json, "Coffee Beans") == NULL);
  assert(config_save_file(path, &cfg));

  memset(&loaded, 0, sizeof(loaded));
  assert(config_load_file(path, &loaded, warn, sizeof(warn)));
  assert(warn[0] == '\0');
  assert(loaded.character_state[nanally].owned);
  assert(loaded.character_state[nanally].level == 5);
  assert(loaded.dish_state[latte].owned);
  assert(loaded.dish_state[latte].level == 2);
  assert(loaded.cafes_owned == 3);
  assert(loaded.cafe_popularity[0] == 2);
  /* An old config's trend is ignored; it always comes back as No Trend. */
  assert(strcmp(loaded.trend_name, "None") == 0);
  assert(loaded.decor);

  /* Default path always points at a config.json. */
  char *default_path = config_default_path("/usr/bin/nte-cafe-cli");
  assert(default_path);
  assert(strlen(default_path) >= strlen("config.json"));
  assert(strcmp(default_path + strlen(default_path) - strlen("config.json"),
                "config.json") == 0);
  free(default_path);

  /* Unknown IDs and out-of-range levels are reported but don't break load. */
  FILE *f = fopen(path, "w");
  assert(f);
  fputs("{\"format\":\"" CONFIG_FORMAT "\",\"characters\":{"
        "\"does_not_exist\":{\"owned\":true,\"level\":5},"
        "\"nanally\":{\"owned\":true,\"level\":99}},\"dishes\":{}}",
        f);
  fclose(f);
  assert(config_load_file(path, &loaded, warn, sizeof(warn)));
  assert(strstr(warn, "does_not_exist") != NULL);
  assert(strstr(warn, "out of range") != NULL);
  assert(loaded.character_state[nanally].level == gamedata_character_min_level());

  remove(path);
  puts("All config tests passed.");
  return 0;
}
