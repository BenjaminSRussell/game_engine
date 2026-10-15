/**
 * =================================================================================================
 *                              ECS IMPLEMENTATION
 *                              Agent: AGENT_ECS_1
 * =================================================================================================
 */

#include "ecs/ecs_system.h"
#include <stdlib.h>
#include <string.h>

#define ECS_NAME_MAX 64
#define ECS_MAX_COMPONENTS 32
#define ECS_MAX_SYSTEMS 32
#define ECS_MAX_DEPS 8
#define ECS_MAX_EVENTS 32
#define ECS_MAX_SUBS 8
#define ECS_MAX_DEFERRED 64

/* =================================================================================================
 *                                    INTERNAL STRUCTURES
 * =================================================================================================
 */

typedef struct ComponentType {
  uint32_t id;
  char name[ECS_NAME_MAX];
  size_t size;
  bool in_use;
} ComponentType;

typedef struct Entity {
  uint32_t id;
  uint32_t generation;
  bool alive;
  bool enabled;
  char name[ECS_NAME_MAX];
  uint32_t parent_id;
  uint32_t *children;
  uint32_t child_count;
  uint32_t child_capacity;
  void *components[ECS_MAX_COMPONENTS];
  bool component_enabled[ECS_MAX_COMPONENTS];
} Entity;

typedef struct EcsSystem {
  char name[ECS_NAME_MAX];
  void (*update)(float);
  bool in_use;
  bool enabled;
  int priority;
  char deps[ECS_MAX_DEPS][ECS_NAME_MAX];
  int dep_count;
} EcsSystem;

typedef struct EcsEvent {
  char name[ECS_NAME_MAX];
  bool in_use;
  void (*subs[ECS_MAX_SUBS])(void *);
  int sub_count;
} EcsEvent;

typedef struct DeferredEvent {
  int event_index;
  void *data;
} DeferredEvent;

static Entity *g_entities = NULL;
static uint32_t g_entity_count = 0;
static uint32_t g_entity_capacity = 0;
static ComponentType g_components[ECS_MAX_COMPONENTS];
static uint32_t g_component_count = 0;
static EcsSystem g_systems[ECS_MAX_SYSTEMS];
static EcsEvent g_events[ECS_MAX_EVENTS];
static DeferredEvent g_deferred[ECS_MAX_DEFERRED];
static int g_deferred_count = 0;

static void copy_name(char *dst, const char *src) {
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, ECS_NAME_MAX - 1);
  dst[ECS_NAME_MAX - 1] = '\0';
}

static Entity *entity_slot(uint32_t entity) {
  if (entity == 0 || entity > g_entity_count || !g_entities)
    return NULL;
  return &g_entities[entity - 1];
}

static bool entity_alive(uint32_t entity) {
  Entity *e = entity_slot(entity);
  return e && e->alive;
}

static void unlink_from_parent_list(uint32_t parent, uint32_t child) {
  Entity *p = entity_slot(parent);
  uint32_t i;
  uint32_t w = 0;
  if (!p || !p->children)
    return;
  for (i = 0; i < p->child_count; i++) {
    if (p->children[i] != child)
      p->children[w++] = p->children[i];
  }
  p->child_count = w;
}

static bool append_child(Entity *parent, uint32_t child) {
  uint32_t i;
  uint32_t cap;
  uint32_t *next;
  for (i = 0; i < parent->child_count; i++) {
    if (parent->children[i] == child)
      return true;
  }
  if (parent->child_count >= parent->child_capacity) {
    cap = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
    next = realloc(parent->children, cap * sizeof(uint32_t));
    if (!next)
      return false;
    parent->children = next;
    parent->child_capacity = cap;
  }
  parent->children[parent->child_count++] = child;
  return true;
}

static void free_entity_memory(Entity *e) {
  uint32_t c;
  free(e->children);
  e->children = NULL;
  e->child_count = 0;
  e->child_capacity = 0;
  for (c = 0; c < ECS_MAX_COMPONENTS; c++) {
    free(e->components[c]);
    e->components[c] = NULL;
    e->component_enabled[c] = false;
  }
}

static ComponentType *component_type(uint32_t id) {
  if (id == 0 || id > g_component_count || id > ECS_MAX_COMPONENTS)
    return NULL;
  if (!g_components[id - 1].in_use)
    return NULL;
  return &g_components[id - 1];
}

