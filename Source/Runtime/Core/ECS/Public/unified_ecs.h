#ifndef UNIFIED_ECS_H
#define UNIFIED_ECS_H

#include <core/types.h>
#include <stdbool.h>

// ============================================================================
// Opaque Types
// ============================================================================
typedef struct World World;
typedef struct Query Query;
typedef struct System System;

// Static component ID type
typedef u32 ECSComponentID;

// ============================================================================
// Entity Definition
// ============================================================================
typedef struct {
  u32 id;
  u32 generation;
} Entity;

// ============================================================================
// Configuration
// ============================================================================
typedef struct {
  u32 max_entities;
  u32 max_components;
  u32 max_systems;
  u32 max_archetypes;
} WorldConfig;

// Component Information
typedef struct {
  const char *name;
  u32 size;
  u32 alignment;
} ComponentInfo;

// Query Description
typedef struct {
  ECSComponentID *all_components;
  u32 all_count;
  ECSComponentID *any_components;
  u32 any_count;
  ECSComponentID *none_components;
  u32 none_count;
} QueryDesc;

// ============================================================================
// Public API
// ============================================================================

// World Management
World *ecs_world_create(const WorldConfig *config);
void ecs_world_destroy(World *world);
WorldConfig ecs_world_create_default_config(void);

// Entity Management
Entity ecs_create_entity(World *world);
void ecs_destroy_entity(World *world, Entity entity);
bool ecs_is_valid(World *world, Entity entity);
Entity ecs_get_entity_by_id(World *world, u32 id);
u32 ecs_get_max_entity_id(World *world);

// Component Management
ECSComponentID ecs_register_component(World *world, const ComponentInfo *info);
bool ecs_add_component(World *world, Entity entity, ECSComponentID type,
                       const void *data);
bool ecs_remove_component(World *world, Entity entity, ECSComponentID type);
bool ecs_has_component(World *world, Entity entity, ECSComponentID type);
void *ecs_get_component(World *world, Entity entity, ECSComponentID type);
bool ecs_set_component(World *world, Entity entity, ECSComponentID type,
                       const void *data);

// Query Management
Query *ecs_query_create(World *world, const QueryDesc *desc);
void ecs_query_destroy(World *world, Query *query);
void ecs_query_reset(Query *query);
bool ecs_query_next(Query *query, Entity *entity, void **components);

// ============================================================================
// System Management
// ============================================================================
typedef void (*SystemUpdateFn)(World *world, float dt);

void ecs_register_system(World *world, const char *name,
                         SystemUpdateFn update_fn);
void ecs_run_systems(World *world, float dt);

#endif // UNIFIED_ECS_H
