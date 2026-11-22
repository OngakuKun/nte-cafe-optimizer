#include "data.h"
#include "types.h"
#include "webui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_HTML_PATH "/ui/index.html"

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

static const void *file_handler(const char *filename, int *length) {
  const char *path = filename;
  if (!path || path[0] == '\0' || strcmp(path, "/") == 0 ||
      strcmp(path, "/index.html") == 0 || strcmp(path, "/ui/index.html") == 0 ||
      strcmp(path, "ui/index.html") == 0)
    path = APP_HTML_PATH;
  /* normalize accidental encoded-slash style or missing leading slash */
  if (strcmp(path, "ui/index.html") == 0)
    path = APP_HTML_PATH;
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
           "\"characterCount\":%zu,\"ingredientCount\":%zu}",
           WEBUI_VERSION, DISH_COUNT, CHARACTER_COUNT, INGREDIENT_COUNT);
  webui_return_string(e, buf);
}

static void get_dishes(webui_event_t *e) {
  /* rough size: names/types are small */
  size_t cap = 256 + DISH_COUNT * 160;
  char *buf = (char *)malloc(cap);
  if (!buf) {
    webui_return_string(e, "[]");
    return;
  }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < DISH_COUNT && off + 200 < cap; i++) {
    char name[128];
    char type[128];
    json_escape(DISHES[i].name ? DISHES[i].name : "", name, sizeof(name));
    json_escape(DISHES[i].type ? DISHES[i].type : "", type, sizeof(type));
    off += snprintf(buf + off, cap - off,
                    "%s{\"name\":\"%s\",\"type\":\"%s\",\"priceL1\":%.2f,"
                    "\"priceL2\":%.2f,\"ingredients\":%zu}",
                    i ? "," : "", name, type, DISHES[i].price_l1,
                    DISHES[i].price_l2, DISHES[i].ingredient_count);
  }
  if (off + 2 < cap)
    off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

static void get_characters(webui_event_t *e) {
  size_t cap = 256 + CHARACTER_COUNT * 512;
  char *buf = (char *)malloc(cap);
  if (!buf) {
    webui_return_string(e, "[]");
    return;
  }
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "[");
  for (size_t i = 0; i < CHARACTER_COUNT && off + 300 < cap; i++) {
    char name[128];
    json_escape(CHARACTERS[i].name ? CHARACTERS[i].name : "", name, sizeof(name));
    off += snprintf(buf + off, cap - off, "%s{\"name\":\"%s\",\"skillCount\":%zu,\"skills\":[",
                    i ? "," : "", name, CHARACTERS[i].skill_count);
    for (size_t s = 0; s < CHARACTERS[i].skill_count && off + 160 < cap; s++) {
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
      off += snprintf(buf + off, cap - off,
                      "%s{\"level\":%d,\"value\":%.4f,\"req\":%d,\"type\":\"%s\",\"tag\":\"%s\"}",
                      s ? "," : "", sk->level, sk->value, sk->requirement, stype, stag);
    }
    if (off + 4 < cap)
      off += snprintf(buf + off, cap - off, "]}");
  }
  if (off + 2 < cap)
    off += snprintf(buf + off, cap - off, "]");
  webui_return_string(e, buf);
  free(buf);
}

int main(int argc, char **argv) {
  if (argc > 0 && argv[0] && argv[0][0] != '\0') {
    static char root_buf[1024];
    snprintf(root_buf, sizeof(root_buf), "%s", argv[0]);
    char *slash = strrchr(root_buf, '/');
    if (slash)
      *slash = '\0';
    app_root = root_buf;
  }

  size_t win = webui_new_window();
  webui_set_file_handler(win, file_handler);
  webui_bind(win, "getAppInfo", get_app_info);
  webui_bind(win, "getDishes", get_dishes);
  webui_bind(win, "getCharacters", get_characters);

  const char *url = webui_start_server(win, "/");
  if (!url) {
    fprintf(stderr, "Failed to start WebUI server\n");
    return 1;
  }

  printf("NTE Cafe server running at %s\n", url);
  fflush(stdout);

  webui_wait();
  webui_clean();
  return 0;
}
