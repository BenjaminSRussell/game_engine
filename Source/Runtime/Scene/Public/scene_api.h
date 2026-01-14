/**
 * @file scene_api.h
 * @brief Scene/ECS subsystem API
 */

#ifndef VOXELFORGE_SCENE_API_H
#define VOXELFORGE_SCENE_API_H

#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Quaternion/quat.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Entity Handle
// ============================================================================

typedef struct Entity {
  u32 id;
  u32 generation;
} Entity;

#define ENTITY_NULL ((Entity){0, 0})
#define ENTITY_IS_NULL(e) ((e).id == 0)
#define ENTITY_EQUALS(a, b)                                                    \
  ((a).id == (b).id && (a).generation == (b).generation)

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct World World;
typedef struct Query Query;
typedef struct ComponentType ComponentType;

// ============================================================================
// World
// ============================================================================

VF_API World *world_create(void);
VF_API void world_destroy(World *world);
VF_API void world_update(World *world, f32 delta_time);
VF_API void world_clear(World *world);

// ============================================================================
// Entity
// ============================================================================

VF_API Entity entity_create(World *world);
VF_API void entity_destroy(World *world, Entity entity);
VF_API b8 entity_is_alive(World *world, Entity entity);
VF_API void entity_set_name(World *world, Entity entity, const char *name);
VF_API const char *entity_get_name(World *world, Entity entity);

// ============================================================================
// Hierarchy
// ============================================================================

VF_API void entity_set_parent(World *world, Entity child, Entity parent);
VF_API Entity entity_get_parent(World *world, Entity entity);
VF_API u32 entity_get_child_count(World *world, Entity entity);
VF_API Entity entity_get_child(World *world, Entity entity, u32 index);
VF_API void entity_detach_from_parent(World *world, Entity entity);

// ============================================================================
// Transform Component (Built-in)
// ============================================================================

typedef struct TransformComponent {
  Vec3 position;
  Quat rotation;
  Vec3 scale;
  Mat4 local_matrix;
  Mat4 world_matrix;
  b8 dirty;
} TransformComponent;

VF_API TransformComponent *transform_get(World *world, Entity entity);
VF_API void transform_set_position(World *world, Entity entity, Vec3 position);
VF_API void transform_set_rotation(World *world, Entity entity, Quat rotation);
VF_API void transform_set_scale(World *world, Entity entity, Vec3 scale);
VF_API Vec3 transform_get_world_position(World *world, Entity entity);
VF_API Quat transform_get_world_rotation(World *world, Entity entity);
VF_API Vec3 transform_get_forward(World *world, Entity entity);
VF_API Vec3 transform_get_right(World *world, Entity entity);
VF_API Vec3 transform_get_up(World *world, Entity entity);

// ============================================================================
// Component Registration
// ============================================================================

typedef void (*ComponentInitFn)(void *component);
typedef void (*ComponentDestroyFn)(void *component);
typedef void (*ComponentCopyFn)(void *dst, const void *src);

typedef struct ComponentDesc {
  const char *name;
  usize size;
  usize alignment;
  ComponentInitFn init;
  ComponentDestroyFn destroy;
  ComponentCopyFn copy;
} ComponentDesc;

VF_API ComponentType *component_register(World *world,
                                         const ComponentDesc *desc);
VF_API ComponentType *component_find(World *world, const char *name);

// ============================================================================
// Component Operations
// ============================================================================

VF_API void *component_add(World *world, Entity entity, ComponentType *type);
VF_API void component_remove(World *world, Entity entity, ComponentType *type);
VF_API void *component_get(World *world, Entity entity, ComponentType *type);
VF_API b8 component_has(World *world, Entity entity, ComponentType *type);

// ============================================================================
// Query
// ============================================================================

typedef struct QueryDesc {
  ComponentType **with;
  u32 with_count;
  ComponentType **without;
  u32 without_count;
} QueryDesc;

typedef struct QueryIterator {
  Query *query;
  u32 archetype_index;
  u32 entity_index;
  Entity current_entity;
} QueryIterator;

VF_API Query *query_create(World *world, const QueryDesc *desc);
VF_API void query_destroy(Query *query);
VF_API QueryIterator query_iter(Query *query);
VF_API b8 query_next(QueryIterator *iter);
VF_API Entity query_entity(QueryIterator *iter);
VF_API void *query_get(QueryIterator *iter, ComponentType *type);
VF_API u32 query_count(Query *query);

// ============================================================================
// Systems
// ============================================================================

typedef void (*SystemFn)(World *world, f32 delta_time, void *user_data);

typedef enum SystemPhase {
  SYSTEM_PHASE_PRE_UPDATE = 0,
  SYSTEM_PHASE_UPDATE,
  SYSTEM_PHASE_POST_UPDATE,
  SYSTEM_PHASE_FIXED_UPDATE,
  SYSTEM_PHASE_RENDER,
} SystemPhase;

VF_API void system_register(World *world, const char *name, SystemPhase phase,
                            SystemFn fn, void *user_data);
VF_API void system_unregister(World *world, const char *name);
VF_API void system_set_enabled(World *world, const char *name, b8 enabled);

// ============================================================================
// Prefabs
// ============================================================================

typedef struct Prefab Prefab;

VF_API Prefab *prefab_create(World *world, const char *name);
VF_API void prefab_destroy(Prefab *prefab);
VF_API void prefab_add_component(Prefab *prefab, ComponentType *type,
                                 const void *data);
VF_API Entity prefab_instantiate(World *world, Prefab *prefab);
VF_API Entity prefab_instantiate_at(World *world, Prefab *prefab, Vec3 position,
                                    Quat rotation);

// ============================================================================
// Scene
// ============================================================================

VF_API VF_Result scene_init(void);
VF_API void scene_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_SCENE_API_H
