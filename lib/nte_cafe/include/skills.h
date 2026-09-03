#ifndef NTE_CAFE_SKILLS_H
#define NTE_CAFE_SKILLS_H

#include "types.h"

const char *skillTypeName(SkillType type);

size_t collect_active_skills(
    const Character *character,
    int character_level,
    Skill *out,
    size_t capacity
);

void calculate_menu_stats(
    const CandidateDish *dishes,
    size_t dish_count,
    MenuStats *stats
);

int skill_tag_count(const Skill *skill, const MenuStats *stats);
bool skill_activates(const Skill *skill, const MenuStats *stats);

#endif
