#include "config.h"
#include "data.h"
#include "gamedata.h"
#include "types.h"
#include "webui.h"
#include "trend.h"
#include "popularity.h"
#include "optimizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_HTML_PATH "/ui/index.html"
#define STATUS_HTML_PATH "/ui/status.html"
#define CHARACTERS_HTML_PATH "/ui/characters.html"
#define DISHES_HTML_PATH "/ui/dishes.html"
#define CAFES_HTML_PATH "/ui/cafes.html"

/* The desktop target compiles with NTE_CAFE_DESKTOP=1. Both binaries share
 * this source, the UI files and config.json; only the final window mode
 * differs (native WebView vs. HTTP server). --desktop/--server can override
 * the compiled default at runtime. */
#if defined(NTE_CAFE_DESKTOP) && NTE_CAFE_DESKTOP
#define DESKTOP_BY_DEFAULT true
#else
#define DESKTOP_BY_DEFAULT false
#endif

static void print_usage(const char *exe) {
  printf("Usage: %s [options]\n"
         "  --desktop   Open a native WebView window (fallback: default browser)\n"
         "  --server    Start the HTTP server (default for nte-cafe-app)\n"
         "  --help      Show this help\n\n"
         "Environment:\n"
         "  PORT              Server port (default 80, fallback 8080)\n"
         "  NTE_CAFE_CONFIG   Path to config.json (default: next to executable)\n"
         "  NTE_CAFE_GAMEDATA Path to gamedata.json (default: next to executable)\n",
         exe ? exe : "nte-cafe-app");
}

static char *read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long n = ftell(f);
  if (n < 0) {
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }

  char *buf = (char *)malloc((size_t)n + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t read_n = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[read_n] = '\0';
  if (out_len)
    *out_len = read_n;
  return buf;
}

static const char *app_root = ".";

/* Per-WebUI-client state. Keeps users/sessions isolated in memory while the
 * server runs; persistence across restarts is handled separately later. */
typedef struct {
  size_t client_id;
  bool used;
  bool character_state_initialized;
  bool dish_state_initialized;
  bool cafe_settings_initialized;
  CharacterState character_state[MAX_CHARACTER_POOL];
  DishState dish_state[MAX_DISH_POOL];
  CafeSettings cafe_settings;
} ClientState;

#define MAX_CLIENT_STATES 256
static ClientState client_states[MAX_CLIENT_STATES];

/* One shared config.json, in the same format as the CLI, seeds every new
 * WebUI client. It is loaded once at startup and can be saved/loaded from the
 * settings panel without restarting. */
static CafeConfig g_app_config;
static char g_config_path[1200] = "";

static Trend find_trend_by_name(const char *name) {
  TrendOptions opts = get_trend_options();
  for (size_t i = 0; i < opts.count; ++i)
    if (opts.trends[i].name && name && strcmp(opts.trends[i].name, name) == 0)
      return opts.trends[i];
  Trend none = {TREND_NONE, "None"};
  return none;
}

static void apply_trend(CafeSettings *cs, const char *name) {
  Trend t = find_trend_by_name(name);
  cs->trend_name = t.name;
  cs->trend_tag = get_trend_tag(t);
  cs->trend_bonus = get_trend_bonus(t.type);
}

static void init_default_cafe_settings(CafeSettings *cs) {
  memset(cs, 0, sizeof(*cs));
  cs->cafes_owned = g_app_config.cafes_owned;
  memcpy(cs->cafe_popularity, g_app_config.cafe_popularity,
         sizeof(cs->cafe_popularity));
  cs->hotori_cat_decor = g_app_config.decor;
  apply_trend(cs, g_app_config.trend_name);
  cs->popularity_bonus = popularity_total_bonus(cs);
}

/* Defaults come from config.json; when none exists they are all minimums. */
static void init_default_character_state(ClientState *cs) {
  for (size_t i = 0; i < CHARACTER_COUNT && i < MAX_CHARACTER_POOL; i++)
    cs->character_state[i] = g_app_config.character_state[i];
  cs->character_state_initialized = true;
}

static void init_default_dish_state(ClientState *cs) {
  for (size_t i = 0; i < DISH_COUNT && i < MAX_DISH_POOL; i++)
    cs->dish_state[i] = g_app_config.dish_state[i];
  cs->dish_state_initialized = true;
}

/* Look up or create state for a WebUI client. If the fixed table is full,
 * reuse the first slot (oldest simple eviction) to avoid unbounded memory. */
static ClientState *get_client_state(size_t client_id) {
  ClientState *free_slot = NULL;
  for (size_t i = 0; i < MAX_CLIENT_STATES; i++) {
    if (client_states[i].used && client_states[i].client_id == client_id) return &client_states[i];
    if (!client_states[i].used && !free_slot) free_slot = &client_states[i];
  }
  ClientState *cs = free_slot ? free_slot : &client_states[0];
  memset(cs, 0, sizeof(*cs));
  cs->used = true;
  cs->client_id = client_id;
  init_default_cafe_settings(&cs->cafe_settings);
  cs->cafe_settings_initialized = true;
  init_default_character_state(cs);
  init_default_dish_state(cs);
  return cs;
}

