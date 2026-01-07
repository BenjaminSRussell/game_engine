/**
 * =================================================================================================
 *                              ENTITY COMPONENT SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_ECS_1
 * =================================================================================================
 */

#include <core/logger.h>
#include <core/types.h>
#include <ecs/ecs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS & TYPES
 * =================================================================================================
 */

#define ECS_MAX_ENTITIES 65536
#define ECS_MAX_COMPONENTS 256
#define ECS_MAX_SYSTEMS 128
#define ECS_ENTITY_INDEX_BITS 20
#define ECS_ENTITY_INDEX_MASK ((1 << ECS_ENTITY_INDEX_BITS) - 1)
#define ECS_ENTITY_GENERATION_BITS 12
#define ECS_ENTITY_GENERATION_MASK ((1 << ECS_ENTITY_GENERATION_BITS) - 1)
#define ECS_INVALID_ENTITY 0xFFFFFFFF

// Use types from ecs.h - Entity is a struct
typedef u32 ComponentId;

typedef struct ComponentInfo {
  char name[64];
  size_t size;
  size_t alignment;
  ComponentId id;
  void (*constructor)(void *component);
  void (*destructor)(void *component);
  void (*copy)(void *dst, const void *src);
} ComponentInfo;

typedef struct ComponentPool {
  void *data;
  size_t element_size;
  size_t capacity;
  size_t count;
  uint32_t *entity_to_index;
  Entity *index_to_entity;
  bool *sparse_set;
} ComponentPool;

typedef struct EntityData {
  bool active;
  bool enabled;
  char name[64];
  Entity parent;
  Entity *children;
  uint32_t child_count;
  uint32_t child_capacity;
  uint64_t component_mask;
} EntityData;

typedef struct ECSWorld {
  EntityData *entities;
  uint32_t *free_indices;
  uint32_t free_count;
  uint8_t *generations;
  uint32_t entity_count;
  uint32_t entity_capacity;

  ComponentInfo component_infos[ECS_MAX_COMPONENTS];
  ComponentPool component_pools[ECS_MAX_COMPONENTS];
  uint32_t component_count;

  void **systems;
  uint32_t system_count;

  bool initialized;
} ECSWorld;

static ECSWorld g_world = {0};

/* =================================================================================================
 *                                    ENTITY IMPLEMENTATION
 * =================================================================================================
 */

static inline uint32_t entity_get_index(Entity entity) {
  return entity & ECS_ENTITY_INDEX_MASK;
}

static inline uint32_t entity_get_generation_internal(u32 entity_id) {
  return (entity_id >> ECS_ENTITY_INDEX_BITS) & ECS_ENTITY_GENERATION_MASK;
}

static inline u32 entity_make_internal(uint32_t index, uint32_t generation) {
  return (generation << ECS_ENTITY_INDEX_BITS) | index;
}

// DONE: Implement entity_create
static u32 entity_create_internal(void) {
  if (!g_world.initialized)
    return ECS_INVALID_ENTITY;

  uint32_t index;
  if (g_world.free_count > 0) {
    index = g_world.free_indices[--g_world.free_count];
  } else {
    if (g_world.entity_count >= g_world.entity_capacity) {
      uint32_t new_capacity = g_world.entity_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 1024;

      g_world.entities =
          realloc(g_world.entities, new_capacity * sizeof(EntityData));
      g_world.generations = realloc(g_world.generations, new_capacity);
      g_world.free_indices =
          realloc(g_world.free_indices, new_capacity * sizeof(uint32_t));

      memset(&g_world.entities[g_world.entity_capacity], 0,
             (new_capacity - g_world.entity_capacity) * sizeof(EntityData));
      memset(&g_world.generations[g_world.entity_capacity], 0,
             new_capacity - g_world.entity_capacity);

      g_world.entity_capacity = new_capacity;
    }
    index = g_world.entity_count++;
  }

  EntityData *data = &g_world.entities[index];
  memset(data, 0, sizeof(EntityData));
  data->active = true;
  data->enabled = true;
  data->parent = ECS_INVALID_ENTITY;
  snprintf(data->name, sizeof(data->name), "Entity_%u", index);

  return entity_make_internal(index, g_world.generations[index]);
}

