#include "config.h"
#include "data.h"
#include "gamedata.h"
#define JSMN_STATIC
#include "jsmn.h"
#include "popularity.h"
#include "trend.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
#include <unistd.h>
#endif

#define CONFIG_MAX_FILE (256 * 1024)
#define CONFIG_MAX_TOKENS 4096

static char *dup_str(const char *s) {
  size_t n = strlen(s) + 1;
  char *out = (char *)malloc(n);
  if (out)
    memcpy(out, s, n);
  return out;
}

static void warnf(char *buf, size_t len, const char *fmt, ...) {
  if (!buf || len == 0)
    return;
  size_t used = strlen(buf);
  if (used + 1 >= len)
    return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf + used, len - used, fmt, ap);
  va_end(ap);
}

static bool tok_is(const char *js, const jsmntok_t *t, const char *want) {
  if (!t || (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE))
    return false;
  size_t n = (size_t)(t->end - t->start);
  return n == strlen(want) && memcmp(js + t->start, want, n) == 0;
}

static const char *tok_str(const char *js, const jsmntok_t *t) {
  if (!t || t->type != JSMN_STRING || t->start < 0 || t->end <= t->start)
    return "";
  return js + t->start;
}

static int tok_int(const char *js, const jsmntok_t *t) {
  char buf[64];
  if (!t || t->start < 0 || t->end <= t->start)
    return 0;
  size_t n = (size_t)(t->end - t->start);
  if (n >= sizeof(buf))
    n = sizeof(buf) - 1;
  memcpy(buf, js + t->start, n);
  buf[n] = '\0';
  return (int)strtol(buf, NULL, 10);
}

static bool tok_bool(const char *js, const jsmntok_t *t) {
  return t && tok_is(js, t, "true");
}

static int skip_token(const jsmntok_t *tokens, int count, int idx) {
  if (idx < 0 || idx >= count)
    return idx;
  int children = tokens[idx].size;
  int i = idx + 1;
  if (tokens[idx].type == JSMN_OBJECT) {
    for (int k = 0; k < children && i < count; ++k) {
      i = skip_token(tokens, count, i);
      i = skip_token(tokens, count, i);
    }
  } else if (tokens[idx].type == JSMN_ARRAY) {
    for (int k = 0; k < children && i < count; ++k)
      i = skip_token(tokens, count, i);
  }
  return i;
}

static int obj_get(const char *js, const jsmntok_t *tokens, int count,
                   int obj, const char *key) {
  if (obj < 0 || obj >= count || tokens[obj].type != JSMN_OBJECT)
    return -1;
  int i = obj + 1;
  for (int k = 0; k < tokens[obj].size && i + 1 < count; ++k) {
    if (tok_is(js, &tokens[i], key))
      return i + 1;
    i = skip_token(tokens, count, i);
    i = skip_token(tokens, count, i);
  }
  return -1;
}

static int find_character(const char *id) {
  for (size_t i = 0; i < CHARACTER_COUNT; ++i)
    if (CHARACTERS[i].id && strcmp(CHARACTERS[i].id, id) == 0)
      return (int)i;
  return -1;
}

static int find_dish(const char *id) {
  for (size_t i = 0; i < DISH_COUNT; ++i)
    if (DISHES[i].id && strcmp(DISHES[i].id, id) == 0)
      return (int)i;
  return -1;
}

static const char *find_trend_name(const char *name) {
  TrendOptions options = get_trend_options();
  for (size_t i = 0; i < options.count; ++i)
    if (strcmp(options.trends[i].name, name) == 0)
      return options.trends[i].name;
  return NULL;
}

void config_init_defaults(CafeConfig *cfg) {
  if (!cfg)
    return;
  memset(cfg, 0, sizeof(*cfg));
  for (size_t i = 0; i < DISH_COUNT; ++i)
    cfg->dish_state[i].level = gamedata_dish_min_level();
  for (size_t i = 0; i < CHARACTER_COUNT; ++i)
    cfg->character_state[i].level = gamedata_character_min_level();
  cfg->cafes_owned = 1;
  cfg->trend_name = "None";
  cfg->decor = false;
}

char *config_default_path(const char *argv0) {
  char path[4096] = "";
  bool have = false;

#if defined(__linux__)
  ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (n > 0) {
    path[n] = '\0';
    have = true;
  }
#endif
  if (!have && argv0 && *argv0) {
    snprintf(path, sizeof(path), "%s", argv0);
    have = true;
  }
  if (!have)
    snprintf(path, sizeof(path), ".");

  char *sep = NULL;
  for (char *p = path; *p; ++p)
    if (*p == '/' || *p == '\\')
      sep = p;

  size_t len = 0;
  if (sep == path) {
    path[1] = '\0';
    len = 1;
  } else if (sep) {
    *sep = '\0';
    len = strlen(path);
  } else {
    snprintf(path, sizeof(path), ".");
    len = 1;
  }

  char full[4200];
  snprintf(full, sizeof(full), "%s%s%s", path,
           (len > 0 && path[len - 1] == '/') ? "" : "/", CONFIG_FILE_NAME);
  return dup_str(full);
}