static const void *file_handler(const char *filename, int *length) {
  const char *path = filename;
  if (!path || path[0] == '\0' || strcmp(path, "/") == 0 ||
      strcmp(path, "/index.html") == 0 || strcmp(path, "/ui/index.html") == 0 ||
      strcmp(path, "ui/index.html") == 0)
    path = APP_HTML_PATH;
  else if (strcmp(path, "/status") == 0 || strcmp(path, "/status.html") == 0 ||
           strcmp(path, "/ui/status.html") == 0)
    path = STATUS_HTML_PATH;
  else if (strcmp(path, "/characters") == 0 || strcmp(path, "/characters.html") == 0 ||
           strcmp(path, "/ui/characters.html") == 0)
    path = CHARACTERS_HTML_PATH;
  else if (strcmp(path, "/dishes") == 0 || strcmp(path, "/dishes.html") == 0 ||
           strcmp(path, "/ui/dishes.html") == 0)
    path = DISHES_HTML_PATH;
  else if (strcmp(path, "/cafes") == 0 || strcmp(path, "/cafes.html") == 0 ||
           strcmp(path, "/ui/cafes.html") == 0)
    path = CAFES_HTML_PATH;

  if (path[0] == '/')
    path++;

  char full[1024];
  snprintf(full, sizeof(full), "%s/%s", app_root, path);

  size_t body_len = 0;
  char *body = read_file(full, &body_len);
  if (!body) {
    const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/plain; charset=utf-8\r\n"
                            "Cache-Control: no-cache\r\n\r\n"
                            "Not Found";
    size_t n = strlen(not_found);
    char *resp = (char *)webui_malloc(n + 1);
    if (!resp) {
      *length = 0;
      return NULL;
    }
    memcpy(resp, not_found, n + 1);
    *length = (int)n;
    return resp;
  }

  const char *mime = webui_get_mime_type(path);
  if (!mime || mime[0] == '\0')
    mime = "application/octet-stream";

  char header[512];
  int header_len = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: %s\r\n"
                            "Cache-Control: no-cache\r\n"
                            "Content-Length: %zu\r\n\r\n",
                            mime, body_len);
  if (header_len < 0) {
    free(body);
    *length = 0;
    return NULL;
  }

  size_t total = (size_t)header_len + body_len;
  char *resp = (char *)webui_malloc(total);
  if (!resp) {
    free(body);
    *length = 0;
    return NULL;
  }

  memcpy(resp, header, (size_t)header_len);
  memcpy(resp + header_len, body, body_len);
  free(body);

  *length = (int)total;
  return resp;
}

static void json_escape(const char *src, char *dst, size_t dst_size) {
  size_t j = 0;
  if (dst_size == 0)
    return;
  for (size_t i = 0; src && src[i] != '\0' && j + 2 < dst_size; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c == '"' || c == '\\') {
      if (j + 2 >= dst_size)
        break;
      dst[j++] = '\\';
      dst[j++] = (char)c;
    } else if (c == '\n') {
      if (j + 2 >= dst_size)
        break;
      dst[j++] = '\\';
      dst[j++] = 'n';
    } else if (c == '\r') {
      if (j + 2 >= dst_size)
        break;
      dst[j++] = '\\';
      dst[j++] = 'r';
    } else if (c == '\t') {
      if (j + 2 >= dst_size)
        break;
      dst[j++] = '\\';
      dst[j++] = 't';
    } else if (c < 0x20) {
      /* skip control chars */
    } else {
      dst[j++] = (char)c;
    }
  }
  dst[j] = '\0';
}

static void get_app_info(webui_event_t *e) {
  char buf[512];
  snprintf(buf, sizeof(buf),
           "{\"app\":\"NTE Cafe Optimizer\",\"webui\":\"%s\",\"dishCount\":%zu,"
           "\"characterCount\":%zu,\"ingredientCount\":%zu,"
           "\"gameVersion\":\"%s\",\"contentHash\":\"%s\"}",
           WEBUI_VERSION, DISH_COUNT, CHARACTER_COUNT, INGREDIENT_COUNT,
           gamedata_game_version(), gamedata_content_hash());
  webui_return_string(e, buf);
}