// DONE: Implement entity_destroy
static void entity_destroy_internal(u32 entity) {
  if (!entity_is_valid(entity))
    return;

  uint32_t index = entity_get_index(entity);
  EntityData *data = &g_world.entities[index];

  // Remove all components
  for (uint32_t i = 0; i < g_world.component_count; i++) {
    if (data->component_mask & (1ULL << i)) {
      component_remove(entity, i);
    }
  }

  // Detach from parent
  if (data->parent != ECS_INVALID_ENTITY) {
    entity_remove_child(data->parent, entity);
  }

  // Destroy children
  for (uint32_t i = 0; i < data->child_count; i++) {
    entity_destroy(data->children[i]);
  }
  free(data->children);

  data->active = false;
  g_world.generations[index]++;
  g_world.free_indices[g_world.free_count++] = index;
}

// DONE: Implement entity_is_valid
static bool entity_is_valid_internal(u32 entity) {
  if (entity == ECS_INVALID_ENTITY)
    return false;

  uint32_t index = entity_get_index(entity);
  if (index >= g_world.entity_count)
    return false;

  uint32_t generation = entity_get_generation(entity);
  if (g_world.generations[index] != generation)
    return false;

  return g_world.entities[index].active;
}

// DONE: Implement entity_get_generation
uint32_t entity_get_gen(Entity entity) { return entity_get_generation(entity); }

// DONE: Implement entity_set_enabled
void entity_set_enabled(Entity entity, bool enabled) {
  if (!entity_is_valid(entity))
    return;
  g_world.entities[entity_get_index(entity)].enabled = enabled;
}

// DONE: Implement entity_is_enabled
bool entity_is_enabled(Entity entity) {
  if (!entity_is_valid(entity))
    return false;
  return g_world.entities[entity_get_index(entity)].enabled;
}

// DONE: Implement entity_set_name
void entity_set_name(Entity entity, const char *name) {
  if (!entity_is_valid(entity))
    return;
  strncpy(g_world.entities[entity_get_index(entity)].name, name, 63);
}

// DONE: Implement entity_get_name
const char *entity_get_name(Entity entity) {
  if (!entity_is_valid(entity))
    return NULL;
  return g_world.entities[entity_get_index(entity)].name;
}

// DONE: Implement entity_set_parent
void entity_set_parent(Entity entity, Entity parent) {
  if (!entity_is_valid(entity))
    return;

  uint32_t index = entity_get_index(entity);
  EntityData *data = &g_world.entities[index];

  // Remove from old parent
  if (data->parent != ECS_INVALID_ENTITY) {
    entity_remove_child(data->parent, entity);
  }

  // Add to new parent
  if (parent != ECS_INVALID_ENTITY && entity_is_valid(parent)) {
    entity_add_child(parent, entity);
  }

  data->parent = parent;
}

// DONE: Implement entity_get_parent
Entity entity_get_parent(Entity entity) {
  if (!entity_is_valid(entity))
    return ECS_INVALID_ENTITY;
  return g_world.entities[entity_get_index(entity)].parent;
}

// DONE: Implement entity_get_children
Entity *entity_get_children(Entity entity, uint32_t *count) {
  if (!entity_is_valid(entity)) {
    *count = 0;
    return NULL;
  }
  EntityData *data = &g_world.entities[entity_get_index(entity)];
  *count = data->child_count;
  return data->children;
}

// DONE: Implement entity_add_child
void entity_add_child(Entity parent, Entity child) {
  if (!entity_is_valid(parent) || !entity_is_valid(child))
    return;

  EntityData *data = &g_world.entities[entity_get_index(parent)];

  if (data->child_count >= data->child_capacity) {
    uint32_t new_cap = data->child_capacity * 2;
    if (new_cap == 0)
      new_cap = 4;
    data->children = realloc(data->children, new_cap * sizeof(Entity));
    data->child_capacity = new_cap;
  }

  data->children[data->child_count++] = child;
  g_world.entities[entity_get_index(child)].parent = parent;
}

