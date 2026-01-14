// eqs_system.c - Implementation
#include "include/ai/eqs_system.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

EQSQueryTemplate *eqs_create_template(const char *name) {
  EQSQueryTemplate *t = calloc(1, sizeof(EQSQueryTemplate));
  t->generator_type = GEN_POINT_GRID;
  t->radius = 1000.0f;
  t->spacing = 100.0f;
  LOG_INFO("EQS Template created: %s", name);
  return t;
}

EQSQueryInstance *eqs_run_query(EQSQueryTemplate *t, void *querier) {
  EQSQueryInstance *q = calloc(1, sizeof(EQSQueryInstance));
  q->tmplate = t;
  q->querier = querier;

  // Generate points
  u32 side = (u32)(t->radius * 2.0f / t->spacing);
  q->item_count = side * side;
  if (q->item_count > MAX_EQS_ITEMS)
    q->item_count = MAX_EQS_ITEMS;

  for (u32 i = 0; i < q->item_count; i++) {
    q->items[i].is_valid = true;
    q->items[i].score = 1.0f;
    // set location...
  }

  return q;
}