static void get_dishes(webui_event_t *e) {
  size_t cap = 512 + DISH_COUNT * 512;
  char *buf = (char *)malloc(cap);
  if (!buf) { webui_return_string(e, "[]"); return; }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < DISH_COUNT && off + 300 < cap; i++) {
    char name[128], type[128];
    json_escape(DISHES[i].name ? DISHES[i].name : "", name, sizeof(name));
    json_escape(DISHES[i].type ? DISHES[i].type : "", type, sizeof(type));
    off += snprintf(buf + off, cap - off,
                    "%s{\"name\":\"%s\",\"type\":\"%s\",\"priceL1\":%.2f,\"priceL2\":%.2f,\"ingredients\":[",
                    i ? "," : "", name, type, DISHES[i].price_l1, DISHES[i].price_l2);
    for (size_t j = 0; j < DISHES[i].ingredient_count && off + 80 < cap; j++) {
      int idx = DISHES[i].ingredients[j];
      const char *iname = (idx >= 0 && (size_t)idx < INGREDIENT_COUNT && INGREDIENTS[idx].name) ? INGREDIENTS[idx].name : "?";
      char ing[128];
      json_escape(iname, ing, sizeof(ing));
      off += snprintf(buf + off, cap - off, "%s\"%s\"", j ? "," : "", ing);
    }
    if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]}");
  }
  if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

static void get_dish_states(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  size_t cap = 512 + DISH_COUNT * 640;
  char *buf = (char *)malloc(cap);
  if (!buf) { webui_return_string(e, "[]"); return; }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < DISH_COUNT && off + 340 < cap; i++) {
    char name[128], type[128];
    json_escape(DISHES[i].name ? DISHES[i].name : "", name, sizeof(name));
    json_escape(DISHES[i].type ? DISHES[i].type : "", type, sizeof(type));
    off += snprintf(buf + off, cap - off,
                    "%s{\"name\":\"%s\",\"type\":\"%s\",\"priceL1\":%.2f,\"priceL2\":%.2f,\"owned\":%s,\"level\":%d,\"ingredients\":[",
                    i ? "," : "", name, type, DISHES[i].price_l1, DISHES[i].price_l2,
                    cs->dish_state[i].owned ? "true" : "false", cs->dish_state[i].level);
    for (size_t j = 0; j < DISHES[i].ingredient_count && off + 80 < cap; j++) {
      int idx = DISHES[i].ingredients[j];
      const char *iname = (idx >= 0 && (size_t)idx < INGREDIENT_COUNT && INGREDIENTS[idx].name) ? INGREDIENTS[idx].name : "?";
      char ing[128];
      json_escape(iname, ing, sizeof(ing));
      off += snprintf(buf + off, cap - off, "%s\"%s\"", j ? "," : "", ing);
    }
    if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]}");
  }
  if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

static void set_dish_state(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long idx = webui_get_int_at(e, 0);
  bool owned = webui_get_bool_at(e, 1);
  long long level = webui_get_int_at(e, 2);
  if (idx < 0 || (size_t)idx >= DISH_COUNT) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"invalid index\"}");
    return;
  }
  if (level < 1) level = 1;
  if (level > 2) level = 2;
  cs->dish_state[idx].owned = owned;
  cs->dish_state[idx].level = (int)level;
  webui_return_string(e, "{\"ok\":true}");
}

static unsigned long long nck_u64(size_t n, size_t k) {
  if (k > n) return 0;
  if (k > n - k) k = n - k;
  unsigned long long r = 1;
  for (size_t i = 1; i <= k; i++) {
    r = (r * (unsigned long long)(n - k + i)) / (unsigned long long)i;
  }
  return r;
}

static void get_optimizer_overview(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  size_t owned_chars = 0;
  for (size_t i = 0; i < CHARACTER_COUNT; i++)
    if (cs->character_state[i].owned && cs->character_state[i].level > 0) owned_chars++;
  size_t owned_dishes_l1 = 0, owned_dishes_l2 = 0, owned_dishes = 0;
  for (size_t i = 0; i < DISH_COUNT; i++) {
    if (!cs->dish_state[i].owned) continue;
    owned_dishes++;
    if (cs->dish_state[i].level <= 1) owned_dishes_l1++; else owned_dishes_l2++;
  }
  size_t dish_slots = (size_t)cs->cafe_settings.cafes_owned;
  size_t char_slots = (size_t)cs->cafe_settings.cafes_owned * 2;
  if (dish_slots > MAX_DISH_SLOTS) dish_slots = MAX_DISH_SLOTS;
  if (char_slots > MAX_CHARACTER_SLOTS) char_slots = MAX_CHARACTER_SLOTS;
  unsigned long long dish_combos = nck_u64(owned_dishes, dish_slots);
  unsigned long long char_combos = nck_u64(owned_chars, char_slots);
  unsigned long long combos = 0;
  if (dish_combos == 0 || char_combos == 0) combos = 0;
  else if (char_combos > 0 && dish_combos > (18446744073709551615ULL / char_combos)) combos = 18446744073709551615ULL;
  else combos = dish_combos * char_combos;

  bool is_default = (owned_chars == 0 && owned_dishes == 0 &&
                     cs->cafe_settings.cafes_owned == 1 &&
                     !cs->cafe_settings.hotori_cat_decor &&
                     cs->cafe_settings.trend_tag == TAG_NONE &&
                     popularity_total_bonus(&cs->cafe_settings) <= 0.0);
  char buf[720];
  snprintf(buf, sizeof(buf),
           "{\"loadedCharacters\":%zu,\"ownedDishes\":%zu,\"ownedDishesL1\":%zu,\"ownedDishesL2\":%zu,\"dishSlots\":%zu,\"characterSlots\":%zu,\"dishCombinations\":\"%llu\",\"characterCombinations\":\"%llu\",\"possibleCombinations\":\"%llu\",\"isDefault\":%s}",
           owned_chars, owned_dishes, owned_dishes_l1, owned_dishes_l2, dish_slots, char_slots, dish_combos, char_combos, combos, is_default ? "true" : "false");
  webui_return_string(e, buf);
}

