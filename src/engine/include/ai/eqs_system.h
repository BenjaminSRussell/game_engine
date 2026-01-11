// AI/Environment Query System (EQS)
#ifndef EQS_SYSTEM_H
#define EQS_SYSTEM_H

#include "include/common.h"
#include "include/math/vec3.h"

#define MAX_EQS_ITEMS 512

typedef enum {
  EQS_TEST_DISTANCE,
  EQS_TEST_DOT,
  EQS_TEST_PATHFINDING,
  EQS_TEST_TRACE,
  EQS_TEST_VISIBILITY
} EQSTestType;

typedef struct {
  Vec3 location;
  f32 score;
  bool is_valid;
  void *actor_reference; // Implementation specific
} EQSItem;

typedef struct {
  char name[4]; // e.g. "DST"
  EQSTestType type;

  // Configuration
  f32 weight;

  // Filters
  f32 min_limit;
  f32 max_limit;
  bool filter_only; // Don't score, just cull
  bool clamp_score;

  // Params
  Vec3 reference_location; // e.g. Querier vs Target
  void *context_actor;

} EQSTest;

typedef struct {
  char name[64];

  // Generator (How to make items)
  // Simplified: just grid or circle around querier
  enum { GEN_POINT_GRID, GEN_CIRCLE, GEN_DONUT } generator_type;
  f32 radius;
  f32 spacing;

  EQSTest tests[8];
  u32 test_count;

} EQSQueryTemplate;

typedef struct {
  EQSQueryTemplate *tmplate;
  void *querier;

  EQSItem items[MAX_EQS_ITEMS];
  u32 item_count;

  bool finished;
  int best_item_index;

} EQSQueryInstance;

#ifdef __cplusplus
extern "C" {
#endif

// API
EQSQueryTemplate *eqs_create_template(const char *name);
void eqs_add_test(EQSQueryTemplate *t, EQSTest test);

EQSQueryInstance *eqs_run_query(EQSQueryTemplate *t, void *querier);
void eqs_update(EQSQueryInstance *query,
                f32 time_budget); // Time-sliced execution

Vec3 eqs_get_best_location(EQSQueryInstance *query);

#ifdef __cplusplus
}
#endif

#endif // EQS_SYSTEM_H
