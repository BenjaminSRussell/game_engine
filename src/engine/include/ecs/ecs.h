// include/engine/ecs/ecs.h
//
// Purpose: Entity-Component-System architecture designed to outperform Unity's
// DOTS This ECS implementation uses data-oriented design with cache-friendly
// layouts, parallel execution, and advanced query systems for maximum
// performance.
//
// Key Features:
// - Archetype-based storage for optimal cache locality
// - Multi-threaded system execution with job scheduling
// - Component queries with parallel processing
// - Efficient memory management with object pooling
// - Real-time serialization and versioning
// - Advanced debugging and profiling integration
//
// Performance Advantages over Unity DOTS:
// - Better cache locality with archetype chunking
// - Lock-free parallel system execution
// - SIMD-optimized component operations
// - Zero-allocation query system
// - Hot-reloadable components and systems
//
// Public APIs:
// - World: Main ECS container managing entities, components, and systems
// - Entity: Lightweight handle to game objects
// - Component: Data containers stored in archetypes
// - System: Logic operating on component queries
// - Query: Efficient component filtering and iteration
//
// Ownership: World owns all entities, components, and systems
// Invariants: Entities are valid within their world, components belong to
// archetypes
//
#ifndef ECS_H
#define ECS_H

#include "../common.h"
#include "../core/performance.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct World {
  // Public configuration
  struct {
    u32 max_entities;
    u32 max_components;
    u32 max_systems;
  } config;

  // Internal state (exposed for game.c malloc)
  void *component_info;
  void *archetypes;
  void *entities;
  void *systems;
  u32 entity_count;
  u32 component_count;
  u32 system_count;
} World;
typedef struct Entity Entity;
typedef struct Component Component;
typedef struct ECSSystem System;
typedef struct ECSQuery Query;
typedef struct ECSArchetype Archetype;

// Component type registration
typedef u32 ComponentType;
typedef u32 ECSComponentID;

// Component metadata
typedef struct {
  ComponentType type;
  const char *name;
  u32 size;
  u32 alignment;
  bool is_tag; // Zero-size marker components
  void (*default_constructor)(void *data);
  void (*destructor)(void *data);
  void (*serialize)(const void *data, char *buffer, u32 buffer_size);
  void (*deserialize)(void *data, const char *buffer);
} ComponentInfo;

// Entity handle (32 bits for efficiency)
struct Entity {
  u32 id : 24;
  u32 generation : 8;
};

// Entity constants
#define INVALID_ENTITY ((Entity){0, 0})
#ifndef MAX_ENTITIES
#define MAX_ENTITIES (1 << 24)
#endif

// Archetype chunk for cache-friendly storage
#define ARCHETYPE_CHUNK_SIZE 16384 // 16KB chunks for optimal cache usage

typedef struct {
  u8 data[ARCHETYPE_CHUNK_SIZE];
  u32 entity_count;
  u32 capacity;
  ECSArchetype *archetype;
  struct Chunk *next_chunk;
} ECSChunk;

// Archetype - defines component composition
struct ECSArchetype {
  ComponentType *component_types;
  u32 component_count;
  u32 total_component_size;
  ECSChunk *chunks;
  u32 chunk_count;
  u32 entity_capacity;
  u32 entity_count;
  struct ECSArchetype *next; // For archetype graph traversal
};

// Query description for component filtering
typedef struct {
  ComponentType *all_components; // Must have all these
  u32 all_count;
  ComponentType *any_components; // Must have at least one of these
  u32 any_count;
  ComponentType *none_components; // Must not have any of these
  u32 none_count;
  bool changed_only; // Only modified components
} QueryDesc;

// Query result iterator
struct ECSQuery {
  Archetype **matching_archetypes;
  u32 archetype_count;
  u32 current_archetype;
  ECSChunk *current_chunk;
  u32 current_entity;
  ComponentType *component_types;
  u32 component_count;
  u32 *component_offsets;
};

// System execution context
typedef struct {
  World *world;
  Query *query;
  f32 delta_time;
  void *user_data;
} SystemContext;

// System function signature
typedef void (*SystemFunc)(SystemContext *context);

// System scheduling and execution
struct ECSSystem {
  const char *name;
  SystemFunc execute;
  QueryDesc query;
  u32 priority; // Execution order
  bool enabled;
  bool parallel;       // Can run in parallel with other systems
  u32 thread_affinity; // Specific thread (-1 for any)
  f64 execution_time;  // Performance tracking
  u32 execution_count;
};