static void get_optimizer_settings(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  TrendOptions opts = get_trend_options();
  size_t cap = 4096 + opts.count * 128 + MAX_CAFES * 512;
  char *buf = (char*)malloc(cap);
  if (!buf) { webui_return_string(e, "{}"); return; }
  size_t off = 0;
  char trend_name[128];
  json_escape(cs->cafe_settings.trend_name ? cs->cafe_settings.trend_name : "None", trend_name, sizeof(trend_name));
  off += snprintf(buf+off, cap-off, "{\"trend\":\"%s\",\"hotoriCatDecor\":%s,\"cafesOwned\":%d,\"trendBonus\":%.2f,\"trends\":[",
                  trend_name, cs->cafe_settings.hotori_cat_decor ? "true":"false", cs->cafe_settings.cafes_owned, cs->cafe_settings.trend_bonus);
  for (size_t i = 0; i < opts.count && off + 180 < cap; i++) {
    char n[128]; json_escape(opts.trends[i].name ? opts.trends[i].name : "", n, sizeof(n));
    const char *tt = "none";
    if (opts.trends[i].type == TREND_TAG) tt = "tag";
    else if (opts.trends[i].type == TREND_CATEGORY) tt = "category";
    else if (opts.trends[i].type == TREND_INGREDIENT) tt = "ingredient";
    off += snprintf(buf+off, cap-off, "%s{\"name\":\"%s\",\"type\":\"%s\",\"bonus\":%.2f}", i?",":"", n, tt, get_trend_bonus(opts.trends[i].type));
  }
  if (off + 16 < cap) off += snprintf(buf+off, cap-off, "],\"cafes\":[");
  for (int c = 0; c < MAX_CAFES && off + 400 < cap; c++) {
    int selected = cs->cafe_settings.cafe_popularity[c];
    off += snprintf(buf+off, cap-off, "%s{\"index\":%d,\"maxLevel\":%d,\"selected\":%d,\"bonus\":%.3f,\"levels\":[",
                    c ? "," : "", c, popularity_max_level(c), selected,
                    popularity_cafe_bonus(&cs->cafe_settings, c));
    for (int l = 1; l <= popularity_max_level(c); l++) {
      off += snprintf(buf+off, cap-off, "%s{\"level\":%d,\"popularity\":%d,\"bonus\":%.3f}",
                      l > 1 ? "," : "", l, popularity_threshold(c, l),
                      popularity_level_bonus(c, l));
    }
    if (off + 4 < cap) off += snprintf(buf+off, cap-off, "]}");
  }
  if (off + 48 < cap)
    off += snprintf(buf+off, cap-off, "],\"popularityBonus\":%.3f}",
                    popularity_total_bonus(&cs->cafe_settings));
  webui_return_string(e, buf);
  free(buf);
}

static void set_optimizer_settings(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  const char *trend_name = webui_get_string_at(e, 0);
  bool hotori = webui_get_bool_at(e, 1);
  long long cafes = webui_get_int_at(e, 2);
  TrendOptions opts = get_trend_options();
  Trend selected = {TREND_NONE, "None"};
  for (size_t i = 0; i < opts.count; i++) {
    if (opts.trends[i].name && trend_name && strcmp(opts.trends[i].name, trend_name) == 0) {
      selected = opts.trends[i]; break;
    }
  }
  if (cafes < 1) cafes = 1;
  if (cafes > 5) cafes = 5;
  cs->cafe_settings.cafes_owned = (int)cafes;
  cs->cafe_settings.trend_name = selected.name;
  cs->cafe_settings.trend_tag = get_trend_tag(selected);
  cs->cafe_settings.trend_bonus = get_trend_bonus(selected.type);
  cs->cafe_settings.hotori_cat_decor = hotori;
  cs->cafe_settings.popularity_bonus =
      popularity_total_bonus(&cs->cafe_settings);
  webui_return_string(e, "{\"ok\":true}");
}

