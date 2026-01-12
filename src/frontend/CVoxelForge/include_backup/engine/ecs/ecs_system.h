/**
 * =================================================================================================
 *                              ENTITY COMPONENT SYSTEM
 *                              Agent: AGENT_ECS_1
 * =================================================================================================
 */

#ifndef ECS_SYSTEM_H
#define ECS_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* =================================================================================================
 *                                    ENTITY
 * =================================================================================================
 */

uint32_t entity_create(void);
void entity_destroy(uint32_t entity);
bool entity_is_valid(uint32_t entity);
uint32_t entity_get_generation(uint32_t entity);
void entity_set_enabled(uint32_t entity, bool enabled);
bool entity_is_enabled(uint32_t entity);
void entity_set_name(uint32_t entity, const char *name);
const char *entity_get_name(uint32_t entity);
void entity_set_parent(uint32_t entity, uint32_t parent);
uint32_t entity_get_parent(uint32_t entity);
uint32_t *entity_get_children(uint32_t entity, uint32_t *count);
void entity_add_child(uint32_t entity, uint32_t child);
void entity_remove_child(uint32_t entity, uint32_t child);
uint32_t entity_clone(uint32_t entity);
void entity_serialize(uint32_t entity, void *buffer);
void entity_deserialize(void *buffer);

/* =================================================================================================
 *                                    COMPONENT
 * =================================================================================================
 */

uint32_t component_register(const char *name, size_t size);
void component_unregister(uint32_t id);
const char *component_get_info(uint32_t id);
uint32_t component_get_id(const char *name);
void *component_add(uint32_t entity, uint32_t component_id);
void component_remove(uint32_t entity, uint32_t component_id);
void *component_get(uint32_t entity, uint32_t component_id);
bool component_has(uint32_t entity, uint32_t component_id);
void *component_try_get(uint32_t entity, uint32_t component_id);
void component_set_enabled(uint32_t entity, uint32_t component_id,
                           bool enabled);
void component_copy(uint32_t src_entity, uint32_t dst_entity,
                    uint32_t component_id);
void component_serialize(uint32_t entity, uint32_t component_id, void *buffer);
void component_deserialize(uint32_t entity, uint32_t component_id,
                           void *buffer);

void *component_pool_create(size_t item_size);
void component_pool_destroy(void *pool);
void *component_pool_allocate(void *pool);
void component_pool_free(void *pool, void *ptr);
void *component_pool_get(void *pool, uint32_t index);
void component_pool_iterate(void *pool, void (*callback)(void *));

/* =================================================================================================
 *                                    ARCHETYPE
 * =================================================================================================
 */

void *archetype_create(uint32_t *component_ids, uint32_t count);
void archetype_destroy(void *arch);
void archetype_add_component(void *arch, uint32_t component_id);
void archetype_remove_component(void *arch, uint32_t component_id);
bool archetype_match(void *arch, uint32_t *component_ids, uint32_t count);
void archetype_get_entities(void *arch, uint32_t *out_entities);
void archetype_transfer_entity(void *src_arch, void *dst_arch, uint32_t entity);
void *archetype_chunk_create(size_t size);
void *archetype_chunk_allocate(void *chunk);
void archetype_chunk_free(void *chunk, void *ptr);
void archetype_layout_calculate(void *arch);

/* =================================================================================================
 *                                    QUERY
 * =================================================================================================
 */

void *query_create(void);
void query_destroy(void *query);
void query_with(void *query, uint32_t component_id);
void query_without(void *query, uint32_t component_id);
void query_optional(void *query, uint32_t component_id);
void query_filter(void *query, bool (*filter_func)(uint32_t));
void query_execute(void *query, void (*callback)(uint32_t));
void query_iterate(void *query, void *iterator);
void query_parallel(void *query, void (*callback)(uint32_t), int thread_count);
uint32_t query_first(void *query);
uint32_t query_count(void *query);
void query_cache(void *query);
void query_invalidate(void *query);

/* =================================================================================================
 *                                    SYSTEM
 * =================================================================================================
 */

void system_register(const char *name, void (*update_func)(float));
void system_unregister(const char *name);
void system_set_enabled(const char *name, bool enabled);
bool system_get_enabled(const char *name);
void system_set_priority(const char *name, int priority);
void system_add_dependency(const char *system, const char *dependency);
void system_remove_dependency(const char *system, const char *dependency);
void system_schedule(const char *name);
void system_execute(const char *name, float dt);
void system_parallel_execute(const char *name, int threads);
void system_group_create(const char *group_name);
void system_group_add(const char *group_name, const char *system_name);
void system_group_remove(const char *group_name, const char *system_name);
void system_dependency_graph(void);
void system_topological_sort(void);

/* =================================================================================================
 *                                    WORLD
 * =================================================================================================
 */

void *world_create(void);
void world_destroy(void *world);
void world_update(void *world, float dt);
void world_fixed_update(void *world, float fixed_dt);
void world_late_update(void *world, float dt);
uint32_t world_get_entity_count(void *world);
uint32_t world_find_entity_by_name(void *world, const char *name);
void world_find_entities_with_tag(void *world, const char *tag,
                                  uint32_t *out_entities);
void world_clear(void *world);
void world_serialize(void *world, const char *path);
void world_deserialize(void *world, const char *path);
void world_snapshot(void *world);
void world_restore(void *world);
void world_diff(void *world_a, void *world_b);
void world_merge(void *world_src, void *world_dst);

/* =================================================================================================
 *                                    EVENTS
 * =================================================================================================
 */

void event_register(const char *name);
void event_unregister(const char *name);
void event_emit(const char *name, void *data);
void event_emit_deferred(const char *name, void *data);
void event_subscribe(const char *name, void (*callback)(void *));
void event_unsubscribe(const char *name, void (*callback)(void *));
void event_queue_process(void);
void event_queue_clear(void);
void event_on_component_added(uint32_t entity, uint32_t component);
void event_on_component_removed(uint32_t entity, uint32_t component);
void event_on_entity_created(uint32_t entity);
void event_on_entity_destroyed(uint32_t entity);

#endif // ECS_SYSTEM_H
