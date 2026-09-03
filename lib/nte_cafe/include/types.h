#ifndef NTE_CAFE_TYPES_H
#define NTE_CAFE_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_DISH_SLOTS 5
#define MAX_CHARACTER_SLOTS 10

#define MAX_DISH_POOL 32
#define MAX_CHARACTER_POOL 64
#define MAX_SKILLS_PER_CHARACTER 5
#define MAX_LOG_ENTRIES 64
#define MAX_INGREDIENTS_PER_DISH 4
#define MAX_NAME_LEN 64

typedef enum {
    SKILL_PRICE_FLAT,
    SKILL_TRAFFIC_FLAT,
    SKILL_PRICE_MULTIPLY,
    SKILL_TRAFFIC_MULTIPLY
} SkillType;

typedef enum {
    TAG_NONE,
    TAG_ANY,
    TAG_BEVERAGES,
    TAG_DESSERTS,
    TAG_MAIN_DISHES
} SkillTag;

typedef enum {
    TREND_NONE,
    TREND_TAG,
    TREND_INGREDIENT,
    TREND_CATEGORY
} TrendType;

typedef struct {
    TrendType type;
    const char *name;
} Trend;

typedef struct {
    Trend trends[MAX_DISH_POOL];
    size_t count;
} TrendOptions;

typedef struct {
    const char *name;
    const char *category;
} Ingredient;

typedef struct {
    int level;
    double value;
    int requirement;
    SkillType type;
    SkillTag tag;
} Skill;

typedef struct {
    const char *name;
    const char *type;
    double price_l1;
    double price_l2;
    const int *ingredients;
    size_t ingredient_count;
} Dish;

typedef struct {
    const char *name;
    Skill skills[MAX_SKILLS_PER_CHARACTER];
    size_t skill_count;
} Character;

typedef struct {
    bool owned;
    int level;
} DishState;

typedef struct {
    bool owned;
    int level;
} CharacterState;

typedef struct {
    int cafes_owned;
    SkillTag trend_tag;
    const char *trend_name;
    double trend_bonus;
    double popularity_bonus;
    bool hotori_cat_decor;
} CafeSettings;

typedef struct {
    const Dish *dish;
    double base_price;
} CandidateDish;

typedef struct {
    const Character *character;
    int level;
    Skill active_skills[MAX_SKILLS_PER_CHARACTER];
    size_t active_skill_count;
} CandidateCharacter;

typedef struct {
    int type_counts[4];
    int tag_counts[5];
    int max_type_count;
    double base_price_sum;
    size_t dish_count;
} MenuStats;

typedef struct {
    const CandidateDish *dishes;
    size_t dish_count;
    const CandidateCharacter *characters;
    size_t character_count;
} Candidate;

typedef struct {
    char text[128];
    double value;
} BuffLogEntry;

typedef struct {
    BuffLogEntry entries[MAX_LOG_ENTRIES];
    size_t count;
} BuffLog;

typedef struct {
    const Dish *dishes[MAX_DISH_SLOTS];
    size_t dish_count;
    const Character *characters[MAX_CHARACTER_SLOTS];
    size_t character_count;
    double traffic;
    double price_buff;
    double price_multiplier;
    double income;
    BuffLog log;
} OptimizationResult;

typedef struct {
    const Dish *dishes;
    size_t dish_count;
    const Character *characters;
    size_t character_count;
    const DishState *dish_state;
    const CharacterState *character_state;
    CafeSettings settings;
} CafeContext;

#endif