static int find_system(const char *name) {
  int i;
  if (!name)
    return -1;
  for (i = 0; i < ECS_MAX_SYSTEMS; i++) {
    if (g_systems[i].in_use && strcmp(g_systems[i].name, name) == 0)
      return i;
  }
  return -1;
}

static bool deps_satisfied(int idx, const bool *done) {
  int d;
  for (d = 0; d < g_systems[idx].dep_count; d++) {
    int dep = find_system(g_systems[idx].deps[d]);
    if (dep < 0 || !g_systems[dep].in_use || !g_systems[dep].enabled)
      continue;
    if (!done[dep])
      return false;
  }
  return true;
}

static int find_event(const char *name) {
  int i;
  if (!name)
    return -1;
  for (i = 0; i < ECS_MAX_EVENTS; i++) {
    if (g_events[i].in_use && strcmp(g_events[i].name, name) == 0)
      return i;
  }
  return -1;
}

static void event_emit_index(int index, void *data) {
  void (*subs[ECS_MAX_SUBS])(void *);
  int n;
  int s;
  if (index < 0 || index >= ECS_MAX_EVENTS || !g_events[index].in_use)
    return;
  n = g_events[index].sub_count;
  memcpy(subs, g_events[index].subs, sizeof(subs));
  for (s = 0; s < n; s++) {
    if (subs[s])
      subs[s](data);
  }
}

/* =================================================================================================
 *                                    ENTITY SYSTEM
 * =================================================================================================
 */

uint32_t entity_create(void) {
  Entity *e;
  uint32_t id;
  if (g_entity_count >= g_entity_capacity) {
    uint32_t cap = g_entity_capacity == 0 ? 1024 : g_entity_capacity * 2;
    Entity *next = realloc(g_entities, cap * sizeof(Entity));
    if (!next)
      return 0;
    memset(next + g_entity_count, 0, (cap - g_entity_count) * sizeof(Entity));
    g_entities = next;
    g_entity_capacity = cap;
  }
  id = ++g_entity_count;
  e = &g_entities[id - 1];
  memset(e, 0, sizeof(*e));
  e->id = id;
  e->generation = 1;
  e->enabled = true;
  e->alive = true;
  return id;
}

void entity_destroy(uint32_t entity) {
  Entity *e;
  uint32_t i;
  if (!entity_alive(entity))
    return;
  e = &g_entities[entity - 1];
  if (e->parent_id)
    unlink_from_parent_list(e->parent_id, entity);
  for (i = 0; i < e->child_count; i++) {
    Entity *child = entity_slot(e->children[i]);
    if (child && child->parent_id == entity)
      child->parent_id = 0;
  }
  free_entity_memory(e);
  e->enabled = false;
  e->alive = false;
  e->generation++;
  e->name[0] = '\0';
  e->parent_id = 0;
}

bool entity_is_valid(uint32_t entity) { return entity_alive(entity); }

uint32_t entity_get_generation(uint32_t entity) {
  Entity *e = entity_slot(entity);
  return e ? e->generation : 0;
}

void entity_set_enabled(uint32_t entity, bool enabled) {
  Entity *e = entity_slot(entity);
  if (!e || !e->alive)
    return;
  e->enabled = enabled;
}

bool entity_is_enabled(uint32_t entity) {
  Entity *e = entity_slot(entity);
  return e && e->alive && e->enabled;
}

void entity_set_name(uint32_t entity, const char *name) {
  Entity *e = entity_slot(entity);
  if (!e || !e->alive || !name)
    return;
  copy_name(e->name, name);
}

const char *entity_get_name(uint32_t entity) {
  Entity *e = entity_slot(entity);
  if (!e || !e->alive)
    return "";
  return e->name;
}

void entity_set_parent(uint32_t entity, uint32_t parent) {
  Entity *e;
  if (!entity_alive(entity))
    return;
  e = &g_entities[entity - 1];
  if (parent == 0) {
    if (e->parent_id)
      unlink_from_parent_list(e->parent_id, entity);
    e->parent_id = 0;
    return;
  }
  entity_add_child(parent, entity);
}

uint32_t entity_get_parent(uint32_t entity) {
  Entity *e = entity_slot(entity);
  if (!e || !e->alive)
    return 0;
  return e->parent_id;
}

uint32_t *entity_get_children(uint32_t entity, uint32_t *count) {
  Entity *e = entity_slot(entity);
  if (!e || !e->alive) {
    if (count)
      *count = 0;
    return NULL;
  }
  if (count)
    *count = e->child_count;
  return e->children;
}