static void write_string(char **cursor, char *end, const char *s) {
  char *p = *cursor;
  if (p >= end)
    return;
  *p++ = '"';
  for (const unsigned char *c = (const unsigned char *)s; *c && p + 2 < end;
       ++c) {
    if (*c == '"' || *c == '\\') {
      *p++ = '\\';
      *p++ = *c;
    } else if (*c == '\n') {
      *p++ = '\\';
      *p++ = 'n';
    } else {
      *p++ = (char)*c;
    }
  }
  if (p < end)
    *p++ = '"';
  *cursor = p;
}

bool config_to_json(const CafeConfig *cfg, char *buf, size_t len) {
  if (!cfg || !buf || len < 256)
    return false;

  char *p = buf;
  char *end = buf + len - 1;
  int written = snprintf(p, (size_t)(end - p),
                         "{\n  \"format\": \"" CONFIG_FORMAT "\",\n"
                         "  \"version\": %d,\n  \"cafes\": %d,\n  \"trend\": ",
                         CONFIG_VERSION, cfg->cafes_owned);
  if (written < 0 || (size_t)written >= (size_t)(end - p))
    return false;
  p += written;
  write_string(&p, end, cfg->trend_name ? cfg->trend_name : "None");

  written = snprintf(p, (size_t)(end - p),
                     ",\n  \"decor\": %s,\n  \"popularity\": [",
                     cfg->decor ? "true" : "false");
  if (written < 0 || (size_t)written >= (size_t)(end - p))
    return false;
  p += written;
  for (int i = 0; i < MAX_CAFES; ++i) {
    written = snprintf(p, (size_t)(end - p), "%s%d", i ? ", " : "",
                       cfg->cafe_popularity[i]);
    if (written < 0 || (size_t)written >= (size_t)(end - p))
      return false;
    p += written;
  }

  written = snprintf(p, (size_t)(end - p), "],\n  \"characters\": {\n");
  if (written < 0 || (size_t)written >= (size_t)(end - p))
    return false;
  p += written;
  for (size_t i = 0; i < CHARACTER_COUNT; ++i) {
    written = snprintf(p, (size_t)(end - p), "    ");
    if (written < 0 || (size_t)written >= (size_t)(end - p))
      return false;
    p += written;
    write_string(&p, end, CHARACTERS[i].id ? CHARACTERS[i].id : "");
    written = snprintf(p, (size_t)(end - p),
                       ": { \"owned\": %s, \"level\": %d }%s\n",
                       cfg->character_state[i].owned ? "true" : "false",
                       cfg->character_state[i].level,
                       i + 1 < CHARACTER_COUNT ? "," : "");
    if (written < 0 || (size_t)written >= (size_t)(end - p))
      return false;
    p += written;
  }

  written = snprintf(p, (size_t)(end - p), "  },\n  \"dishes\": {\n");
  if (written < 0 || (size_t)written >= (size_t)(end - p))
    return false;
  p += written;
  for (size_t i = 0; i < DISH_COUNT; ++i) {
    written = snprintf(p, (size_t)(end - p), "    ");
    if (written < 0 || (size_t)written >= (size_t)(end - p))
      return false;
    p += written;
    write_string(&p, end, DISHES[i].id ? DISHES[i].id : "");
    written = snprintf(p, (size_t)(end - p),
                       ": { \"owned\": %s, \"level\": %d }%s\n",
                       cfg->dish_state[i].owned ? "true" : "false",
                       cfg->dish_state[i].level, i + 1 < DISH_COUNT ? "," : "");
    if (written < 0 || (size_t)written >= (size_t)(end - p))
      return false;
    p += written;
  }

  written = snprintf(p, (size_t)(end - p), "  }\n}\n");
  if (written < 0 || (size_t)written >= (size_t)(end - p))
    return false;
  p += written;
  return true;
}

bool config_save_file(const char *path, const CafeConfig *cfg) {
  char json[CONFIG_JSON_MAX];
  if (!path || !cfg || !config_to_json(cfg, json, sizeof(json)))
    return false;

  FILE *f = fopen(path, "w");
  if (!f)
    return false;
  size_t expected = strlen(json);
  size_t n = fwrite(json, 1, expected, f);
  int flush_rc = fflush(f);
  int close_rc = fclose(f);
  return n == expected && flush_rc == 0 && close_rc == 0;
}