static void set_cafe_popularity(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long idx = webui_get_int_at(e, 0);
  long long level = webui_get_int_at(e, 1);
  if (idx < 0 || idx >= MAX_CAFES) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"invalid cafe index\"}");
    return;
  }
  int max_level = popularity_max_level((int)idx);
  if (level < 0) level = 0;
  if (level > max_level) level = max_level;
  cs->cafe_settings.cafe_popularity[idx] = (int)level;
  cs->cafe_settings.popularity_bonus =
      popularity_total_bonus(&cs->cafe_settings);
  webui_return_string(e, "{\"ok\":true}");
}

static void set_cafes_owned(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long cafes = webui_get_int_at(e, 0);
  if (cafes < 1) cafes = 1;
  if (cafes > MAX_CAFES) cafes = MAX_CAFES;
  cs->cafe_settings.cafes_owned = (int)cafes;
  cs->cafe_settings.popularity_bonus =
      popularity_total_bonus(&cs->cafe_settings);
  webui_return_string(e, "{\"ok\":true}");
}

/* mode: 1 all max level, 2 all min level, 3 all owned,
 *       4 all unowned, 5 reset everything to the default state. */
static void set_all_character_states(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long mode = webui_get_int_at(e, 0);
  for (size_t i = 0; i < CHARACTER_COUNT && i < MAX_CHARACTER_POOL; i++) {
    if (mode == 1)
      cs->character_state[i].level = 5;
    else if (mode == 2 || mode == 5)
      cs->character_state[i].level = 0;
    if (mode == 3)
      cs->character_state[i].owned = true;
    else if (mode == 4 || mode == 5)
      cs->character_state[i].owned = false;
  }
  webui_return_string(e, "{\"ok\":true}");
}

static void set_all_dish_states(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long mode = webui_get_int_at(e, 0);
  for (size_t i = 0; i < DISH_COUNT && i < MAX_DISH_POOL; i++) {
    if (mode == 1)
      cs->dish_state[i].level = 2;
    else if (mode == 2 || mode == 5)
      cs->dish_state[i].level = 1;
    if (mode == 3)
      cs->dish_state[i].owned = true;
    else if (mode == 4 || mode == 5)
      cs->dish_state[i].owned = false;
  }
  webui_return_string(e, "{\"ok\":true}");
}

static void run_optimization(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  CafeContext ctx = {
    .dishes = DISHES, .dish_count = DISH_COUNT,
    .characters = CHARACTERS, .character_count = CHARACTER_COUNT,
    .dish_state = cs->dish_state, .character_state = cs->character_state,
    .settings = cs->cafe_settings
  };
  OptimizationResult res;
  if (!optimize_cafe(&ctx, &res)) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"Optimization failed. Check owned dishes/characters and cafe count.\"}");
    return;
  }
  size_t cap = 4096;
  char *buf = (char*)malloc(cap);
  if (!buf) { webui_return_string(e, "{\"ok\":false,\"error\":\"Out of memory\"}"); return; }
  size_t off = 0;
  char trend_name_esc[128]; json_escape(cs->cafe_settings.trend_name ? cs->cafe_settings.trend_name : "None", trend_name_esc, sizeof(trend_name_esc));
  off += snprintf(buf+off, cap-off, "{\"ok\":true,\"income\":%.2f,\"traffic\":%.2f,\"priceBuff\":%.2f,\"priceMultiplier\":%.2f,\"trend\":\"%s\",\"trendBonus\":%.2f,\"popularityBonus\":%.3f,\"dishes\":[", res.income, res.traffic, res.price_buff, res.price_multiplier, trend_name_esc, cs->cafe_settings.trend_bonus, res.popularity_bonus);
  for (size_t i = 0; i < res.dish_count && off + 360 < cap; i++) {
    char n[128]; json_escape(res.dishes[i]->name ? res.dishes[i]->name : "", n, sizeof(n));
    int level = 1;
    double base_price = 0.0;
    for (size_t d = 0; d < DISH_COUNT; d++) {
      if (&DISHES[d] == res.dishes[i]) {
        level = cs->dish_state[d].level;
        base_price = dish_base_price(res.dishes[i], level, &cs->cafe_settings);
        break;
      }
    }
    bool in_trend = dish_matches_trend(res.dishes[i], &cs->cafe_settings);
    double trend_bonus = in_trend ? cs->cafe_settings.trend_bonus : 0.0;
    double fph = ((base_price - trend_bonus + res.price_buff) * res.price_multiplier +
                  trend_bonus) * (res.traffic / 100.0) *
                 (1.0 + res.popularity_bonus);
    off += snprintf(buf+off, cap-off, "%s{\"name\":\"%s\",\"level\":%d,\"fonsPerHour\":%.2f,\"inTrend\":%s}", i?",":"", n, level, fph, in_trend ? "true" : "false");
  }
  off += snprintf(buf+off, cap-off, "],\"characters\":[");
  for (size_t i = 0; i < res.character_count && off + 256 < cap; i++) {
    char n[128]; json_escape(res.characters[i]->name ? res.characters[i]->name : "", n, sizeof(n));
    off += snprintf(buf+off, cap-off, "%s{\"name\":\"%s\"}", i?",":"", n);
  }
  off += snprintf(buf+off, cap-off, "]}");
  webui_return_string(e, buf);
  free(buf);
}

