#include "cli_output.h"
#include "data.h"
#include "gamedata.h"
#include "optimizer.h"
#include "popularity.h"
#include "trend.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  DishState dish_state[MAX_DISH_POOL];
  CharacterState character_state[MAX_CHARACTER_POOL];
  int cafes_owned;
  int cafe_popularity[MAX_CAFES];
  const char *trend_name;
  bool decor;
  const char *data_path;
} CliOptions;

static const char *prog = "nte-cafe-cli";

static void print_help(void) {
  printf("%s\n", "NTE Cafe Optimizer CLI");
  printf("Game data: version %s, %zu dishes, %zu characters, %zu ingredients\n\n",
         gamedata_game_version(), DISH_COUNT, CHARACTER_COUNT,
         INGREDIENT_COUNT);

  printf("Usage:\n  %s [options]\n\n", prog);
  printf("Roster (valid IDs come from data/gamedata.json):\n");
  printf("  --char <id> <level>   own a character, level %d..%d\n",
         gamedata_character_min_level(), gamedata_character_max_level());
  printf("                        e.g. --char nanally 5\n");
  printf("  --chars <level>       own every character at that level\n");
  printf("  --dish <id> <level>   own a dish, level %d..%d\n",
         gamedata_dish_min_level(), gamedata_dish_max_level());
  printf("                        e.g. --dish coconut_latte 2\n");
  printf("  --dishes <level>      own every dish at that level\n\n");
  printf("Cafes and trend:\n");
  printf("  --cafes <1..%d>       number of cafes used for optimization\n",
         MAX_CAFES);
  printf("  --cafe <n> <level>    popularity level for cafe n (1-based)\n");
  printf("  --trend <name>        raw trend name, e.g. \"None\" or \"Coffee Beans\"\n");
  printf("  --decor               enable Hotori's Cat Decor\n");
  printf("  --no-decor            disable Hotori's Cat Decor\n\n");
  printf("Data and info:\n");
  printf("  --data <path>         load a JSON dataset instead of the embedded one\n");
  printf("  --list                list every valid ID, level, cafe and trend\n");
  printf("  -h, --help            show this help\n\n");
  printf("Example:\n");
  printf("  %s --char nanally 5 --dish coconut_latte 2 --cafes 5 \\\n", prog);
  printf("      --trend \"Coffee Beans\" --decor\n");
}

static void list_ids(void) {
  printf("Characters (level %d..%d):\n", gamedata_character_min_level(),
         gamedata_character_max_level());
  for (size_t i = 0; i < CHARACTER_COUNT; ++i)
    printf("  %s\n", CHARACTERS[i].id ? CHARACTERS[i].id : "");

  printf("\nDishes (level %d..%d):\n", gamedata_dish_min_level(),
         gamedata_dish_max_level());
  for (size_t i = 0; i < DISH_COUNT; ++i)
    printf("  %s\n", DISHES[i].id ? DISHES[i].id : "");

  printf("\nCafes (1..%d):\n", MAX_CAFES);
  for (int i = 0; i < MAX_CAFES; ++i)
    printf("  %d: popularity 0..%d\n", i + 1, popularity_max_level(i));

  printf("\nTrends:\n");
  TrendOptions options = get_trend_options();
  for (size_t i = 0; i < options.count; ++i)
    printf("  %s\n", options.trends[i].name);
}

static int error_arg(const char *arg, const char *detail) {
  fprintf(stderr, "Error: %s: %s\nRun '%s --help' for valid inputs.\n", arg,
          detail, prog);
  return 2;
}

static bool parse_int(const char *text, int *out) {
  if (!text || !*text)
    return false;
  char *end = NULL;
  long value = strtol(text, &end, 10);
  if (end == text || *end != '\0')
    return false;
  if (value < -100000 || value > 100000)
    return false;
  *out = (int)value;
  return true;
}

