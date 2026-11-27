#ifndef NTE_CAFE_CONFIG_H
#define NTE_CAFE_CONFIG_H

#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/* One format for the CLI and the WebApp. Keyed by stable game-data IDs so
 * game updates never shift a saved state onto a different character/dish. */
#define CONFIG_FILE_NAME "config.json"
#define CONFIG_FORMAT "nte-cafe-config"
#define CONFIG_VERSION 1
#define CONFIG_JSON_MAX 16384

typedef struct {
    DishState dish_state[MAX_DISH_POOL];
    CharacterState character_state[MAX_CHARACTER_POOL];
    int cafes_owned;
    int cafe_popularity[MAX_CAFES];
    const char *trend_name;
    bool decor;
} CafeConfig;

/* Minimum/default values, taken from the loaded dataset's level ranges. */
void config_init_defaults(CafeConfig *cfg);

/* Serialise into buf (CONFIG_JSON_MAX bytes). */
bool config_to_json(const CafeConfig *cfg, char *buf, size_t len);

/* Parse and validate JSON. Unknown IDs are skipped with a warning. Returns
 * false on malformed JSON or invalid values; cfg is then unchanged. */
bool config_from_json(const char *json, size_t len, CafeConfig *cfg, char *warn,
                      size_t warn_len);

/* Convenience wrappers around the string API. */
bool config_save_file(const char *path, const CafeConfig *cfg);
bool config_load_file(const char *path, CafeConfig *cfg, char *warn,
                      size_t warn_len);

/* "<directory of executable>/config.json" (malloc'd; caller frees). */
char *config_default_path(const char *argv0);

#endif /* NTE_CAFE_CONFIG_H */
