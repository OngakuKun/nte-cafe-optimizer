#include "skills.h"

#include "stdio.h"
#include <string.h>

const char *skillTypeName(SkillType type) {
  if (type == SKILL_PRICE_FLAT)
    return " price";
  else if (type == SKILL_TRAFFIC_FLAT)
    return " traffic";
  else if (type == SKILL_PRICE_MULTIPLY)
    return "% price";
  else if (type == SKILL_TRAFFIC_MULTIPLY)
    return "% traffic";
  else
    return "inactive";
}

size_t collect_active_skills(const Character *character, int character_level,
                             Skill *out, size_t capacity) {
  if (!character || !out)
    return 0;

  size_t count = 0;
  for (size_t i = 0; i < character->skill_count; ++i) {
    const Skill *skill = &character->skills[i];

    /* JS behavior: skills are active when their level requirement is met
       and their value is positive. */
    if (skill->requirement <= character_level && skill->value > 0.0) {
      if (count < capacity)
        out[count] = *skill;
      ++count;
    }
  }

  // printf("%s loaded with %u Skills\n", character->name, count);
  return count < capacity ? count : capacity;
}

void calculate_menu_stats(const CandidateDish *dishes, size_t dish_count,
                          MenuStats *stats) {
  memset(stats, 0, sizeof(*stats));
  stats->dish_count = dish_count;

  for (size_t i = 0; i < dish_count; ++i) {
    const Dish *dish = dishes[i].dish;
    stats->base_price_sum += dishes[i].base_price;

    if (!dish->type)
      continue;

    if (strcmp(dish->type, "Beverages") == 0)
      stats->type_counts[0]++;
    else if (strcmp(dish->type, "Desserts") == 0)
      stats->type_counts[1]++;
    else if (strcmp(dish->type, "Main Dishes") == 0)
      stats->type_counts[2]++;
    else
      stats->type_counts[3]++;
  }

  for (int i = 0; i < 4; ++i)
    if (stats->type_counts[i] > stats->max_type_count)
      stats->max_type_count = stats->type_counts[i];
}

int skill_tag_count(const Skill *skill, const MenuStats *stats) {
  if (!skill || !stats)
    return 0;

  switch (skill->tag) {
  case TAG_ANY:
    return stats->max_type_count;
  case TAG_BEVERAGES:
    return stats->type_counts[0];
  case TAG_DESSERTS:
    return stats->type_counts[1];
  case TAG_MAIN_DISHES:
    return stats->type_counts[2];
  case TAG_NONE:
  default:
    return 0;
  }
}

bool skill_activates(const Skill *skill, const MenuStats *stats) {
  if (!skill || !stats)
    return false;

  if (skill->tag == TAG_NONE)
    return true;

  int count = skill_tag_count(skill, stats);

  return count >= skill->requirement;
}