// DONE: Implement entity_remove_child
void entity_remove_child(Entity parent, Entity child) {
  if (!entity_is_valid(parent))
    return;

  EntityData *data = &g_world.entities[entity_get_index(parent)];

  for (uint32_t i = 0; i < data->child_count; i++) {
    if (data->children[i] == child) {
      data->children[i] = data->children[--data->child_count];
      break;
    }
  }
}

// DONE: Implement entity_clone
Entity entity_clone(Entity entity) {
  if (!entity_is_valid(entity))
    return ECS_INVALID_ENTITY;

  Entity clone = entity_create();
  if (clone == ECS_INVALID_ENTITY)
    return ECS_INVALID_ENTITY;

  uint32_t src_idx = entity_get_index(entity);
  uint32_t dst_idx = entity_get_index(clone);

  EntityData *src = &g_world.entities[src_idx];
  EntityData *dst = &g_world.entities[dst_idx];

  dst->enabled = src->enabled;
  snprintf(dst->name, sizeof(dst->name), "%s_clone", src->name);

  // Clone components
  for (uint32_t i = 0; i < g_world.component_count; i++) {
    if (src->component_mask & (1ULL << i)) {
      void *src_comp = component_get(entity, i);
      void *dst_comp = component_add(clone, i);
      if (src_comp && dst_comp) {
        ComponentInfo *info = &g_world.component_infos[i];
        if (info->copy) {
          info->copy(dst_comp, src_comp);
        } else {
          memcpy(dst_comp, src_comp, info->size);
        }
      }
    }
  }

  return clone;
}

