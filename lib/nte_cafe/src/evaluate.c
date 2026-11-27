#include "evaluate.h"
#include "popularity.h"
#include "skills.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

static void log_add(BuffLog *log, const char *text, double value) {
  if (!log || log->count >= MAX_LOG_ENTRIES)
    return;
  snprintf(log->entries[log->count].text, sizeof(log->entries[log->count].text),
           "%s", text);

  log->entries[log->count].value = value;
  ++log->count;
}

/* Hot path: compute income without allocating/filling OptimizationResult or logs. */
double evaluate_income_fast(const MenuStats *menu,
                            const CandidateCharacter *characters,
                            size_t character_count,
                            const CafeSettings *settings) {
  double traffic = 2400.0;
  double price_buff = 0.0;

  for (size_t c = 0; c < character_count; ++c) {
    const CandidateCharacter *ch = &characters[c];
    for (size_t s = 0; s < ch->active_skill_count; ++s) {
      const Skill *skill = &ch->active_skills[s];
      if (!skill_activates(skill, menu))
        continue;
      if (skill->type == SKILL_TRAFFIC_FLAT)
        traffic += skill->value;
      else if (skill->type == SKILL_PRICE_FLAT)
        price_buff += skill->value;
    }
  }

  const double base_traffic = traffic;
  double multiplier_bonus = 0.0;
  double price_multiplier = 1.0;
  for (size_t c = 0; c < character_count; ++c) {
    const CandidateCharacter *ch = &characters[c];
    for (size_t s = 0; s < ch->active_skill_count; ++s) {
      const Skill *skill = &ch->active_skills[s];
      if (!skill_activates(skill, menu))
        continue;
      if (skill->type == SKILL_TRAFFIC_MULTIPLY)
        multiplier_bonus += base_traffic * skill->value;
      else if (skill->type == SKILL_PRICE_MULTIPLY)
        price_multiplier += skill->value;
    }
  }
  if (settings && settings->hotori_cat_decor)
    multiplier_bonus += base_traffic * 0.01;
  traffic = base_traffic + multiplier_bonus;

  double adjusted_price_sum = menu->base_price_sum - menu->trend_bonus_sum +
                              ((double)menu->dish_count * price_buff);
  double income = (adjusted_price_sum * price_multiplier +
                   menu->trend_bonus_sum) *
                  (traffic / 100.0);
  return income * (1.0 + popularity_total_bonus(settings));
}

static void apply_flat_skills(const CandidateCharacter *characters,
                              size_t character_count, const MenuStats *menu,
                              double *traffic, double *price_buff,
                              BuffLog *log) {
  for (size_t c = 0; c < character_count; ++c) {
    const CandidateCharacter *ch = &characters[c];

    for (size_t s = 0; s < ch->active_skill_count; ++s) {
      const Skill *skill = &ch->active_skills[s];
      if (!skill_activates(skill, menu))
        continue;

      if (skill->type == SKILL_TRAFFIC_FLAT) {
        *traffic += skill->value;
        log_add(log, "Traffic", skill->value);
      } else if (skill->type == SKILL_PRICE_FLAT) {
        *price_buff += skill->value;
        log_add(log, "Price", skill->value);
      }
    }
  }
}

static void apply_traffic_multipliers(const CandidateCharacter *characters,
                                      size_t character_count,
                                      const MenuStats *menu,
                                      const CafeSettings *settings,
                                      double *traffic, BuffLog *log) {
  const double base_traffic = *traffic;
  double multiplier_bonus = 0.0;

  for (size_t c = 0; c < character_count; ++c) {
    const CandidateCharacter *ch = &characters[c];

    for (size_t s = 0; s < ch->active_skill_count; ++s) {
      const Skill *skill = &ch->active_skills[s];

      if (skill->type != SKILL_TRAFFIC_MULTIPLY)
        continue;

      if (!skill_activates(skill, menu))
        continue;

      const double bonus = base_traffic * skill->value;

      multiplier_bonus += bonus;

      log_add(log, "Traffic", bonus);
    }
  }

  if (settings && settings->hotori_cat_decor) {
    const double bonus = base_traffic * 0.01;

    multiplier_bonus += bonus;

    log_add(log, "Hotori Cat Decor", bonus);
  }

  *traffic = base_traffic + multiplier_bonus;
}

static double apply_price_multipliers(const CandidateCharacter *characters,
                                      size_t character_count,
                                      const MenuStats *menu, double initial,
                                      BuffLog *log) {
  double multiplier = initial;

  for (size_t c = 0; c < character_count; ++c) {
    const CandidateCharacter *ch = &characters[c];

    for (size_t s = 0; s < ch->active_skill_count; ++s) {
      const Skill *skill = &ch->active_skills[s];
      if (skill->type != SKILL_PRICE_MULTIPLY)
        continue;
      if (!skill_activates(skill, menu))
        continue;

      multiplier += skill->value;
      log_add(log, "Price", skill->value);
    }
  }

  return multiplier;
}

static double calculate_income(const MenuStats *menu, double traffic,
                               double price_buff, double price_multiplier) {
  /*
   * The game adds the trend bonus AFTER character price multipliers:
   *   sum(((basePrice + priceBuff) * priceMultiplier + trendBonus)
   *       * traffic / 100)
   * base_price_sum still contains the bonuses, so remove them first.
   */
  double adjusted_price_sum = menu->base_price_sum - menu->trend_bonus_sum +
                              ((double)menu->dish_count * price_buff);

  return (adjusted_price_sum * price_multiplier + menu->trend_bonus_sum) *
         (traffic / 100.0);
}

OptimizationResult evaluate_candidate(const CandidateDish *dishes,
                                      size_t dish_count, MenuStats menu,
                                      const CandidateCharacter *characters,
                                      size_t character_count,
                                      const CafeSettings *settings) {
  OptimizationResult result;
  memset(&result, 0, sizeof(result));

  double traffic = 2400.0;
  double price_buff = 0.0;
  double price_multiplier = 1.0;

  apply_flat_skills(characters, character_count, &menu, &traffic, &price_buff,
                    &result.log);

  apply_traffic_multipliers(characters, character_count, &menu, settings,
                            &traffic, &result.log);

  price_multiplier = apply_price_multipliers(characters, character_count, &menu,
                                             price_multiplier, &result.log);

  result.dish_count = dish_count;
  result.character_count = character_count;

  for (size_t i = 0; i < dish_count && i < MAX_DISH_SLOTS; ++i) {
    result.dishes[i] = dishes[i].dish;
  }

  for (size_t i = 0; i < character_count && i < MAX_CHARACTER_SLOTS; ++i) {
    result.characters[i] = characters[i].character;
  }

  result.traffic = traffic;
  result.price_buff = price_buff;
  result.price_multiplier = price_multiplier;
  result.popularity_bonus = popularity_total_bonus(settings);
  result.income = calculate_income(&menu, traffic, price_buff,
                                   price_multiplier) *
                  (1.0 + result.popularity_bonus);

  return result;
}