static void append_character_json(char *buf, size_t cap, size_t *off, size_t i, bool include_state, ClientState *cs) {
  char name[128];
  json_escape(CHARACTERS[i].name ? CHARACTERS[i].name : "", name, sizeof(name));
  *off += snprintf(buf + *off, cap - *off, "%s{\"name\":\"%s\",\"skillCount\":%zu",
                  i ? "," : "", name, CHARACTERS[i].skill_count);
  if (include_state) {
    *off += snprintf(buf + *off, cap - *off, ",\"owned\":%s,\"level\":%d",
                    cs->character_state[i].owned ? "true" : "false", cs->character_state[i].level);
  }
  *off += snprintf(buf + *off, cap - *off, ",\"skills\":[");
  for (size_t s = 0; s < CHARACTERS[i].skill_count && *off + 160 < cap; s++) {
    const Skill *sk = &CHARACTERS[i].skills[s];
    const char *stype = "unknown";
    switch (sk->type) {
    case SKILL_PRICE_FLAT: stype = "price_flat"; break;
    case SKILL_TRAFFIC_FLAT: stype = "traffic_flat"; break;
    case SKILL_PRICE_MULTIPLY: stype = "price_mult"; break;
    case SKILL_TRAFFIC_MULTIPLY: stype = "traffic_mult"; break;
    }
    const char *stag = "none";
    switch (sk->tag) {
    case TAG_NONE: stag = "none"; break;
    case TAG_ANY: stag = "any"; break;
    case TAG_BEVERAGES: stag = "beverages"; break;
    case TAG_DESSERTS: stag = "desserts"; break;
    case TAG_MAIN_DISHES: stag = "main_dishes"; break;
    }
    *off += snprintf(buf + *off, cap - *off,
                    "%s{\"level\":%d,\"value\":%.4f,\"req\":%d,\"type\":\"%s\",\"tag\":\"%s\"}",
                    s ? "," : "", sk->level, sk->value, sk->requirement, stype, stag);
  }
  if (*off + 4 < cap) *off += snprintf(buf + *off, cap - *off, "]}");
}

static void get_characters(webui_event_t *e) {
  size_t cap = 256 + CHARACTER_COUNT * 512;
  char *buf = (char *)malloc(cap);
  if (!buf) { webui_return_string(e, "[]"); return; }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < CHARACTER_COUNT && off + 300 < cap; i++) append_character_json(buf, cap, &off, i, false, NULL);
  if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

static void get_character_states(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  size_t cap = 256 + CHARACTER_COUNT * 640;
  char *buf = (char *)malloc(cap);
  if (!buf) { webui_return_string(e, "[]"); return; }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < CHARACTER_COUNT && off + 340 < cap; i++) append_character_json(buf, cap, &off, i, true, cs);
  if (off + 2 < cap) off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

static void set_character_state(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  long long idx = webui_get_int_at(e, 0);
  bool owned = webui_get_bool_at(e, 1);
  long long level = webui_get_int_at(e, 2);
  if (idx < 0 || (size_t)idx >= CHARACTER_COUNT) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"invalid index\"}");
    return;
  }
  if (level < 0) level = 0;
  if (level > 5) level = 5;
  cs->character_state[idx].owned = owned;
  cs->character_state[idx].level = (int)level;
  webui_return_string(e, "{\"ok\":true}");
}

static void update_gamedata(webui_event_t *e) {
  const char *json = webui_get_string_at(e, 0);
  if (!json || !gamedata_init(json, strlen(json))) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"invalid game data\"}");
    return;
  }

  char buf[256];
  snprintf(buf, sizeof(buf),
           "{\"ok\":true,\"gameVersion\":\"%s\",\"contentHash\":\"%s\"}",
           gamedata_game_version(), gamedata_content_hash());
  webui_return_string(e, buf);
}

static void get_i18n(webui_event_t *e) {
  const char *lang = webui_get_string_at(e, 0);
  if (!lang || !*lang)
    lang = "en";

  const size_t cap = 128 * 1024;
  char *buf = (char *)malloc(cap);
  if (!buf) {
    webui_return_string(e, "{}");
    return;
  }

  size_t off = 0;
  off += (size_t)snprintf(buf, cap, "{");
  bool first = true;
  for (size_t i = 0; i < gamedata_i18n_count(); ++i) {
    const char *entry_lang = NULL, *key = NULL, *value = NULL;
    if (!gamedata_i18n_at(i, &entry_lang, &key, &value) ||
        strcmp(entry_lang, lang) != 0)
      continue;

    char escaped_key[256], escaped_value[1024];
    json_escape(key, escaped_key, sizeof(escaped_key));
    json_escape(value, escaped_value, sizeof(escaped_value));
    int written = snprintf(buf + off, cap - off, "%s\"%s\":\"%s\"",
                           first ? "" : ",", escaped_key, escaped_value);
    if (written <= 0 || (size_t)written >= cap - off)
      break;
    off += (size_t)written;
    first = false;
  }
  snprintf(buf + off, cap - off, "}");
  webui_return_string(e, buf);
  free(buf);
}

