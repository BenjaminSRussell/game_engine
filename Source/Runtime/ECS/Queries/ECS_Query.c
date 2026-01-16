#include "../../Memory/Public/Memory.h"
#include "../Public/ECS.h"
#include <string.h>

// Simple Query implementation that iterates all archetypes
// matching signature.

#define MAX_QUERY_COMPONENTS 8

typedef struct Query {
  ComponentType all[MAX_QUERY_COMPONENTS];
  u32 all_count;
  ComponentType any[MAX_QUERY_COMPONENTS];
  u32 any_count;
  ComponentType none[MAX_QUERY_COMPONENTS];
  u32 none_count;
} Query;

// Stub for Query Manager - in full impl, caches queries
void QueryManager_Init(void) {}

// Check if archetype matches query
// Archetype structure is external, we need a way to check.
// For now, this logic will be inside Query iterators or Archetype loops.
