#include "gamedata.h"
#include "data.h"
#include "types.h"
#include "gamedata_embedded.h"
#include "jsmn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 8192
#define MAX_I18N_ENTRIES 512
#define MAX_KEPT_BUFFERS 8

typedef struct {
  const char *lang;
  const char *key;
  const char *value;
} I18nEntry;

typedef struct {
  char *buffer;
  jsmntok_t *tokens;
  int count;
} Parsed;

static int s_dish_ingredients[MAX_DISH_POOL][MAX_INGREDIENTS_PER_DISH];
static I18nEntry s_i18n[MAX_I18N_ENTRIES];
static size_t s_i18n_count = 0;

static const char *s_game_version = "";
static const char *s_content_hash = "";

/* Parsed JSON buffers stay alive for the process lifetime: all names point
 * into them. Updates add a buffer rather than moving strings underneath us. */
static char *s_kept_buffers[MAX_KEPT_BUFFERS];
static size_t s_kept_count = 0;

static bool tok_is(const char *js, const jsmntok_t *t, const char *want) {
  if (!t || (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE))
    return false;
  size_t n = (size_t)(t->end - t->start);
  return n == strlen(want) && memcmp(js + t->start, want, n) == 0;
}

/* String tokens are NUL-terminated in prepare() by replacing the closing
 * quote, so this is safe for all JSMN_STRING values. */
static const char *tok_str(const char *js, const jsmntok_t *t) {
  if (!t || t->type != JSMN_STRING || t->start < 0 || t->end <= t->start)
    return "";
  return js + t->start;
}

static double tok_number(const char *js, const jsmntok_t *t) {
  char buf[64];
  if (!t || t->start < 0 || t->end <= t->start)
    return 0.0;
  size_t n = (size_t)(t->end - t->start);
  if (n >= sizeof(buf))
    n = sizeof(buf) - 1;
  memcpy(buf, js + t->start, n);
  buf[n] = '\0';
  return strtod(buf, NULL);
}

