#include "optimizer.h"
#include "combinations.h"
#include "evaluate.h"
#include "skills.h"
#include "trend.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const CafeContext *ctx;

  CandidateDish *dish_pool;
  size_t dish_pool_count;

  CandidateCharacter *char_pool;
  size_t char_pool_count;

  size_t dish_slots;
  size_t char_slots;

  /* The dish combination currently being evaluated. */
  CandidateDish selected_dishes[MAX_DISH_SLOTS];
  size_t selected_dish_pool_indices[MAX_DISH_SLOTS];
  MenuStats selected_menu;

  /* Best found during fast scan. */
  double best_income;
  size_t best_dish_pool_indices[MAX_DISH_SLOTS];
  size_t best_char_pool_indices[MAX_CHARACTER_SLOTS];
  bool found;
} SearchState;

/*
 * Hot-path: evaluate every character combination for current dish menu.
 * Only tracks best income and indices; full result built once at end.
 */
static bool evaluate_chars(const size_t *char_indices, size_t char_count,
                           void *userdata) {
  SearchState *s = userdata;
  (void)char_count;

  CandidateCharacter selected_chars[MAX_CHARACTER_SLOTS];
  for (size_t i = 0; i < s->char_slots; ++i)
    selected_chars[i] = s->char_pool[char_indices[i]];

  double income = evaluate_income_fast(&s->selected_menu, selected_chars,
                                       s->char_slots, &s->ctx->settings);

  if (!s->found || income > s->best_income) {
    s->best_income = income;
    for (size_t i = 0; i < s->dish_slots; ++i)
      s->best_dish_pool_indices[i] = s->selected_dish_pool_indices[i];
    for (size_t i = 0; i < s->char_slots; ++i)
      s->best_char_pool_indices[i] = char_indices[i];
    s->found = true;
  }

  return true;
}

/*
 * Evaluate one dish combination by enumerating every possible
 * character combination.
 */
static bool evaluate_dishes(const size_t *dish_indices, size_t dish_count,
                            void *userdata) {
  SearchState *s = userdata;
  (void)dish_count;

  for (size_t i = 0; i < s->dish_slots; ++i) {
    s->selected_dish_pool_indices[i] = dish_indices[i];
    s->selected_dishes[i] = s->dish_pool[dish_indices[i]];
  }

  calculate_menu_stats(s->selected_dishes, s->dish_slots, &s->selected_menu);

  return enumerate_combinations(s->char_pool_count, s->char_slots,
                                evaluate_chars, s);
}