/* Parse one "id": { "owned": bool, "level": int } pair into a state array. */
static void parse_entries(const char *js, const jsmntok_t *tokens, int count,
                          int object, bool characters, CafeConfig *cfg,
                          char *warn, size_t warn_len) {
  if (object < 0 || object >= count || tokens[object].type != JSMN_OBJECT)
    return;

  int i = object + 1;
  for (int k = 0; k < tokens[object].size && i + 1 < count; ++k) {
    const char *id = tok_str(js, &tokens[i]);
    int value = i + 1;
    int index = characters ? find_character(id) : find_dish(id);

    if (index < 0) {
      warnf(warn, warn_len, "unknown %s '%s'; ",
            characters ? "character" : "dish", id);
    } else {
      int entry = obj_get(js, tokens, count, value, "owned");
      int level = obj_get(js, tokens, count, value, "level");
      int lo = characters ? gamedata_character_min_level()
                          : gamedata_dish_min_level();
      int hi = characters ? gamedata_character_max_level()
                          : gamedata_dish_max_level();

      if (entry >= 0) {
        if (characters)
          cfg->character_state[index].owned = tok_bool(js, &tokens[entry]);
        else
          cfg->dish_state[index].owned = tok_bool(js, &tokens[entry]);
      }
      if (level >= 0) {
        int l = tok_int(js, &tokens[level]);
        if (l < lo || l > hi) {
          warnf(warn, warn_len, "%s level %d out of range (%d..%d); ", id, l,
                lo, hi);
        } else if (characters) {
          cfg->character_state[index].level = l;
        } else {
          cfg->dish_state[index].level = l;
        }
      }
    }

    i = skip_token(tokens, count, i);
    i = skip_token(tokens, count, i);
  }
}

bool config_from_json(const char *json, size_t len, CafeConfig *cfg,
                      char *warn, size_t warn_len) {
  if (warn && warn_len > 0)
    warn[0] = '\0';
  if (!json || len == 0 || len > CONFIG_MAX_FILE || !cfg)
    return false;

  char *buffer = (char *)malloc(len + 1);
  if (!buffer)
    return false;
  memcpy(buffer, json, len);
  buffer[len] = '\0';

  jsmntok_t *tokens = (jsmntok_t *)calloc(CONFIG_MAX_TOKENS, sizeof(*tokens));
  if (!tokens) {
    free(buffer);
    return false;
  }

  jsmn_parser parser;
  jsmn_init(&parser);
  int count = jsmn_parse(&parser, buffer, len, tokens, CONFIG_MAX_TOKENS);
  if (count < 1 || tokens[0].type != JSMN_OBJECT) {
    free(tokens);
    free(buffer);
    return false;
  }

  for (int i = 0; i < count; ++i)
    if (tokens[i].type == JSMN_STRING && tokens[i].end > 0 &&
        (size_t)tokens[i].end <= len)
      buffer[tokens[i].end] = '\0';

  int format = obj_get(buffer, tokens, count, 0, "format");
  if (format >= 0 && !tok_is(buffer, &tokens[format], CONFIG_FORMAT))
    warnf(warn, warn_len, "unrecognised config format; ");
  int version = obj_get(buffer, tokens, count, 0, "version");
  if (version >= 0 && tok_int(buffer, &tokens[version]) != CONFIG_VERSION)
    warnf(warn, warn_len, "unexpected config version; ");

  CafeConfig tmp;
  config_init_defaults(&tmp);

  int cafes = obj_get(buffer, tokens, count, 0, "cafes");
  if (cafes >= 0) {
    int n = tok_int(buffer, &tokens[cafes]);
    if (n < 1 || n > MAX_CAFES)
      warnf(warn, warn_len, "cafe count %d out of range; ", n);
    else
      tmp.cafes_owned = n;
  }

  int trend = obj_get(buffer, tokens, count, 0, "trend");
  if (trend >= 0) {
    const char *name = tok_str(buffer, &tokens[trend]);
    const char *known = find_trend_name(name);
    if (known)
      tmp.trend_name = known;
    else
      warnf(warn, warn_len, "unknown trend '%s'; ", name);
  }

  int decor = obj_get(buffer, tokens, count, 0, "decor");
  if (decor >= 0)
    tmp.decor = tok_bool(buffer, &tokens[decor]);

  int popularity = obj_get(buffer, tokens, count, 0, "popularity");
  if (popularity >= 0 && tokens[popularity].type == JSMN_ARRAY) {
    int n = tokens[popularity].size;
    if (n > MAX_CAFES)
      n = MAX_CAFES;
    int i = popularity + 1;
    for (int c = 0; c < n; ++c, i = skip_token(tokens, count, i)) {
      int level = tok_int(buffer, &tokens[i]);
      int max = popularity_max_level(c);
      if (level < 0 || level > max)
        warnf(warn, warn_len, "cafe %d popularity %d out of range; ", c + 1,
              level);
      else
        tmp.cafe_popularity[c] = level;
    }
  }

  parse_entries(buffer, tokens, count,
                obj_get(buffer, tokens, count, 0, "characters"), true, &tmp,
                warn, warn_len);
  parse_entries(buffer, tokens, count,
                obj_get(buffer, tokens, count, 0, "dishes"), false, &tmp,
                warn, warn_len);

  *cfg = tmp;
  free(tokens);
  free(buffer);
  return true;
}

bool config_load_file(const char *path, CafeConfig *cfg, char *warn,
                      size_t warn_len) {
  if (warn && warn_len > 0)
    warn[0] = '\0';
  if (!path || !cfg)
    return false;

  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  char *buffer = (char *)malloc(CONFIG_MAX_FILE + 1);
  if (!buffer) {
    fclose(f);
    return false;
  }
  size_t len = fread(buffer, 1, CONFIG_MAX_FILE, f);
  fclose(f);
  bool ok = config_from_json(buffer, len, cfg, warn, warn_len);
  free(buffer);
  return ok;
}