void entity_add_child(uint32_t entity, uint32_t child) {
  Entity *c;
  uint32_t old_parent;
  if (!entity_alive(entity) || !entity_alive(child) || entity == child)
    return;
  c = &g_entities[child - 1];
  if (c->parent_id == entity) {
    append_child(&g_entities[entity - 1], child);
    return;
  }
  old_parent = c->parent_id;
  if (old_parent)
    unlink_from_parent_list(old_parent, child);
  if (!append_child(&g_entities[entity - 1], child)) {
    c->parent_id = 0;
    return;
  }
  c->parent_id = entity;
}

void entity_remove_child(uint32_t entity, uint32_t child) {
  Entity *c;
  if (!entity_slot(entity))
    return;
  unlink_from_parent_list(entity, child);
  c = entity_slot(child);
  if (c && c->parent_id == entity)
    c->parent_id = 0;
}

uint32_t entity_clone(uint32_t entity) {
  Entity *src;
  Entity *dst;
  uint32_t id;
  uint32_t c;
  if (!entity_alive(entity))
    return 0;
  id = entity_create();
  if (id == 0)
    return 0;
  src = &g_entities[entity - 1];
  dst = &g_entities[id - 1];
  memcpy(dst->name, src->name, sizeof(dst->name));
  dst->enabled = src->enabled;
  for (c = 0; c < g_component_count && c < ECS_MAX_COMPONENTS; c++) {
    void *copy;
    if (!g_components[c].in_use || !src->components[c])
      continue;
    copy = malloc(g_components[c].size);
    if (!copy)
      continue;
    memcpy(copy, src->components[c], g_components[c].size);
    dst->components[c] = copy;
    dst->component_enabled[c] = src->component_enabled[c];
  }
  return id;
}

void entity_serialize(uint32_t entity, void *buffer) {
  (void)entity;
  (void)buffer;
}

void entity_deserialize(void *buffer) { (void)buffer; }

/* =================================================================================================
 *                                    COMPONENT SYSTEM
 * =================================================================================================
 */

uint32_t component_register(const char *name, size_t size) {
  ComponentType *t;
  uint32_t existing;
  if (!name || !name[0] || size == 0)
    return 0;
  existing = component_get_id(name);
  if (existing)
    return existing;
  if (g_component_count >= ECS_MAX_COMPONENTS)
    return 0;
  t = &g_components[g_component_count];
  memset(t, 0, sizeof(*t));
  t->id = g_component_count + 1;
  t->size = size;
  copy_name(t->name, name);
  t->in_use = true;
  g_component_count++;
  return t->id;
}

void component_unregister(uint32_t id) {
  uint32_t i;
  ComponentType *t = component_type(id);
  if (!t)
    return;
  if (g_entities) {
    for (i = 0; i < g_entity_count; i++) {
      free(g_entities[i].components[id - 1]);
      g_entities[i].components[id - 1] = NULL;
      g_entities[i].component_enabled[id - 1] = false;
    }
  }
  t->in_use = false;
  t->name[0] = '\0';
}

const char *component_get_info(uint32_t id) {
  ComponentType *t = component_type(id);
  return t ? t->name : "";
}

uint32_t component_get_id(const char *name) {
  uint32_t i;
  if (!name)
    return 0;
  for (i = 0; i < g_component_count; i++) {
    if (g_components[i].in_use && strcmp(g_components[i].name, name) == 0)
      return g_components[i].id;
  }
  return 0;
}

void *component_add(uint32_t entity, uint32_t component_id) {
  ComponentType *t = component_type(component_id);
  Entity *e;
  void *data;
  uint32_t idx;
  if (!t || !entity_alive(entity))
    return NULL;
  e = &g_entities[entity - 1];
  idx = component_id - 1;
  if (e->components[idx]) {
    e->component_enabled[idx] = true;
    return e->components[idx];
  }
  data = calloc(1, t->size);
  if (!data)
    return NULL;
  e->components[idx] = data;
  e->component_enabled[idx] = true;
  return data;
}

void component_remove(uint32_t entity, uint32_t component_id) {
  Entity *e = entity_slot(entity);
  if (!e || component_id == 0 || component_id > ECS_MAX_COMPONENTS)
    return;
  free(e->components[component_id - 1]);
  e->components[component_id - 1] = NULL;
  e->component_enabled[component_id - 1] = false;
}

void *component_try_get(uint32_t entity, uint32_t component_id) {
  Entity *e;
  if (!component_type(component_id) || !entity_alive(entity))
    return NULL;
  e = &g_entities[entity - 1];
  return e->components[component_id - 1];
}