static int find_character(const char *id) {
  for (size_t i = 0; i < CHARACTER_COUNT; ++i) {
    if (CHARACTERS[i].id && strcmp(CHARACTERS[i].id, id) == 0)
      return (int)i;
  }
  return -1;
}

static int find_dish(const char *id) {
  for (size_t i = 0; i < DISH_COUNT; ++i) {
    if (DISHES[i].id && strcmp(DISHES[i].id, id) == 0)
      return (int)i;
  }
  return -1;
}

static void print_unknown(const char *kind, const char *id) {
  fprintf(stderr, "Unknown %s '%s'. Valid IDs:\n", kind, id);
  if (strcmp(kind, "character") == 0) {
    for (size_t i = 0; i < CHARACTER_COUNT; ++i)
      fprintf(stderr, "  %s\n", CHARACTERS[i].id);
  } else {
    for (size_t i = 0; i < DISH_COUNT; ++i)
      fprintf(stderr, "  %s\n", DISHES[i].id);
  }
}

static void find_data_path(int argc, char **argv, const char **out) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
      *out = argv[i + 1];
      return;
    }
  }
  const char *env = getenv("NTE_CAFE_GAMEDATA");
  if (env && *env)
    *out = env;
}

static bool load_data(const char *path) {
  if (path && gamedata_init_file(path))
    return true;
  return gamedata_init_embedded();
}