/* Index after the token subtree rooted at idx (object/array or scalar). */
static int skip_token(const jsmntok_t *tokens, int count, int idx) {
  if (idx < 0 || idx >= count)
    return idx;

  int children = tokens[idx].size;
  int i = idx + 1;
  if (tokens[idx].type == JSMN_OBJECT) {
    for (int k = 0; k < children && i < count; ++k) {
      i = skip_token(tokens, count, i); /* key */
      i = skip_token(tokens, count, i); /* value */
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

static SkillType parse_skill_type(const char *s) {
  if (strcmp(s, "traffic_flat") == 0)
    return SKILL_TRAFFIC_FLAT;
  if (strcmp(s, "price_multiply") == 0)
    return SKILL_PRICE_MULTIPLY;
  if (strcmp(s, "traffic_multiply") == 0)
    return SKILL_TRAFFIC_MULTIPLY;
  return SKILL_PRICE_FLAT;
}

static SkillTag parse_skill_tag(const char *s) {
  if (strcmp(s, "any") == 0)
    return TAG_ANY;
  if (strcmp(s, "beverages") == 0)
    return TAG_BEVERAGES;
  if (strcmp(s, "desserts") == 0)
    return TAG_DESSERTS;
  if (strcmp(s, "main_dishes") == 0)
    return TAG_MAIN_DISHES;
  return TAG_NONE;
}

static int find_ingredient(const char *id, const Ingredient *ings, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (ings[i].id && strcmp(ings[i].id, id) == 0)
      return (int)i;
  }
  return -1;
}

static bool prepare(const char *json, size_t len, Parsed *p) {
  p->buffer = NULL;
  p->tokens = NULL;
  p->count = 0;
  if (!json || len == 0 || len > 16 * 1024 * 1024)
    return false;

  p->buffer = (char *)malloc(len + 1);
  if (!p->buffer)
    return false;
  memcpy(p->buffer, json, len);
  p->buffer[len] = '\0';

  p->tokens = (jsmntok_t *)calloc(MAX_TOKENS, sizeof(*p->tokens));
  if (!p->tokens) {
    free(p->buffer);
    p->buffer = NULL;
    return false;
  }

  jsmn_parser parser;
  jsmn_init(&parser);
  int parsed = jsmn_parse(&parser, p->buffer, len, p->tokens, MAX_TOKENS);
  if (parsed < 1 || p->tokens[0].type != JSMN_OBJECT) {
    free(p->tokens);
    free(p->buffer);
    p->tokens = NULL;
    p->buffer = NULL;
    return false;
  }
  p->count = parsed;

  /* Make every string value a normal C string by overwriting its quote. */
  for (int i = 0; i < parsed; ++i) {
    if (p->tokens[i].type == JSMN_STRING && p->tokens[i].end >= 0 &&
        (size_t)p->tokens[i].end < len)
      p->buffer[p->tokens[i].end] = '\0';
  }
  return true;
}

static bool parse_dataset(const Parsed *p, Ingredient *ings, size_t *ing_count,
                          Dish *dishes, size_t *dish_count,
                          Character *chars, size_t *char_count,
                          int ing_pool[MAX_DISH_POOL][MAX_INGREDIENTS_PER_DISH],
                          I18nEntry *i18n, size_t *i18n_count,
                          const char **version, const char **hash) {
  const char *js = p->buffer;
  const jsmntok_t *t = p->tokens;
  int n = p->count;

  int format = obj_get(js, t, n, 0, "format");
  int schema = obj_get(js, t, n, 0, "schemaVersion");
  if (format < 0 || !tok_is(js, &t[format], "nte-cafe-gamedata"))
    return false;
  if (schema < 0 || (int)tok_number(js, &t[schema]) != 1)
    return false;

  int v = obj_get(js, t, n, 0, "gameVersion");
  int h = obj_get(js, t, n, 0, "contentHash");
  *version = v >= 0 ? tok_str(js, &t[v]) : "";
  *hash = h >= 0 ? tok_str(js, &t[h]) : "";

  size_t ic = 0;
  int ingredients = obj_get(js, t, n, 0, "ingredients");
  if (ingredients < 0 || t[ingredients].type != JSMN_ARRAY)
    return false;
  int pos = ingredients + 1;
  for (int k = 0; k < t[ingredients].size && ic < MAX_INGREDIENTS; ++k) {
    int item = pos;
    pos = skip_token(t, n, pos);
    if (t[item].type != JSMN_OBJECT)
      continue;

    int id = obj_get(js, t, n, item, "id");
    int name = obj_get(js, t, n, item, "name");
    int canonical = obj_get(js, t, n, item, "canonicalName");
    int category = obj_get(js, t, n, item, "category");
    if (name < 0)
      continue;

    ings[ic].id = id >= 0 ? tok_str(js, &t[id]) : "";
    ings[ic].name = tok_str(js, &t[name]);
    ings[ic].canonical_name = canonical >= 0 ? tok_str(js, &t[canonical])
                                             : ings[ic].name;
    ings[ic].category = category >= 0 ? tok_str(js, &t[category]) : "";
    ++ic;
  }
  if (ic == 0)
    return false;
  *ing_count = ic;

  size_t dc = 0;
  int dish_array = obj_get(js, t, n, 0, "dishes");
  if (dish_array < 0 || t[dish_array].type != JSMN_ARRAY)
    return false;
  pos = dish_array + 1;
  for (int k = 0; k < t[dish_array].size && dc < MAX_DISH_POOL; ++k) {
    int item = pos;
    pos = skip_token(t, n, pos);
    if (t[item].type != JSMN_OBJECT)
      continue;

    int id = obj_get(js, t, n, item, "id");
    int name = obj_get(js, t, n, item, "name");
    int canonical = obj_get(js, t, n, item, "canonicalName");
    int type = obj_get(js, t, n, item, "type");
    int p1 = obj_get(js, t, n, item, "priceL1");
    int p2 = obj_get(js, t, n, item, "priceL2");
    int ingredients_ref = obj_get(js, t, n, item, "ingredients");
    if (name < 0)
      continue;

    size_t count = 0;
    if (ingredients_ref >= 0 && t[ingredients_ref].type == JSMN_ARRAY) {
      int ip = ingredients_ref + 1;
      for (int m = 0; m < t[ingredients_ref].size &&
                     count < MAX_INGREDIENTS_PER_DISH; ++m) {
        int ref = ip;
        ip = skip_token(t, n, ip);
        int index = find_ingredient(tok_str(js, &t[ref]), ings, ic);
        if (index >= 0)
          ing_pool[dc][count++] = index;
      }
    }

    dishes[dc].id = id >= 0 ? tok_str(js, &t[id]) : "";
    dishes[dc].name = tok_str(js, &t[name]);
    dishes[dc].canonical_name = canonical >= 0 ? tok_str(js, &t[canonical])
                                               : dishes[dc].name;
    dishes[dc].type = type >= 0 ? tok_str(js, &t[type]) : "";
    dishes[dc].price_l1 = p1 >= 0 ? tok_number(js, &t[p1]) : 0.0;
    dishes[dc].price_l2 = p2 >= 0 ? tok_number(js, &t[p2]) : 0.0;
    dishes[dc].ingredients = ing_pool[dc];
    dishes[dc].ingredient_count = count;
    ++dc;
  }
  if (dc == 0)
    return false;
  *dish_count = dc;

  size_t cc = 0;
  int characters = obj_get(js, t, n, 0, "characters");
  if (characters < 0 || t[characters].type != JSMN_ARRAY)
    return false;
  pos = characters + 1;
  for (int k = 0; k < t[characters].size && cc < MAX_CHARACTER_POOL; ++k) {
    int item = pos;
    pos = skip_token(t, n, pos);
    if (t[item].type != JSMN_OBJECT)
      continue;

    int id = obj_get(js, t, n, item, "id");
    int name = obj_get(js, t, n, item, "name");
    int canonical = obj_get(js, t, n, item, "canonicalName");
    int skills = obj_get(js, t, n, item, "skills");
    if (name < 0)
      continue;

    chars[cc].id = id >= 0 ? tok_str(js, &t[id]) : "";
    chars[cc].name = tok_str(js, &t[name]);
    chars[cc].canonical_name = canonical >= 0 ? tok_str(js, &t[canonical])
                                              : chars[cc].name;
    chars[cc].skill_count = 0;
    if (skills >= 0 && t[skills].type == JSMN_ARRAY) {
      int sp = skills + 1;
      for (int m = 0; m < t[skills].size &&
                     chars[cc].skill_count < MAX_SKILLS_PER_CHARACTER; ++m) {
        int sk = sp;
        sp = skip_token(t, n, sp);
        if (t[sk].type != JSMN_OBJECT)
          continue;

        int level = obj_get(js, t, n, sk, "level");
        int value = obj_get(js, t, n, sk, "value");
        int requirement = obj_get(js, t, n, sk, "requirement");
        int stype = obj_get(js, t, n, sk, "type");
        int tag = obj_get(js, t, n, sk, "tag");

        Skill *out = &chars[cc].skills[chars[cc].skill_count++];
        out->level = level >= 0 ? (int)tok_number(js, &t[level]) : 0;
        out->value = value >= 0 ? tok_number(js, &t[value]) : 0.0;
        out->requirement = requirement >= 0 ? (int)tok_number(js, &t[requirement]) : 0;
        out->type = stype >= 0 ? parse_skill_type(tok_str(js, &t[stype]))
                               : SKILL_PRICE_FLAT;
        out->tag = tag >= 0 ? parse_skill_tag(tok_str(js, &t[tag])) : TAG_NONE;
      }
    }
    ++cc;
  }
  if (cc == 0)
    return false;
  *char_count = cc;

  size_t icount = 0;
  int translations = obj_get(js, t, n, 0, "i18n");
  if (translations >= 0 && t[translations].type == JSMN_OBJECT) {
    int lp = translations + 1;
    for (int li = 0; li < t[translations].size && lp + 1 < n; ++li) {
      const char *lang = tok_str(js, &t[lp]);
      lp = skip_token(t, n, lp);
      int dict = lp;
      lp = skip_token(t, n, lp);
      if (dict < 0 || dict >= n || t[dict].type != JSMN_OBJECT)
        continue;

      int kv = dict + 1;
      for (int ki = 0; ki < t[dict].size && kv + 1 < n &&
                       icount < MAX_I18N_ENTRIES; ++ki) {
        const char *key = tok_str(js, &t[kv]);
        kv = skip_token(t, n, kv);
        int val = kv;
        kv = skip_token(t, n, kv);
        i18n[icount].lang = lang;
        i18n[icount].key = key;
        i18n[icount].value = tok_str(js, &t[val]);
        ++icount;
      }
    }
  }
  *i18n_count = icount;
  return true;
}

bool gamedata_init(const char *json, size_t len) {
  Parsed p;
  if (!prepare(json, len, &p))
    return false;

  Ingredient tmp_ings[MAX_INGREDIENTS];
  Dish tmp_dishes[MAX_DISH_POOL];
  Character tmp_chars[MAX_CHARACTER_POOL];
  int tmp_pool[MAX_DISH_POOL][MAX_INGREDIENTS_PER_DISH];
  I18nEntry tmp_i18n[MAX_I18N_ENTRIES];
  size_t ing_n = 0, dish_n = 0, char_n = 0, i18n_n = 0;
  const char *version = "";
  const char *hash = "";

  bool ok = parse_dataset(&p, tmp_ings, &ing_n, tmp_dishes, &dish_n,
                          tmp_chars, &char_n, tmp_pool, tmp_i18n, &i18n_n,
                          &version, &hash);
  if (!ok) {
    free(p.tokens);
    free(p.buffer);
    return false;
  }

  memcpy(INGREDIENTS, tmp_ings, ing_n * sizeof(*INGREDIENTS));
  INGREDIENT_COUNT = ing_n;
  memcpy(DISHES, tmp_dishes, dish_n * sizeof(*DISHES));
  DISH_COUNT = dish_n;
  memcpy(CHARACTERS, tmp_chars, char_n * sizeof(*CHARACTERS));
  CHARACTER_COUNT = char_n;
  memcpy(s_dish_ingredients, tmp_pool, sizeof(s_dish_ingredients));
  for (size_t i = 0; i < dish_n; ++i)
    DISHES[i].ingredients = s_dish_ingredients[i];

  memcpy(s_i18n, tmp_i18n, i18n_n * sizeof(*s_i18n));
  s_i18n_count = i18n_n;
  s_game_version = version;
  s_content_hash = hash;

  if (s_kept_count < MAX_KEPT_BUFFERS)
    s_kept_buffers[s_kept_count++] = p.buffer;
  free(p.tokens);
  return true;
}

bool gamedata_init_embedded(void) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
  const char *json = GAMEDATA_EMBEDDED_JSON;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  return gamedata_init(json, strlen(json));
}

bool gamedata_init_file(const char *path) {
  if (!path)
    return false;

  FILE *fp = fopen(path, "rb");
  if (!fp)
    return false;
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return false;
  }
  long size = ftell(fp);
  if (size <= 0 || size > 16 * 1024 * 1024) {
    fclose(fp);
    return false;
  }
  rewind(fp);

  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf) {
    fclose(fp);
    return false;
  }
  size_t read = fread(buf, 1, (size_t)size, fp);
  fclose(fp);
  if (read != (size_t)size) {
    free(buf);
    return false;
  }
  buf[read] = '\0';

  bool ok = gamedata_init(buf, read);
  free(buf);
  return ok;
}

const char *gamedata_game_version(void) { return s_game_version; }

const char *gamedata_content_hash(void) { return s_content_hash; }

const char *gamedata_i18n(const char *lang, const char *key) {
  if (!lang || !key)
    return key ? key : "";
  for (size_t i = 0; i < s_i18n_count; ++i) {
    if (strcmp(s_i18n[i].lang, lang) == 0 &&
        strcmp(s_i18n[i].key, key) == 0)
      return s_i18n[i].value;
  }
  return key;
}

size_t gamedata_i18n_count(void) { return s_i18n_count; }

bool gamedata_i18n_at(size_t index, const char **lang, const char **key,
                      const char **value) {
  if (index >= s_i18n_count)
    return false;
  if (lang)
    *lang = s_i18n[index].lang;
  if (key)
    *key = s_i18n[index].key;
  if (value)
    *value = s_i18n[index].value;
  return true;
}