void *component_get(uint32_t entity, uint32_t component_id) {
  Entity *e;
  void *data = component_try_get(entity, component_id);
  if (!data)
    return NULL;
  e = &g_entities[entity - 1];
  if (!e->component_enabled[component_id - 1])
    return NULL;
  return data;
}

bool component_has(uint32_t entity, uint32_t component_id) {
  return component_try_get(entity, component_id) != NULL;
}

void component_set_enabled(uint32_t entity, uint32_t component_id, bool enabled) {
  Entity *e;
  if (!component_try_get(entity, component_id))
    return;
  e = &g_entities[entity - 1];
  e->component_enabled[component_id - 1] = enabled;
}

void component_copy(uint32_t src_entity, uint32_t dst_entity, uint32_t component_id) {
  ComponentType *t = component_type(component_id);
  void *src = component_try_get(src_entity, component_id);
  void *dst;
  bool enabled;
  if (!t || !src || src_entity == dst_entity || !entity_alive(dst_entity))
    return;
  enabled = g_entities[src_entity - 1].component_enabled[component_id - 1];
  dst = component_add(dst_entity, component_id);
  if (!dst)
    return;
  memcpy(dst, src, t->size);
  g_entities[dst_entity - 1].component_enabled[component_id - 1] = enabled;
}

void component_serialize(uint32_t entity, uint32_t component_id, void *buffer) {
  (void)entity;
  (void)component_id;
  (void)buffer;
}

void component_deserialize(uint32_t entity, uint32_t component_id, void *buffer) {
  (void)entity;
  (void)component_id;
  (void)buffer;
}

void *component_pool_create(size_t item_size) {
  (void)item_size;
  return malloc(1024);
}

void component_pool_destroy(void *pool) { free(pool); }

void *component_pool_allocate(void *pool) {
  (void)pool;
  return malloc(64);
}

void component_pool_free(void *pool, void *ptr) {
  (void)pool;
  free(ptr);
}

void *component_pool_get(void *pool, uint32_t index) {
  (void)pool;
  (void)index;
  return NULL;
}

void component_pool_iterate(void *pool, void (*callback)(void *)) {
  (void)pool;
  (void)callback;
}

/* =================================================================================================
 *                                    ARCHETYPE SYSTEM
 * =================================================================================================
 */

void *archetype_create(uint32_t *component_ids, uint32_t count) {
  (void)component_ids;
  (void)count;
  return malloc(64);
}

void archetype_destroy(void *arch) { free(arch); }

void archetype_add_component(void *arch, uint32_t component_id) {
  (void)arch;
  (void)component_id;
}

void archetype_remove_component(void *arch, uint32_t component_id) {
  (void)arch;
  (void)component_id;
}

bool archetype_match(void *arch, uint32_t *component_ids, uint32_t count) {
  (void)arch;
  (void)component_ids;
  (void)count;
  return false;
}

void archetype_get_entities(void *arch, uint32_t *out_entities) {
  (void)arch;
  (void)out_entities;
}

void archetype_transfer_entity(void *src_arch, void *dst_arch, uint32_t entity) {
  (void)src_arch;
  (void)dst_arch;
  (void)entity;
}

void *archetype_chunk_create(size_t size) { return malloc(size); }

void *archetype_chunk_allocate(void *chunk) {
  (void)chunk;
  return NULL;
}

void archetype_chunk_free(void *chunk, void *ptr) {
  (void)chunk;
  (void)ptr;
}

void archetype_layout_calculate(void *arch) { (void)arch; }

/* =================================================================================================
 *                                    QUERY SYSTEM
 * =================================================================================================
 */

void *query_create(void) { return malloc(64); }

void query_destroy(void *query) { free(query); }

void query_with(void *query, uint32_t component_id) {
  (void)query;
  (void)component_id;
}

void query_without(void *query, uint32_t component_id) {
  (void)query;
  (void)component_id;
}

void query_optional(void *query, uint32_t component_id) {
  (void)query;
  (void)component_id;
}

void query_filter(void *query, bool (*filter_func)(uint32_t)) {
  (void)query;
  (void)filter_func;
}

void query_execute(void *query, void (*callback)(uint32_t)) {
  (void)query;
  (void)callback;
}

void query_iterate(void *query, void *iterator) {
  (void)query;
  (void)iterator;
}

void query_parallel(void *query, void (*callback)(uint32_t), int thread_count) {
  (void)query;
  (void)callback;
  (void)thread_count;
}