static int parse_args(int argc, char **argv, CliOptions *opts) {
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];

    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      print_help();
      return 1;
    }
    if (strcmp(arg, "--list") == 0) {
      list_ids();
      return 1;
    }
    if (strcmp(arg, "--data") == 0) {
      if (++i >= argc)
        return error_arg(arg, "missing <path>");
      continue;
    }
    if (strcmp(arg, "--char") == 0 || strcmp(arg, "--chars") == 0) {
      bool all = strcmp(arg, "--chars") == 0;
      if (i + 1 >= argc)
        return error_arg(arg, all ? "missing <level>" : "missing <id> <level>");
      const char *id = argv[++i];
      const char *level_text = all ? id : argv[++i];
      int level = 0;
      if (!parse_int(level_text, &level) ||
          level < gamedata_character_min_level() ||
          level > gamedata_character_max_level())
        return error_arg(arg, "level out of range");
      if (all) {
        for (size_t c = 0; c < CHARACTER_COUNT; ++c) {
          opts->character_state[c].owned = true;
          opts->character_state[c].level = level;
        }
      } else {
        int index = find_character(id);
        if (index < 0) {
          print_unknown("character", id);
          return 2;
        }
        opts->character_state[index].owned = true;
        opts->character_state[index].level = level;
      }
      continue;
    }
    if (strcmp(arg, "--dish") == 0 || strcmp(arg, "--dishes") == 0) {
      bool all = strcmp(arg, "--dishes") == 0;
      if (i + 1 >= argc)
        return error_arg(arg, all ? "missing <level>" : "missing <id> <level>");
      const char *id = argv[++i];
      const char *level_text = all ? id : argv[++i];
      int level = 0;
      if (!parse_int(level_text, &level) ||
          level < gamedata_dish_min_level() ||
          level > gamedata_dish_max_level())
        return error_arg(arg, "level out of range");
      if (all) {
        for (size_t d = 0; d < DISH_COUNT; ++d) {
          opts->dish_state[d].owned = true;
          opts->dish_state[d].level = level;
        }
      } else {
        int index = find_dish(id);
        if (index < 0) {
          print_unknown("dish", id);
          return 2;
        }
        opts->dish_state[index].owned = true;
        opts->dish_state[index].level = level;
      }
      continue;
    }
    if (strcmp(arg, "--cafes") == 0) {
      if (++i >= argc)
        return error_arg(arg, "missing <1..5>");
      int cafes = 0;
      if (!parse_int(argv[i], &cafes) || cafes < 1 || cafes > MAX_CAFES)
        return error_arg(arg, "expected 1..5");
      opts->cafes_owned = cafes;
      continue;
    }
    if (strcmp(arg, "--cafe") == 0) {
      if (i + 2 >= argc)
        return error_arg(arg, "missing <n> <level>");
      int cafe = 0, level = 0;
      if (!parse_int(argv[++i], &cafe) || cafe < 1 || cafe > MAX_CAFES)
        return error_arg(arg, "cafe number out of range");
      if (!parse_int(argv[++i], &level) || level < 0 ||
          level > popularity_max_level(cafe - 1))
        return error_arg(arg, "popularity level out of range");
      opts->cafe_popularity[cafe - 1] = level;
      continue;
    }
    if (strcmp(arg, "--trend") == 0) {
      if (++i >= argc)
        return error_arg(arg, "missing <name>");
      opts->trend_name = argv[i];
      TrendOptions options = get_trend_options();
      bool found = false;
      for (size_t t = 0; t < options.count; ++t) {
        if (strcmp(options.trends[t].name, opts->trend_name) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "Unknown trend '%s'. Valid trends:\n", opts->trend_name);
        for (size_t t = 0; t < options.count; ++t)
          fprintf(stderr, "  %s\n", options.trends[t].name);
        return 2;
      }
      continue;
    }
    if (strcmp(arg, "--decor") == 0) {
      opts->decor = true;
      continue;
    }
    if (strcmp(arg, "--no-decor") == 0) {
      opts->decor = false;
      continue;
    }
    return error_arg(arg, "unknown option");
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 0 && argv[0] && *argv[0]) {
    const char *slash = strrchr(argv[0], '/');
    if (slash)
      prog = slash + 1;
  }

  if (argc == 1) {
    print_help();
    return 0;
  }

  const char *data_path = NULL;
  find_data_path(argc, argv, &data_path);
  if (!load_data(data_path)) {
    fprintf(stderr, "Failed to load game data%s%s\n",
            data_path ? " from " : "", data_path ? data_path : "");
    return 1;
  }

  CliOptions opts;
  memset(&opts, 0, sizeof(opts));
  for (size_t i = 0; i < DISH_COUNT; ++i)
    opts.dish_state[i].level = gamedata_dish_min_level();
  for (size_t i = 0; i < CHARACTER_COUNT; ++i)
    opts.character_state[i].level = gamedata_character_min_level();
  opts.cafes_owned = 1;
  opts.trend_name = "None";
  opts.decor = false;

  int parsed = parse_args(argc, argv, &opts);
  if (parsed != 0)
    return parsed == 1 ? 0 : 2;

  TrendOptions options = get_trend_options();
  Trend selected = {TREND_NONE, "None"};
  for (size_t i = 0; i < options.count; ++i) {
    if (strcmp(options.trends[i].name, opts.trend_name) == 0) {
      selected = options.trends[i];
      break;
    }
  }

  CafeSettings settings;
  memset(&settings, 0, sizeof(settings));
  settings.cafes_owned = opts.cafes_owned;
  settings.trend_tag = get_trend_tag(selected);
  settings.trend_name = selected.name;
  settings.trend_bonus = get_trend_bonus(selected.type);
  settings.hotori_cat_decor = opts.decor;
  for (int i = 0; i < MAX_CAFES; ++i)
    settings.cafe_popularity[i] = opts.cafe_popularity[i];
  settings.popularity_bonus = popularity_total_bonus(&settings);

  CafeContext ctx = {DISHES,           DISH_COUNT,      CHARACTERS,
                     CHARACTER_COUNT, opts.dish_state, opts.character_state,
                     settings};
  OptimizationResult result;
  if (!optimize_cafe(&ctx, &result)) {
    fprintf(stderr, "No valid optimization result. Check your roster inputs.\n");
    return 1;
  }

  print_optimization_result(&ctx, &result);
  return 0;
}
