#ifndef ECS_TYPES_H
#define ECS_TYPES_H

#include "../Public/unified_ecs.h"
#include <core/types.h>

// ============================================================================
// Internal Data Structures
// ============================================================================

// Entity Record: Tracks where an entity lives (archetype + index)
typedef struct {
  bool active;
  void *archetype_ptr; // Points to SimpleArchetype
  u32 chunk_idx; // Not fully utilized in simple implementation, but good for
                 // future chunking
  u32 index_in_chunk; // Index within the archetype's dense arrays
  u8 generation;
} EntityRecord;

// Entity Manager Data
typedef struct {
  EntityRecord *records;
  u32 next_entity_id;
  u32 *free_entity_ids;
  u32 free_count;
  u32 capacity;
} EntityData;

// Component Data Storage Metadata
typedef struct {
  ComponentInfo *infos;
  u32 count;
  u32 capacity;
} ComponentDataInternal;

// Archetype Definition
// An archetype is a unique combination of components.
// Entities with the same components are stored together in an archeytype.
typedef struct SimpleArchetype {
  ECSComponentID
      component_types[32]; // Hardcoded max components per entity for now
  u32 component_count;

  u32 entity_capacity;
  u32 entity_count;

  Entity *entities;      // Dense array of entities in this archetype
  void **component_data; // Array of pointers, each pointing to a flat array of
                         // component data

  struct SimpleArchetype *next; // Linked list of archetypes
} SimpleArchetype;

// Archetype Manager Data
typedef struct {
  SimpleArchetype *head;
  SimpleArchetype *empty_archetype; // Archetype 0 (no components)
  u32 count;
} ArchetypeData;

// World Definition
struct World {
  WorldConfig config;

  void *entities;       // Cast to EntityData*
  void *component_info; // Cast to ComponentDataInternal*
  void *archetypes;     // Cast to ArchetypeData*

  u32 entity_count;
  u32 component_count;
  u32 system_count;

  struct System *systems_head;
};

// Query Definition
struct Query {
  ECSComponentID *component_types; // Types requested (All)
  u32 component_count;

  // Cached matching archetypes for iteration
  struct SimpleArchetype *
      *matching_archetypes; // Array of pointers to archetypes
  u32 archetype_count;

  // Iterator state
  u32 current_archetype;
  u32 current_entity;
  void *current_chunk; // Reserved

  u32 *component_offsets; // Helper for retrieving pointers
};

// System Definition
struct System {
  char name[64];
  void (*update_fn)(World *world, float dt);
  struct System *next;
};

#endif // ECS_TYPES_H