static void client_to_config(const ClientState *cs, CafeConfig *cfg) {
  config_init_defaults(cfg);
  memcpy(cfg->dish_state, cs->dish_state, sizeof(cfg->dish_state));
  memcpy(cfg->character_state, cs->character_state,
         sizeof(cfg->character_state));
  cfg->cafes_owned = cs->cafe_settings.cafes_owned;
  memcpy(cfg->cafe_popularity, cs->cafe_settings.cafe_popularity,
         sizeof(cfg->cafe_popularity));
  cfg->trend_name =
      cs->cafe_settings.trend_name ? cs->cafe_settings.trend_name : "None";
  cfg->decor = cs->cafe_settings.hotori_cat_decor;
}

static void config_to_client(const CafeConfig *cfg, ClientState *cs) {
  memcpy(cs->dish_state, cfg->dish_state, sizeof(cs->dish_state));
  memcpy(cs->character_state, cfg->character_state,
         sizeof(cs->character_state));
  cs->cafe_settings.cafes_owned = cfg->cafes_owned;
  memcpy(cs->cafe_settings.cafe_popularity, cfg->cafe_popularity,
         sizeof(cs->cafe_settings.cafe_popularity));
  cs->cafe_settings.hotori_cat_decor = cfg->decor;
  apply_trend(&cs->cafe_settings, cfg->trend_name);
  cs->cafe_settings.popularity_bonus =
      popularity_total_bonus(&cs->cafe_settings);
  cs->dish_state_initialized = true;
  cs->character_state_initialized = true;
  cs->cafe_settings_initialized = true;
}

/* Returns the canonical config JSON for the current client, so WebUI export
 * and the CLI share one file format. */
static void export_config(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  CafeConfig cfg;
  client_to_config(cs, &cfg);
  char json[CONFIG_JSON_MAX];
  if (!config_to_json(&cfg, json, sizeof(json))) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"serialization failed\"}");
    return;
  }
  webui_return_string(e, json);
}

static void import_config(webui_event_t *e) {
  const char *json = webui_get_string_at(e, 0);
  CafeConfig cfg;
  char warn[512];
  if (!json || !config_from_json(json, strlen(json), &cfg, warn, sizeof(warn))) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"invalid config\"}");
    return;
  }
  config_to_client(&cfg, get_client_state(e->client_id));
  char escaped[700];
  char buf[800];
  json_escape(warn, escaped, sizeof(escaped));
  snprintf(buf, sizeof(buf), "{\"ok\":true,\"warning\":\"%s\"}", escaped);
  webui_return_string(e, buf);
}

static void save_config(webui_event_t *e) {
  ClientState *cs = get_client_state(e->client_id);
  CafeConfig cfg;
  client_to_config(cs, &cfg);
  webui_return_string(e, config_save_file(g_config_path, &cfg)
                             ? "{\"ok\":true}"
                             : "{\"ok\":false,\"error\":\"write failed\"}");
}

static void load_config(webui_event_t *e) {
  CafeConfig cfg;
  char warn[512];
  if (!config_load_file(g_config_path, &cfg, warn, sizeof(warn))) {
    webui_return_string(e, "{\"ok\":false,\"error\":\"read failed\"}");
    return;
  }
  config_to_client(&cfg, get_client_state(e->client_id));
  char escaped[700];
  char buf[800];
  json_escape(warn, escaped, sizeof(escaped));
  snprintf(buf, sizeof(buf), "{\"ok\":true,\"warning\":\"%s\"}", escaped);
  webui_return_string(e, buf);
}

static void get_config_path(webui_event_t *e) {
  char escaped[1300];
  char buf[1400];
  json_escape(g_config_path, escaped, sizeof(escaped));
  snprintf(buf, sizeof(buf), "{\"path\":\"%s\"}", escaped);
  webui_return_string(e, buf);
}