bool optimize_cafe(const CafeContext *ctx, OptimizationResult *result) {
  if (!ctx || !result)
    return false;

  /*
   * Determine how many dishes and characters the café can use.
   *
   * 1 café  -> 1 dish + 2 characters
   * 5 cafés -> 5 dishes + 10 characters
   */
  size_t dish_slots = (size_t)ctx->settings.cafes_owned;

  size_t char_slots = (size_t)ctx->settings.cafes_owned * 2;

  /*
   * Prevent the selected combinations from exceeding
   * the fixed-size arrays.
   */
  if (dish_slots > MAX_DISH_SLOTS)
    dish_slots = MAX_DISH_SLOTS;

  if (char_slots > MAX_CHARACTER_SLOTS)
    char_slots = MAX_CHARACTER_SLOTS;

  /*
   * We need at least enough owned dishes to fill the café.
   */
  if (dish_slots == 0 || ctx->dish_count < dish_slots) {
    return false;
  }

  /*
   * Build the pool of owned dishes.
   */
  CandidateDish dish_pool[MAX_DISH_POOL];
  size_t dish_pool_count = 0;

  for (size_t i = 0; i < ctx->dish_count; ++i) {

    if (!ctx->dish_state[i].owned)
      continue;

    if (dish_pool_count >= MAX_DISH_POOL)
      return false;

    dish_pool[dish_pool_count].dish = &ctx->dishes[i];

    dish_pool[dish_pool_count].base_price = dish_base_price(
        &ctx->dishes[i], ctx->dish_state[i].level, &ctx->settings);

    dish_pool[dish_pool_count].trend_bonus =
        dish_trend_bonus(&ctx->dishes[i], &ctx->settings);

    ++dish_pool_count;
  }

  /*
   * Build the pool of owned characters.
   */
  CandidateCharacter char_pool[MAX_CHARACTER_POOL];
  size_t char_pool_count = 0;

  for (size_t i = 0; i < ctx->character_count; ++i) {

    if (!ctx->character_state[i].owned)
      continue;

    if (char_pool_count >= MAX_CHARACTER_POOL)
      return false;

    char_pool[char_pool_count].character = &ctx->characters[i];

    char_pool[char_pool_count].level = ctx->character_state[i].level;

    char_pool[char_pool_count].active_skill_count = collect_active_skills(
        &ctx->characters[i], ctx->character_state[i].level,
        char_pool[char_pool_count].active_skills, MAX_SKILLS_PER_CHARACTER);

    ++char_pool_count;
  }

  /*
   * We cannot optimize without characters.
   */
  if (char_pool_count == 0)
    return false;

  /*
   * If the player owns fewer characters than the café
   * can accommodate, use all available characters.
   */
  if (char_slots > char_pool_count)
    char_slots = char_pool_count;

  /*
   * Initialize the search state.
   */
  SearchState state = {0};
  state.ctx = ctx;
  state.dish_pool = dish_pool;
  state.dish_pool_count = dish_pool_count;
  state.char_pool = char_pool;
  state.char_pool_count = char_pool_count;
  state.dish_slots = dish_slots;
  state.char_slots = char_slots;
  state.found = false;
  state.best_income = -1.0;

  memset(result, 0, sizeof(*result));

  /*
   * Skills depend on dish type counts. For each type-count distribution,
   * the best menu is the top-priced dishes of those types. This collapses the
   * dish space dramatically vs brute-forcing all dish combinations.
   */
  typedef struct { int count; } TypePick;
  /* Group dish pool indices by dish type (3 known types). */
  size_t type_groups[4][MAX_DISH_POOL];
  size_t type_group_counts[4] = {0,0,0,0};
  for (size_t i = 0; i < dish_pool_count; ++i) {
    int t = 3;
    const char *dt = dish_pool[i].dish->type;
    if (dt && strcmp(dt, "Beverages") == 0) t = 0;
    else if (dt && strcmp(dt, "Desserts") == 0) t = 1;
    else if (dt && strcmp(dt, "Main Dishes") == 0) t = 2;
    type_groups[t][type_group_counts[t]++] = i;
  }
  /* Sort each group by base_price desc. */
  for (int t = 0; t < 4; ++t) {
    for (size_t a = 0; a < type_group_counts[t]; ++a) {
      for (size_t b = a + 1; b < type_group_counts[t]; ++b) {
        size_t ia = type_groups[t][a], ib = type_groups[t][b];
        if (dish_pool[ib].base_price > dish_pool[ia].base_price) {
          type_groups[t][a] = ib; type_groups[t][b] = ia;
        }
      }
    }
  }

  /* Enumerate type-count distributions summing to dish_slots. */
  size_t c0min = 0, c0max = dish_slots < type_group_counts[0] ? dish_slots : type_group_counts[0];
  for (size_t c0 = 0; c0 <= c0max; ++c0) {
    size_t rem1 = dish_slots - c0;
    size_t cap1 = rem1 < type_group_counts[1] ? rem1 : type_group_counts[1];
    size_t min1 = rem1 > (type_group_counts[2]+type_group_counts[3]) ? rem1 - (type_group_counts[2]+type_group_counts[3]) : 0;
    for (size_t c1 = min1; c1 <= cap1; ++c1) {
      size_t rem2 = rem1 - c1;
      size_t cap2 = rem2 < type_group_counts[2] ? rem2 : type_group_counts[2];
      size_t min2 = rem2 > type_group_counts[3] ? rem2 - type_group_counts[3] : 0;
      for (size_t c2 = min2; c2 <= cap2; ++c2) {
        size_t c3 = rem2 - c2;
        if (c3 > type_group_counts[3]) continue;
        /* build dish combination from top items of each type */
        size_t idxs[MAX_DISH_SLOTS];
        size_t nidx = 0;
        size_t pick[4] = {c0,c1,c2,c3};
        for (int t = 0; t < 4; ++t)
          for (size_t k = 0; k < pick[t]; ++k)
            idxs[nidx++] = type_groups[t][k];
        if (nidx != dish_slots) continue;
        /* Preserve original dish-pool order for stable output. */
        for (size_t a = 0; a < nidx; ++a)
          for (size_t b = a + 1; b < nidx; ++b)
            if (idxs[b] < idxs[a]) { size_t tmp = idxs[a]; idxs[a] = idxs[b]; idxs[b] = tmp; }
        evaluate_dishes(idxs, dish_slots, &state);
      }
    }
  }

  if (!state.found)
    return false;

  /* Rebuild full result once for the winning combination. */
  CandidateDish best_dishes[MAX_DISH_SLOTS];
  MenuStats best_menu;
  for (size_t i = 0; i < dish_slots; ++i)
    best_dishes[i] = dish_pool[state.best_dish_pool_indices[i]];
  calculate_menu_stats(best_dishes, dish_slots, &best_menu);

  CandidateCharacter best_chars[MAX_CHARACTER_SLOTS];
  for (size_t i = 0; i < char_slots; ++i)
    best_chars[i] = char_pool[state.best_char_pool_indices[i]];

  *result = evaluate_candidate(best_dishes, dish_slots, best_menu, best_chars,
                               char_slots, &ctx->settings);
  return true;
}
