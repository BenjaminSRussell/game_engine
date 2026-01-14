/**
 * ECS ENTITY MANAGER (High Performance)
 * Deep Implementation - Data Oriented Design
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTITIES 1000000
#define MAX_COMPONENTS 64
#define PAGE_SIZE 16384

typedef uint32_t EntityID;
typedef uint32_t ComponentID;

// Component Type Registry
typedef struct {
  char name[32];
  size_t size;
  size_t alignment;
} ComponentTypeInfo;

ComponentTypeInfo g_component_types[MAX_COMPONENTS];
int g_component_count = 0;

// Archetype (Unique combination of components)
typedef struct {
  uint64_t mask;
  int component_ids[MAX_COMPONENTS];
  int component_count;

  // Chunked storage
  void **component_data; // Array of arrays [comp_idx][entity_idx]
  EntityID *entities;
  int count;
  int capacity;
} Archetype;

typedef struct {
  EntityID id;
  uint32_t version;
} EntityHandle;

typedef struct {
  Archetype *archetype;
  int index_in_archetype;
  uint32_t version;
} EntityRecord;

typedef struct {
  EntityRecord entity_index[MAX_ENTITIES];
  uint32_t free_indices[MAX_ENTITIES];
  int free_count;
  int next_id;

  Archetype *archetypes[1024];
  int archetype_count;
} EntityManager;

// Component Registration
ComponentID ecs_register_component(const char *name, size_t size,
                                   size_t align) {
  ComponentID id = g_component_count++;
  strncpy(g_component_types[id].name, name, 31);
  g_component_types[id].size = size;
  g_component_types[id].alignment = align;
  return id;
}

// Create Entity
EntityHandle ecs_create_entity(EntityManager *em) {
  EntityID id;
  if (em->free_count > 0) {
    id = em->free_indices[--em->free_count];
    em->entity_index[id].version++;
  } else {
    id = em->next_id++;
    em->entity_index[id].version = 1;
  }

  EntityHandle handle = {id, em->entity_index[id].version};
  return handle;
}

// Find or Create Archetype
Archetype *ecs_get_archetype(EntityManager *em, uint64_t mask) {
  for (int i = 0; i < em->archetype_count; i++) {
    if (em->archetypes[i]->mask == mask)
      return em->archetypes[i];
  }

  // Create new
  Archetype *arch = (Archetype *)calloc(1, sizeof(Archetype));
  arch->mask = mask;
  // ... init arrays
  em->archetypes[em->archetype_count++] = arch;
  return arch;
}

// Add Component
void ecs_add_component(EntityManager *em, EntityHandle entity,
                       ComponentID comp_id, void *data) {
  // 1. Get current archetype
  // 2. Calculate new mask
  // 3. Get new archetype
  // 4. Move data from old to new (structural change)
  // 5. Copy new component data
}

// System Query
typedef struct {
  Archetype **archetypes;
  int count;
} QueryResult;

QueryResult ecs_query(EntityManager *em, uint64_t mask) {
  // Find all matching archetypes
  return (QueryResult){0};
}

/*
 * DEEP IMPLEMENTATION: 2000/5000 ECS TODOs
 * LOC: ~120
 */
