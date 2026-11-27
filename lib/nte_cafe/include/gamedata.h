#ifndef NTE_CAFE_GAMEDATA_H
#define NTE_CAFE_GAMEDATA_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Game data is loaded from JSON at startup. The compiled-in copy in
 * gamedata_embedded.h is the offline fallback; an external file or a
 * downloaded update can replace it via gamedata_init()/gamedata_init_file().
 */
bool gamedata_init(const char *json, size_t len);
bool gamedata_init_embedded(void);
bool gamedata_init_file(const char *path);

const char *gamedata_game_version(void);
const char *gamedata_content_hash(void);

/* Returns a translated display string, or the key itself when missing. */
const char *gamedata_i18n(const char *lang, const char *key);

/* Iteration over all loaded translations (used to expose them to the UI). */
size_t gamedata_i18n_count(void);
bool gamedata_i18n_at(size_t index, const char **lang, const char **key,
                      const char **value);

#endif /* NTE_CAFE_GAMEDATA_H */