// World configuration
typedef struct {
  u32 max_entities;
  u32 max_archetypes;
  u32 max_systems;
  u32 worker_threads;
  u32 chunk_size;
  bool enable_profiling;
  bool enable_serialization;
} WorldConfig;

// Main ECS world
// [Duplicate World struct removed]

// ============================================================================
// WORLD MANAGEMENT
// ============================================================================

// Create and destroy worlds
World *ecs_world_create(const WorldConfig *config);
void ecs_world_destroy(World *world);
void ecs_world_free(World *world); // Matching game.c expectation

// Initialization (for game.c which allocs World)
void ecs_world_init(World *world, u32 max_entities, u32 max_components,
                    u32 max_systems);

// Helpers
Entity ecs_get_entity_by_id(World *world, u32 id);
u32 ecs_get_max_entity_id(World *world);

// World configuration
WorldConfig ecs_world_create_default_config(void);
WorldConfig ecs_world_create_high_performance_config(void);

// ============================================================================
// COMPONENT REGISTRATION
// ============================================================================

// Register component types
ECSComponentID ecs_register_component(World *world, const ComponentInfo *info);
ECSComponentID ecs_register_component_simple(World *world, const char *name,
                                             u32 size);

// Component information
const ComponentInfo *ecs_get_component_info(World *world, ECSComponentID id);
ECSComponentID ecs_get_component_id(World *world, const char *name);

// ============================================================================
// ENTITY MANAGEMENT
// ============================================================================

// Create and destroy entities
Entity ecs_create_entity(World *world);
Entity ecs_create_entity_with(World *world, ComponentType *components,
                              u32 count);
void ecs_destroy_entity(World *world, Entity entity);

// Entity validation
bool ecs_is_valid(World *world, Entity entity);
bool ecs_is_entity_valid(World *world, Entity entity);
bool ecs_is_entity_alive(World *world, Entity entity);

// ============================================================================
// COMPONENT OPERATIONS
// ============================================================================

// Add/remove components
bool ecs_add_component(World *world, Entity entity, ECSComponentID component_id,
                       const void *data);
bool ecs_remove_component(World *world, Entity entity,
                          ECSComponentID component_id);
bool ecs_has_component(World *world, Entity entity,
                       ECSComponentID component_id);

// Entity validation
bool ecs_entity_exists(World *world, Entity entity);
bool ecs_entity_is_alive(World *world, Entity entity);

// Component access
void *ecs_get_component(World *world, Entity entity,
                        ECSComponentID component_id);
const void *ecs_get_component_const(World *world, Entity entity,
                                    ECSComponentID component_id);

// Component modification
bool ecs_set_component(World *world, Entity entity, ECSComponentID component_id,
                       const void *data);

// ============================================================================
// QUERY SYSTEM
// ============================================================================

// Create and manage queries
Query *ecs_query_create(World *world, const QueryDesc *desc);
void ecs_query_destroy(World *world, Query *query);

// Query iteration
bool ecs_query_next(Query *query, Entity *entity, void **components);
void ecs_query_reset(Query *query);

// Query statistics
u32 ecs_query_count(Query *query);
u32 ecs_query_archetype_count(Query *query);

// ============================================================================
// SYSTEM MANAGEMENT
// ============================================================================

// Create and manage systems
System *ecs_system_create(World *world, const char *name, SystemFunc execute,
                          const QueryDesc *query);
void ecs_system_destroy(World *world, System *system);

// System configuration
void ecs_system_set_priority(World *world, System *system, u32 priority);
void ecs_system_set_parallel(World *world, System *system, bool parallel);
void ecs_system_set_enabled(World *world, System *system, bool enabled);

// ============================================================================
// WORLD EXECUTION
// ============================================================================

// Execute all systems
void ecs_world_update(World *world, f32 delta_time);

// Execute specific systems
void ecs_world_execute_systems(World *world, f32 delta_time);

// System scheduling
void ecs_world_rebuild_execution_order(World *world);

// ============================================================================
// SERIALIZATION
// ============================================================================

// Save/load world state
bool ecs_world_save(World *world, const char *filename);
bool ecs_world_load(World *world, const char *filename);

// Entity serialization
bool ecs_entity_save(World *world, Entity entity, char *buffer,
                     u32 buffer_size);
bool ecs_entity_load(World *world, const char *buffer, Entity *entity);

// ============================================================================
// DEBUGGING AND PROFILING
// ============================================================================