uint32_t query_first(void *query) {
  (void)query;
  return 0;
}

uint32_t query_count(void *query) {
  (void)query;
  return 0;
}

void query_cache(void *query) { (void)query; }

void query_invalidate(void *query) { (void)query; }

/* =================================================================================================
 *                                    SYSTEM
 * =================================================================================================
 */

void system_register(const char *name, void (*update_func)(float)) {
  int existing;
  int i;
  if (!name || !name[0] || !update_func)
    return;
  existing = find_system(name);
  if (existing >= 0) {
    g_systems[existing].update = update_func;
    return;
  }
  for (i = 0; i < ECS_MAX_SYSTEMS; i++) {
    if (g_systems[i].in_use)
      continue;
    memset(&g_systems[i], 0, sizeof(g_systems[i]));
    copy_name(g_systems[i].name, name);
    g_systems[i].update = update_func;
    g_systems[i].enabled = true;
    g_systems[i].in_use = true;
    return;
  }
}

void system_unregister(const char *name) {
  int i = find_system(name);
  if (i < 0)
    return;
  memset(&g_systems[i], 0, sizeof(g_systems[i]));
}

void system_set_enabled(const char *name, bool enabled) {
  int i = find_system(name);
  if (i < 0)
    return;
  g_systems[i].enabled = enabled;
}

bool system_get_enabled(const char *name) {
  int i = find_system(name);
  return i >= 0 && g_systems[i].enabled;
}

void system_set_priority(const char *name, int priority) {
  int i = find_system(name);
  if (i < 0)
    return;
  g_systems[i].priority = priority;
}

void system_add_dependency(const char *system, const char *dependency) {
  int i;
  int d;
  if (!system || !dependency || !dependency[0] || strcmp(system, dependency) == 0)
    return;
  i = find_system(system);
  if (i < 0)
    return;
  for (d = 0; d < g_systems[i].dep_count; d++) {
    if (strcmp(g_systems[i].deps[d], dependency) == 0)
      return;
  }
  if (g_systems[i].dep_count >= ECS_MAX_DEPS)
    return;
  copy_name(g_systems[i].deps[g_systems[i].dep_count], dependency);
  g_systems[i].dep_count++;
}

void system_remove_dependency(const char *system, const char *dependency) {
  int i;
  int d;
  int w = 0;
  if (!dependency)
    return;
  i = find_system(system);
  if (i < 0)
    return;
  for (d = 0; d < g_systems[i].dep_count; d++) {
    if (strcmp(g_systems[i].deps[d], dependency) == 0)
      continue;
    if (w != d)
      memcpy(g_systems[i].deps[w], g_systems[i].deps[d], ECS_NAME_MAX);
    w++;
  }
  g_systems[i].dep_count = w;
}

void system_schedule(const char *name) { (void)name; }

void system_execute(const char *name, float dt) {
  int i = find_system(name);
  if (i < 0 || !g_systems[i].enabled || !g_systems[i].update)
    return;
  g_systems[i].update(dt);
}

void system_parallel_execute(const char *name, int threads) {
  (void)name;
  (void)threads;
}

void system_group_create(const char *group_name) { (void)group_name; }

void system_group_add(const char *group_name, const char *system_name) {
  (void)group_name;
  (void)system_name;
}

void system_group_remove(const char *group_name, const char *system_name) {
  (void)group_name;
  (void)system_name;
}

void system_dependency_graph(void) {}

void system_topological_sort(void) {}

/* =================================================================================================
 *                                    WORLD
 * =================================================================================================
 */

void *world_create(void) { return calloc(1, sizeof(int)); }

void world_destroy(void *world) { free(world); }

void world_update(void *world, float dt) {
  bool done[ECS_MAX_SYSTEMS];
  if (!world)
    return;
  memset(done, 0, sizeof(done));
  for (;;) {
    int best = -1;
    int i;
    for (i = 0; i < ECS_MAX_SYSTEMS; i++) {
      if (!g_systems[i].in_use || !g_systems[i].enabled || done[i])
        continue;
      if (!deps_satisfied(i, done))
        continue;
      if (best < 0 || g_systems[i].priority > g_systems[best].priority)
        best = i;
    }
    if (best < 0)
      break;
    done[best] = true;
    if (g_systems[best].update)
      g_systems[best].update(dt);
  }
}

void world_fixed_update(void *world, float fixed_dt) {
  (void)world;
  (void)fixed_dt;
}

