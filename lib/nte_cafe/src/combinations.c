#include "combinations.h"

#include <stdlib.h>

typedef struct {
  size_t n;
  size_t k;
  size_t *indices;
  CombinationCallback callback;
  void *userdata;
  bool stopped;
} State;

static void recurse(State *s, size_t depth, size_t next) {
  if (s->stopped)
    return;

  if (depth == s->k) {
    if (!s->callback(s->indices, s->k, s->userdata))
      s->stopped = true;
    return;
  }

  size_t remaining = s->k - depth;
  for (size_t i = next; i + remaining <= s->n; ++i) {
    s->indices[depth] = i;
    recurse(s, depth + 1, i + 1);
    if (s->stopped)
      return;
  }
}

bool enumerate_combinations(size_t n, size_t k, CombinationCallback callback,
                            void *userdata) {
  if (!callback || k > n)
    return false;
  if (k == 0)
    return callback(NULL, 0, userdata);

  size_t *indices = malloc(k * sizeof(*indices));
  if (!indices)
    return false;

  State s = {n, k, indices, callback, userdata, false};
  recurse(&s, 0, 0);
  free(indices);
  return !s.stopped;
}
