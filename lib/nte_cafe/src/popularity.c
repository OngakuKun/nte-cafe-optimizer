#include "popularity.h"

/*
 * The game shows the popularity bonus of the currently reached level, not the
 * sum of all lower levels. Reaching a higher level replaces the lower bonus.
 * Summing the highest level of every shop gives the displayed
 * "Gesamtmultiplikator fuer alle Laeden" (0.090 with all tiers reached).
 */
typedef struct {
  int threshold;
  double bonus;
} PopularityTier;

static const PopularityTier CAFE_1[] = {
    {50, 0.005}, {100, 0.007}, {150, 0.010}};

static const PopularityTier CAFE_2[] = {
    {100, 0.003}, {200, 0.006}, {300, 0.009}, {400, 0.012}, {500, 0.015}};

static const PopularityTier CAFE_4[] = {
    {150, 0.005}, {300, 0.010}, {450, 0.015}, {600, 0.020}, {750, 0.025}};

static const PopularityTier *const CAFE_TIERS[MAX_CAFES] = {
    CAFE_1, CAFE_2, CAFE_2, CAFE_4, CAFE_4};

static const int CAFE_TIER_COUNTS[MAX_CAFES] = {
    (int)(sizeof(CAFE_1) / sizeof(CAFE_1[0])),
    (int)(sizeof(CAFE_2) / sizeof(CAFE_2[0])),
    (int)(sizeof(CAFE_2) / sizeof(CAFE_2[0])),
    (int)(sizeof(CAFE_4) / sizeof(CAFE_4[0])),
    (int)(sizeof(CAFE_4) / sizeof(CAFE_4[0]))};

int popularity_max_level(int cafe_index) {
  if (cafe_index < 0 || cafe_index >= MAX_CAFES)
    return 0;
  return CAFE_TIER_COUNTS[cafe_index];
}

int popularity_threshold(int cafe_index, int level) {
  if (cafe_index < 0 || cafe_index >= MAX_CAFES)
    return 0;
  if (level < 1 || level > CAFE_TIER_COUNTS[cafe_index])
    return 0;
  return CAFE_TIERS[cafe_index][level - 1].threshold;
}

double popularity_level_bonus(int cafe_index, int level) {
  if (cafe_index < 0 || cafe_index >= MAX_CAFES)
    return 0.0;
  if (level < 1 || level > CAFE_TIER_COUNTS[cafe_index])
    return 0.0;
  return CAFE_TIERS[cafe_index][level - 1].bonus;
}

double popularity_cafe_bonus(const CafeSettings *settings, int cafe_index) {
  if (!settings || cafe_index < 0 || cafe_index >= MAX_CAFES)
    return 0.0;

  int level = settings->cafe_popularity[cafe_index];
  if (level <= 0)
    return 0.0;
  if (level > CAFE_TIER_COUNTS[cafe_index])
    level = CAFE_TIER_COUNTS[cafe_index];

  return CAFE_TIERS[cafe_index][level - 1].bonus;
}

double popularity_total_bonus(const CafeSettings *settings) {
  if (!settings)
    return 0.0;

  int cafes = settings->cafes_owned;
  if (cafes < 0)
    cafes = 0;
  if (cafes > MAX_CAFES)
    cafes = MAX_CAFES;

  double total = 0.0;
  for (int i = 0; i < cafes; ++i)
    total += popularity_cafe_bonus(settings, i);
  return total;
}