int main(int argc, char **argv) {
  bool desktop = DESKTOP_BY_DEFAULT;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--desktop") == 0)
      desktop = true;
    else if (strcmp(argv[i], "--server") == 0)
      desktop = false;
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argc > 0 ? argv[0] : NULL);
      return 0;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      print_usage(argc > 0 ? argv[0] : NULL);
      return 1;
    }
  }

  if (argc > 0 && argv[0] && argv[0][0] != '\0') {
    static char root_buf[1024];
    snprintf(root_buf, sizeof(root_buf), "%s", argv[0]);
    /* Windows uses backslashes, POSIX uses slashes; accept both. */
    char *sep = NULL;
    for (char *p = root_buf; *p; ++p)
      if (*p == '/' || *p == '\\')
        sep = p;
    if (sep == root_buf)
      root_buf[1] = '\0'; /* filesystem root */
    else if (sep)
      *sep = '\0';
    else
      root_buf[0] = '\0'; /* bare executable name: use current directory */
    app_root = root_buf;
  }

  const char *data_env = getenv("NTE_CAFE_GAMEDATA");
  bool data_loaded = false;
  if (data_env && *data_env)
    data_loaded = gamedata_init_file(data_env);
  if (!data_loaded && app_root) {
    char data_path[1200];
    snprintf(data_path, sizeof(data_path), "%s/data/gamedata.json", app_root);
    data_loaded = gamedata_init_file(data_path);
  }
  if (!data_loaded) {
    data_loaded = gamedata_init_embedded();
    if (data_loaded)
      fprintf(stderr, "External game data unavailable; using embedded fallback\n");
  }
  if (!data_loaded) {
    fprintf(stderr, "Failed to load game data\n");
    return 1;
  }
  printf("Game data: version %s, hash %s, %zu dishes, %zu characters\n",
         gamedata_game_version(), gamedata_content_hash(), DISH_COUNT,
         CHARACTER_COUNT);

  config_init_defaults(&g_app_config);
  const char *config_env = getenv("NTE_CAFE_CONFIG");
  char *default_config = config_default_path(argv[0]);
  if (config_env && *config_env)
    snprintf(g_config_path, sizeof(g_config_path), "%s", config_env);
  else
    snprintf(g_config_path, sizeof(g_config_path), "%s",
             default_config ? default_config : "config.json");
  free(default_config);
  char config_warn[512];
  if (config_load_file(g_config_path, &g_app_config, config_warn,
                       sizeof(config_warn)))
    printf("Config: %s%s\n", g_config_path,
           config_warn[0] ? " (warnings on load)" : "");
  else
    printf("Config: %s not found; using minimum defaults\n", g_config_path);

  webui_set_config(multi_client, !desktop);
  size_t win = webui_new_window();
  webui_set_file_handler(win, file_handler);
  webui_set_public(win, !desktop);
  webui_bind(win, "getAppInfo", get_app_info);
  webui_bind(win, "getDishes", get_dishes);
  webui_bind(win, "getDishStates", get_dish_states);
  webui_bind(win, "setDishState", set_dish_state);
  webui_bind(win, "setAllDishStates", set_all_dish_states);
  webui_bind(win, "getOptimizerSettings", get_optimizer_settings);
  webui_bind(win, "setOptimizerSettings", set_optimizer_settings);
  webui_bind(win, "setCafePopularity", set_cafe_popularity);
  webui_bind(win, "setCafesOwned", set_cafes_owned);
  webui_bind(win, "getOptimizerOverview", get_optimizer_overview);
  webui_bind(win, "runOptimization", run_optimization);
  webui_bind(win, "getCharacters", get_characters);
  webui_bind(win, "getCharacterStates", get_character_states);
  webui_bind(win, "setCharacterState", set_character_state);
  webui_bind(win, "setAllCharacterStates", set_all_character_states);
  webui_bind(win, "getI18n", get_i18n);
  webui_bind(win, "updateGamedata", update_gamedata);
  webui_bind(win, "exportConfig", export_config);
  webui_bind(win, "importConfig", import_config);
  webui_bind(win, "saveConfig", save_config);
  webui_bind(win, "loadConfig", load_config);
  webui_bind(win, "getConfigPath", get_config_path);

  if (!desktop) {
    const char *port_env = getenv("PORT");
    size_t port = 80;
    if (port_env && *port_env) {
      char *end = NULL;
      unsigned long parsed = strtoul(port_env, &end, 10);
      if (end != port_env && *end == '\0' && parsed > 0 && parsed <= 65535) {
        port = (size_t)parsed;
      }
    }
    if (!webui_set_port(win, port)) {
      if (port == 80 && webui_set_port(win, 8080)) {
        port = 8080;
        fprintf(stderr, "Warning: could not bind port 80, falling back to 8080\n");
      } else {
        fprintf(stderr, "Warning: could not set preferred port %zu, using a free port\n", port);
      }
    }

    const char *url = webui_start_server(win, "/");
    if (!url) {
      fprintf(stderr, "Failed to start WebUI server\n");
      return 1;
    }
    printf("NTE Cafe server running at %s\n", url);
  } else {
    webui_set_size(win, 1280, 800);
    webui_set_center(win);
    bool shown = webui_show_wv(win, "/index.html");
    if (!shown) {
      fprintf(stderr, "WebView unavailable; opening in the default browser\n");
      shown = webui_show(win, "/index.html");
    }
    if (!shown) {
      fprintf(stderr, "Failed to open the desktop window\n");
      return 1;
    }
    printf("NTE Cafe desktop running (config: %s)\n", g_config_path);
  }
  fflush(stdout);

  webui_wait();
  webui_clean();
  return 0;
}
