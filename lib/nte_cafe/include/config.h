#ifndef NTE_CAFE_CONFIG_H
#define NTE_CAFE_CONFIG_H

#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/* Both the CLI and a future standalone frontend use the same file. */
#define CONFIG_FILE_NAME "config.json"
#define CONFIG_FORMAT "nte-cafe-cli-config"
#define CONFIG_VERSION 1

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

/* Write a complete config file (all known IDs, everything at its minimum). */
bool config_save_file(const char *path, const CafeConfig *cfg);

/* Load and validate a config file. Unknown IDs are skipped with a warning.
 * Returns false on malformed JSON or invalid values; cfg is then unchanged. */
bool config_load_file(const char *path, CafeConfig *cfg, char *warn,
                      size_t warn_len);

/* "<directory of executable>/config.json" (malloc'd; caller frees). */
char *config_default_path(const char *argv0);

#endif /* NTE_CAFE_CONFIG_H */