// World statistics
typedef struct {
  u32 entity_count;
  u32 archetype_count;
  u32 system_count;
  u32 component_type_count;
  u32 total_memory_usage;
  u32 chunk_count;
  f64 average_frame_time;
  f64 peak_frame_time;
} WorldStats;

WorldStats ecs_world_get_stats(World *world);
void ecs_world_print_stats(World *world);

// Performance profiling
void ecs_world_enable_profiling(World *world, bool enable);
void ecs_world_print_performance_report(World *world);

// Entity debugging
void ecs_entity_print_components(World *world, Entity entity);
void ecs_archetype_print_components(const Archetype *archetype);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Component registration macro
#define ECS_REGISTER_COMPONENT(world, type, name)                              \
  ecs_register_component_simple(world, name, sizeof(type))

// Component access macro
#define ECS_GET_COMPONENT(world, entity, type)                                 \
  ((type *)ecs_get_component(world, entity, ecs_get_component_id(world, #type)))

// Query builder macros
#define ECS_QUERY_ALL(...)                                                     \
  (QueryDesc) {                                                                \
    .all_components = (ComponentType[]){__VA_ARGS__},                          \
    .all_count =                                                               \
        sizeof((ComponentType[]){__VA_ARGS__}) / sizeof(ComponentType)         \
  }

#define ECS_QUERY_ANY(...)                                                     \
  (QueryDesc) {                                                                \
    .any_components = (ComponentType[]){__VA_ARGS__},                          \
    .any_count =                                                               \
        sizeof((ComponentType[]){__VA_ARGS__}) / sizeof(ComponentType)         \
  }

#define ECS_QUERY_NONE(...)                                                    \
  (QueryDesc) {                                                                \
    .none_components = (ComponentType[]){__VA_ARGS__},                         \
    .none_count =                                                              \
        sizeof((ComponentType[]){__VA_ARGS__}) / sizeof(ComponentType)         \
  }

// System creation macro
#define ECS_SYSTEM_CREATE(world, name, func, query)                            \
  ecs_system_create(world, name, func, query)

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Prefab system
typedef struct {
  Entity prefab_entity;
  char *name;
  ComponentType *component_types;
  u32 component_count;
} Prefab;

Prefab *ecs_prefab_create(World *world, const char *name);
Entity ecs_prefab_instantiate(World *world, Prefab *prefab);
void ecs_prefab_destroy(World *world, Prefab *prefab);

// Event system
typedef struct {
  ComponentType event_type;
  void *event_data;
  Entity target_entity;
} ECSEvent;

void ecs_world_emit_event(World *world, const ECSEvent *event);
void ecs_world_add_event_system(World *world, ComponentType event_type,
                                SystemFunc handler);

// Resource management (global singletons)
void ecs_world_set_resource(World *world, const char *name, void *resource);
void *ecs_world_get_resource(World *world, const char *name);

// Component relationships (parent-child, references)
bool ecs_set_parent(World *world, Entity child, Entity parent);
Entity ecs_get_parent(World *world, Entity entity);
u32 ecs_get_child_count(World *world, Entity entity);
void ecs_get_children(World *world, Entity entity, Entity *children,
                      u32 max_children);

// ============================================================================
// SIMD OPTIMIZATION
// ============================================================================

// SIMD-optimized component operations (for common component types)
void ecs_transform_components_simd(void *components, u32 count,
                                   const void *transform_data);
void ecs_physics_components_simd(void *components, u32 count, f32 delta_time);
void ecs_animation_components_simd(void *components, u32 count, f32 delta_time);

// ============================================================================
// THREADING AND PARALLELISM
// ============================================================================

// Job system for parallel system execution
typedef struct {
  System *system;
  SystemContext context;
  u32 start_index;
  u32 end_index;
  u32 thread_id;
} SystemJob;

void ecs_world_execute_parallel(World *world, f32 delta_time);
void ecs_world_set_worker_threads(World *world, u32 thread_count);

// =============================================================================
// ECS World Management
// =============================================================================

/**
 * Initialize an ECS world with specified capacities
 * @param world Pointer to World structure to initialize
 * @param max_entities Maximum number of entities
 * @param max_components Maximum number of components
 * @param max_systems Maximum number of systems
 */
void ecs_world_init(World *world, u32 max_entities, u32 max_components,
                    u32 max_systems);

/**
 * Free all resources associated with an ECS world
 * @param world Pointer to World to free
 */
void ecs_world_free(World *world);

/**
 * Update all systems in the ECS world
 * @param world Pointer to World
 * @param delta_time Time elapsed since last update in seconds
 */
void ecs_update_systems(World *world, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // ECS_H
