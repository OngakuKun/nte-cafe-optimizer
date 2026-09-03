#include "data.h"

const Ingredient INGREDIENTS[] = {
    {"Coffee Beans", ""},  {"Coconut", "Fruit"},
    {"Cocoa Powder", ""},  {"Flour", ""},
    {"Apple", "Fruit"},    {"Eggs", ""},
    {"Lettuce", ""},       {"Fish", ""},
    {"Orange", "Fruit"},   {"Milk", ""},
    {"Matcha Powder", ""}, {"Whipping Cream", ""},
    {"Beef", ""},          {"Butter", ""},
    {"Red Beans", ""},     {"Strawberry", "Fruit"},
    {"Onion", ""},         {"Pork", ""},
    {"Salt", ""},          {"Cookies", ""},
    {"Cheese", ""},        {"Ham", ""},
    {"Sugar", ""},         {"Skibbles", ""},
    {"Tomato", ""}};

const size_t INGREDIENT_COUNT = sizeof(INGREDIENTS) / sizeof(INGREDIENTS[0]);

static const int DISH0_ING[] = {0, 1};
static const int DISH1_ING[] = {0, 2};
static const int DISH2_ING[] = {3, 4};
static const int DISH3_ING[] = {3, 4, 5};
static const int DISH4_ING[] = {3, 6, 7};
static const int DISH5_ING[] = {0, 9, 23, 8};
static const int DISH6_ING[] = {3, 9, 10};
static const int DISH7_ING[] = {0, 9, 11, 2};
static const int DISH8_ING[] = {3, 12, 6, 13};
static const int DISH9_ING[] = {3, 13, 14};
static const int DISH10_ING[] = {3, 15, 11};
static const int DISH11_ING[] = {0, 8, 4};
static const int DISH12_ING[] = {3, 16, 17, 18};
static const int DISH13_ING[] = {0, 9, 19, 1};
static const int DISH14_ING[] = {0, 9, 11, 10};
static const int DISH15_ING[] = {3, 19, 20, 0};
static const int DISH16_ING[] = {3, 6, 21, 20};
static const int DISH17_ING[] = {3, 11, 22, 2};

const Dish DISHES[] = {
    {"Coconut Latte", "Beverages", 2.4, 10.2, DISH0_ING, 2},

    {"Ice Mocha", "Beverages", 3.0, 10.6, DISH1_ING, 2},

    {"Apple Pie", "Desserts", 3.5, 11.0, DISH2_ING, 2},

    {"Tomato & Fried Egg", "Main Dishes", 4.0, 11.4, DISH3_ING, 3},

    {"Tuna Sandwich", "Main Dishes", 4.6, 11.8, DISH4_ING, 3},

    {"Amber Glamor", "Beverages", 5.1, 12.2, DISH5_ING, 4},

    {"Matacha Lava Mousse", "Desserts", 5.5, 12.6, DISH6_ING, 3},

    {"Snowy Latte", "Beverages", 6.0, 13.0, DISH7_ING, 4},

    {"Thick-Cut Fresh Beef Sandwich", "Main Dishes", 6.4, 13.5, DISH8_ING, 4},

    {"Red Bean & Butter Crossiant", "Main Dishes", 7.0, 14.0, DISH9_ING, 3},

    {"Strawberry Box Cake", "Desserts", 7.4, 14.5, DISH10_ING, 3},

    {"Orange Hot Coffee", "Beverages", 7.8, 15.0, DISH11_ING, 3},

    {"Pork Cutlet", "Main Dishes", 8.1, 15.5, DISH12_ING, 4},

    {"Coconut Macchiato", "Beverages", 8.6, 16.0, DISH13_ING, 4},

    {"Matcha Latte with Milk Cap", "Beverages", 9.0, 16.5, DISH14_ING, 4},

    {"Tiramisu", "Desserts", 9.3, 17.0, DISH15_ING, 4},

    {"Ham & Cheese", "Main Dishes", 9.7, 17.5, DISH16_ING, 4},

    {"Caramel & Cocoa Mille-Feuille", "Desserts", 10.0, 18.0, DISH17_ING, 4}};

const size_t DISH_COUNT = sizeof(DISHES) / sizeof(DISHES[0]);

/* =========================================================
 * CHARACTERS
 * ========================================================= */

const Character CHARACTERS[] = {
    /* name, skills, skillCount */

    {"Nanally",
     {{1, 0.20, 1, SKILL_PRICE_FLAT, TAG_MAIN_DISHES},
      {5, 0.30, 2, SKILL_PRICE_FLAT, TAG_MAIN_DISHES}},
     2},

    {"Sakiri",
     {{1, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {5, 0.30, 3, SKILL_PRICE_FLAT, TAG_ANY}},
     2},

    {"Skia",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     2},

    {"Baicang",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     2},

    {"Mint",
     {{1, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {5, 0.18, 0, SKILL_PRICE_FLAT, TAG_NONE}},
     2},

    {"Haniel",
     {{1, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {3, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {5, 0.18, 0, SKILL_PRICE_FLAT, TAG_NONE}},
     3},

    {"Adler",
     {{1, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {5, 0.18, 0, SKILL_PRICE_FLAT, TAG_NONE}},
     2},

    {"Dafodil",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     2},

    {"Edgar",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {3, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     3},

    {"Aurelia",
     {{1, 0.12, 0, SKILL_PRICE_FLAT, TAG_NONE},
      {2, 0.01, 2, SKILL_TRAFFIC_MULTIPLY, TAG_BEVERAGES},
      {5, 0.015, 2, SKILL_TRAFFIC_MULTIPLY, TAG_BEVERAGES}},
     3},

    {"Chiz",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     2},

    {"Lacrimosa",
     {{1, 0.01, 1, SKILL_PRICE_MULTIPLY, TAG_DESSERTS},
      {5, 0.015, 2, SKILL_PRICE_MULTIPLY, TAG_DESSERTS}},
     2},

    {"Shinku",
     {{1, 0.30, 3, SKILL_PRICE_FLAT, TAG_BEVERAGES},
      {2, 0.30, 3, SKILL_PRICE_FLAT, TAG_BEVERAGES},
      {5, 0.03, 3, SKILL_PRICE_MULTIPLY, TAG_BEVERAGES}},
     3},

    {"Iroi",
     {{1, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {3, 18.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE},
      {5, 27.0, 0, SKILL_TRAFFIC_FLAT, TAG_NONE}},
     3}};

const size_t CHARACTER_COUNT = sizeof(CHARACTERS) / sizeof(CHARACTERS[0]);