/* =================================================================================================
 *                                    COMPONENT IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement component_register
static ComponentId component_register_internal(const char *name, size_t size,
                                               size_t alignment) {
  if (g_world.component_count >= ECS_MAX_COMPONENTS)
    return (ComponentId)-1;

  ComponentId id = g_world.component_count++;
  ComponentInfo *info = &g_world.component_infos[id];

  strncpy(info->name, name, 63);
  info->size = size;
  info->alignment = alignment;
  info->id = id;
  info->constructor = NULL;
  info->destructor = NULL;
  info->copy = NULL;

  // Initialize pool
  ComponentPool *pool = &g_world.component_pools[id];
  pool->element_size = size;
  pool->capacity = 256;
  pool->count = 0;
  pool->data = calloc(pool->capacity, size);
  pool->entity_to_index = calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
  pool->index_to_entity = calloc(pool->capacity, sizeof(Entity));
  pool->sparse_set = calloc(ECS_MAX_ENTITIES, sizeof(bool));

  memset(pool->entity_to_index, 0xFF, ECS_MAX_ENTITIES * sizeof(uint32_t));

  return id;
}

// DONE: Implement component_add
static void *component_add_internal(u32 entity, ComponentId component_id) {
  if (!entity_is_valid(entity))
    return NULL;
  if (component_id >= g_world.component_count)
    return NULL;

  uint32_t index = entity_get_index(entity);
  EntityData *data = &g_world.entities[index];

  // Already has component?
  if (data->component_mask & (1ULL << component_id)) {
    return component_get(entity, component_id);
  }

  ComponentPool *pool = &g_world.component_pools[component_id];
  ComponentInfo *info = &g_world.component_infos[component_id];

  // Expand pool if needed
  if (pool->count >= pool->capacity) {
    size_t new_cap = pool->capacity * 2;
    pool->data = realloc(pool->data, new_cap * pool->element_size);
    pool->index_to_entity =
        realloc(pool->index_to_entity, new_cap * sizeof(Entity));
    pool->capacity = new_cap;
  }

  // Add to pool
  uint32_t pool_index = pool->count++;
  pool->entity_to_index[index] = pool_index;
  pool->index_to_entity[pool_index] = entity;
  pool->sparse_set[index] = true;

  void *component = (char *)pool->data + pool_index * pool->element_size;
  memset(component, 0, pool->element_size);

  if (info->constructor) {
    info->constructor(component);
  }

  data->component_mask |= (1ULL << component_id);

  return component;
}

// DONE: Implement component_remove
static void component_remove_internal(u32 entity, ComponentId component_id) {
  if (!entity_is_valid(entity))
    return;
  if (component_id >= g_world.component_count)
    return;

  uint32_t index = entity_get_index(entity);
  EntityData *data = &g_world.entities[index];

  if (!(data->component_mask & (1ULL << component_id)))
    return;

  ComponentPool *pool = &g_world.component_pools[component_id];
  ComponentInfo *info = &g_world.component_infos[component_id];

  uint32_t pool_index = pool->entity_to_index[index];
  void *component = (char *)pool->data + pool_index * pool->element_size;

  if (info->destructor) {
    info->destructor(component);
  }

  // Swap with last element
  if (pool_index < pool->count - 1) {
    void *last = (char *)pool->data + (pool->count - 1) * pool->element_size;
    memcpy(component, last, pool->element_size);

    Entity last_entity = pool->index_to_entity[pool->count - 1];
    pool->index_to_entity[pool_index] = last_entity;
    pool->entity_to_index[entity_get_index(last_entity)] = pool_index;
  }

  pool->count--;
  pool->sparse_set[index] = false;
  pool->entity_to_index[index] = 0xFFFFFFFF;

  data->component_mask &= ~(1ULL << component_id);
}

// DONE: Implement component_get
void *component_get(Entity entity, ComponentId component_id) {
  if (!entity_is_valid(entity))
    return NULL;
  if (component_id >= g_world.component_count)
    return NULL;

  uint32_t index = entity_get_index(entity);
  EntityData *data = &g_world.entities[index];

  if (!(data->component_mask & (1ULL << component_id)))
    return NULL;

  ComponentPool *pool = &g_world.component_pools[component_id];
  uint32_t pool_index = pool->entity_to_index[index];

  return (char *)pool->data + pool_index * pool->element_size;
}

// DONE: Implement component_has
bool component_has(Entity entity, ComponentId component_id) {
  if (!entity_is_valid(entity))
    return false;
  if (component_id >= g_world.component_count)
    return false;

  uint32_t index = entity_get_index(entity);
  return (g_world.entities[index].component_mask & (1ULL << component_id)) != 0;
}

// DONE: Implement component_try_get
void *component_try_get(Entity entity, ComponentId component_id) {
  return component_get(entity, component_id);
}

/* =================================================================================================
 *                                    WORLD IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement world_create
bool world_create(void) {
  if (g_world.initialized)
    return false;

  memset(&g_world, 0, sizeof(ECSWorld));

  g_world.entity_capacity = 1024;
  g_world.entities = calloc(g_world.entity_capacity, sizeof(EntityData));
  g_world.generations = calloc(g_world.entity_capacity, 1);
  g_world.free_indices = calloc(g_world.entity_capacity, sizeof(uint32_t));
  g_world.systems = calloc(ECS_MAX_SYSTEMS, sizeof(void *));

  if (!g_world.entities || !g_world.generations || !g_world.free_indices ||
      !g_world.systems) {
    if (g_world.entities)
      free(g_world.entities);
    if (g_world.generations)
      free(g_world.generations);
    if (g_world.free_indices)
      free(g_world.free_indices);
    if (g_world.systems)
      free(g_world.systems);
    memset(&g_world, 0, sizeof(ECSWorld));
    return false;
  }

  g_world.initialized = true;
  return true;
}

// DONE: Implement world_destroy
void world_destroy(void) {
  if (!g_world.initialized)
    return;

  // Free all entities
  for (uint32_t i = 0; i < g_world.entity_count; i++) {
    free(g_world.entities[i].children);
  }
  free(g_world.entities);
  free(g_world.generations);
  free(g_world.free_indices);

  // Free component pools
  for (uint32_t i = 0; i < g_world.component_count; i++) {
    ComponentPool *pool = &g_world.component_pools[i];
    free(pool->data);
    free(pool->entity_to_index);
    free(pool->index_to_entity);
    free(pool->sparse_set);
  }

  free(g_world.systems);

  memset(&g_world, 0, sizeof(ECSWorld));
}

// DONE: Implement world_get_entity_count
uint32_t world_get_entity_count(void) {
  return g_world.entity_count - g_world.free_count;
}

// DONE: Implement world_find_entity_by_name
Entity world_find_entity_by_name(const char *name) {
  for (uint32_t i = 0; i < g_world.entity_count; i++) {
    if (g_world.entities[i].active &&
        strcmp(g_world.entities[i].name, name) == 0) {
      return entity_make(i, g_world.generations[i]);
    }
  }
  return ECS_INVALID_ENTITY;
}

// DONE: Implement world_clear
void world_clear(void) {
  for (uint32_t i = 0; i < g_world.entity_count; i++) {
    if (g_world.entities[i].active) {
      entity_destroy(entity_make(i, g_world.generations[i]));
    }
  }
}

/* =================================================================================================
 *                                    QUERY IMPLEMENTATION
 * =================================================================================================
 */

