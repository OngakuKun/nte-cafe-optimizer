#include "optimizer.h"
#include "combinations.h"
#include "evaluate.h"
#include "skills.h"
#include "trend.h"

#include <stddef.h>
#include <stdio.h>
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

  MenuStats selected_menu;
  size_t character_combinations_evaluated;

  OptimizationResult *best;
  bool found;
} SearchState;

/*
 * Evaluate every character combination for the currently selected
 * dish combination.
 */
static bool evaluate_chars(const size_t *char_indices, size_t char_count,
                           void *userdata) {
  SearchState *s = userdata;

  CandidateCharacter selected_chars[MAX_CHARACTER_SLOTS];

  for (size_t i = 0; i < char_count; ++i) {
    selected_chars[i] = s->char_pool[char_indices[i]];
  }

  OptimizationResult candidate =
      evaluate_candidate(s->selected_dishes, s->dish_slots, s->selected_menu,
                         selected_chars, char_count, &s->ctx->settings);

  s->character_combinations_evaluated++;

  if (!s->found || candidate.income > s->best->income) {
    *s->best = candidate;
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

  /*
   * Copy the current dish combination into the search state.
   */
  for (size_t i = 0; i < dish_count; ++i) {
    s->selected_dishes[i] = s->dish_pool[dish_indices[i]];
  }

  calculate_menu_stats(s->selected_dishes, s->dish_slots, &s->selected_menu);

  /*
   * For this menu, try every possible character team.
   */
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
  SearchState state = {.ctx = ctx,

                       .dish_pool = dish_pool,
                       .dish_pool_count = dish_pool_count,

                       .char_pool = char_pool,
                       .char_pool_count = char_pool_count,

                       .dish_slots = dish_slots,
                       .char_slots = char_slots,

                       .best = result,
                       .found = false};

  memset(result, 0, sizeof(*result));

  /*
   * Main exhaustive search:
   *
   *   every possible menu
   *       ×
   *   every possible character team
   */
  enumerate_combinations(dish_pool_count, dish_slots, evaluate_dishes, &state);

  return state.found;
}