void world_late_update(void *world, float dt) {
  (void)world;
  (void)dt;
}

uint32_t world_get_entity_count(void *world) {
  uint32_t i;
  uint32_t n = 0;
  if (!world)
    return 0;
  for (i = 0; i < g_entity_count; i++) {
    if (g_entities[i].alive)
      n++;
  }
  return n;
}

uint32_t world_find_entity_by_name(void *world, const char *name) {
  uint32_t i;
  if (!world || !name)
    return 0;
  for (i = 0; i < g_entity_count; i++) {
    if (g_entities[i].alive && strcmp(g_entities[i].name, name) == 0)
      return g_entities[i].id;
  }
  return 0;
}

void world_find_entities_with_tag(void *world, const char *tag, uint32_t *out_entities) {
  (void)world;
  (void)tag;
  (void)out_entities;
}

void world_clear(void *world) {
  uint32_t i;
  if (!world || !g_entities)
    return;
  for (i = 0; i < g_entity_capacity; i++)
    free_entity_memory(&g_entities[i]);
  memset(g_entities, 0, g_entity_capacity * sizeof(Entity));
  g_entity_count = 0;
}

void world_serialize(void *world, const char *path) {
  (void)world;
  (void)path;
}

void world_deserialize(void *world, const char *path) {
  (void)world;
  (void)path;
}

void world_snapshot(void *world) { (void)world; }

void world_restore(void *world) { (void)world; }

void world_diff(void *world_a, void *world_b) {
  (void)world_a;
  (void)world_b;
}

void world_merge(void *world_src, void *world_dst) {
  (void)world_src;
  (void)world_dst;
}

/* =================================================================================================
 *                                    EVENTS
 * =================================================================================================
 */

void event_register(const char *name) {
  int i;
  if (!name || !name[0] || find_event(name) >= 0)
    return;
  for (i = 0; i < ECS_MAX_EVENTS; i++) {
    if (g_events[i].in_use)
      continue;
    memset(&g_events[i], 0, sizeof(g_events[i]));
    copy_name(g_events[i].name, name);
    g_events[i].in_use = true;
    return;
  }
}

void event_unregister(const char *name) {
  int i = find_event(name);
  if (i < 0)
    return;
  memset(&g_events[i], 0, sizeof(g_events[i]));
}

void event_emit(const char *name, void *data) { event_emit_index(find_event(name), data); }

void event_emit_deferred(const char *name, void *data) {
  int index = find_event(name);
  if (index < 0 || g_deferred_count >= ECS_MAX_DEFERRED)
    return;
  g_deferred[g_deferred_count].event_index = index;
  g_deferred[g_deferred_count].data = data;
  g_deferred_count++;
}

void event_subscribe(const char *name, void (*callback)(void *)) {
  int i;
  int s;
  if (!callback)
    return;
  i = find_event(name);
  if (i < 0)
    return;
  for (s = 0; s < g_events[i].sub_count; s++) {
    if (g_events[i].subs[s] == callback)
      return;
  }
  if (g_events[i].sub_count >= ECS_MAX_SUBS)
    return;
  g_events[i].subs[g_events[i].sub_count++] = callback;
}

void event_unsubscribe(const char *name, void (*callback)(void *)) {
  int i;
  int s;
  int w = 0;
  i = find_event(name);
  if (i < 0)
    return;
  for (s = 0; s < g_events[i].sub_count; s++) {
    if (g_events[i].subs[s] == callback)
      continue;
    g_events[i].subs[w++] = g_events[i].subs[s];
  }
  g_events[i].sub_count = w;
}

void event_queue_process(void) {
  DeferredEvent batch[ECS_MAX_DEFERRED];
  int n = g_deferred_count;
  int q;
  if (n > ECS_MAX_DEFERRED)
    n = ECS_MAX_DEFERRED;
  memcpy(batch, g_deferred, (size_t)n * sizeof(DeferredEvent));
  g_deferred_count = 0;
  for (q = 0; q < n; q++)
    event_emit_index(batch[q].event_index, batch[q].data);
}

void event_queue_clear(void) { g_deferred_count = 0; }

void event_on_component_added(uint32_t entity, uint32_t component) {
  (void)entity;
  (void)component;
}

void event_on_component_removed(uint32_t entity, uint32_t component) {
  (void)entity;
  (void)component;
}

void event_on_entity_created(uint32_t entity) { (void)entity; }

void event_on_entity_destroyed(uint32_t entity) { (void)entity; }