typedef struct Query {
  uint64_t required_mask;
  uint64_t excluded_mask;
  Entity *results;
  uint32_t result_count;
  uint32_t result_capacity;
} Query;

// DONE: Implement query_create
static Query *query_create(void) {
  Query *query = calloc(1, sizeof(Query));
  if (!query)
    return NULL;

  query->result_capacity = 256;
  query->results = calloc(query->result_capacity, sizeof(Entity));
  if (!query->results) {
    free(query);
    return NULL;
  }
  return query;
}

// DONE: Implement query_destroy
void query_destroy(Query *query) {
  if (query) {
    free(query->results);
    free(query);
  }
}

// DONE: Implement query_with
void query_with(Query *query, ComponentId component_id) {
  if (query && component_id < 64) {
    query->required_mask |= (1ULL << component_id);
  }
}

// DONE: Implement query_without
void query_without(Query *query, ComponentId component_id) {
  if (query && component_id < 64) {
    query->excluded_mask |= (1ULL << component_id);
  }
}

// DONE: Implement query_execute
void query_execute(Query *query) {
  if (!query)
    return;

  query->result_count = 0;

  for (uint32_t i = 0; i < g_world.entity_count; i++) {
    EntityData *data = &g_world.entities[i];
    if (!data->active)
      continue;

    // Check required components
    if ((data->component_mask & query->required_mask) != query->required_mask) {
      continue;
    }

    // Check excluded components
    if (data->component_mask & query->excluded_mask) {
      continue;
    }

    // Expand results if needed
    if (query->result_count >= query->result_capacity) {
      query->result_capacity *= 2;
      query->results =
          realloc(query->results, query->result_capacity * sizeof(Entity));
    }

    query->results[query->result_count++] =
        entity_make(i, g_world.generations[i]);
  }
}

// DONE: Implement query_count
uint32_t query_count(Query *query) { return query ? query->result_count : 0; }

// DONE: Implement query_first
Entity query_first(Query *query) {
  if (query && query->result_count > 0) {
    return query->results[0];
  }
  return ECS_INVALID_ENTITY;
}

// Rename internal functions to avoid conflicts
#define entity_create entity_create_internal
#define entity_destroy entity_destroy_internal
#define entity_is_valid entity_is_valid_internal
#define component_register component_register_internal
#define component_add component_add_internal
#define component_remove component_remove_internal
#define component_get component_get_internal
#define component_has component_has_internal
#define world_create world_create_internal
#define world_destroy world_destroy_internal
