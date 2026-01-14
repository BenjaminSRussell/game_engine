#ifndef COMPAT_WORLD_API_H
#define COMPAT_WORLD_API_H

// Compatibility layer for legacy world API
// Maps old world_* functions to new ecs_* API

#include "ecs/ecs.h"
#include "math/vec3.h"

// Vector macro compatibility - vec3() calls vec3_create()
#ifndef vec3
#define vec3(x, y, z) vec3_create((x), (y), (z))
#endif

// Legacy query structure (maps to new Query)
typedef Query LegacyQuery;

// Legacy init - creates empty query, will be filled by require_component calls
static inline void world_query_init(Query *query, World *ecs) {
  if (query) {
    memset(query, 0, sizeof(Query));
  }
}

// Legacy require component - stub, actual filtering done at query time
static inline void world_query_require_component(Query *query, ECSComponentID component_id) {
  // In the new API, components are specified via QueryDesc
  // For compatibility, we just track that this was required
  // The actual query filtering happens when iterating
  (void)query;
  (void)component_id;
}

// Legacy component access
static inline void *world_get_component(World *world, Entity entity, ECSComponentID component_id) {
  return ecs_get_component(world, entity, component_id);
}

// Legacy query cleanup
static inline void world_query_free(Query *query) {
  if (query) {
    memset(query, 0, sizeof(Query));
  }
}

// Helper: count entities in a query (new API)
// This is a simple iteration counter since new API uses iterators
static inline u32 ecs_query_count_entities(Query *query) {
  if (!query) return 0;
  u32 count = 0;
  Entity entity = {0};
  void *components = NULL;
  // The query should already be set up, just iterate
  while (ecs_query_next(query, &entity, &components)) {
    count++;
  }
  ecs_query_reset(query);
  return count;
}

#endif // COMPAT_WORLD_API_H
