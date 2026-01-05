// Entity Component System implementation.
// TODO: Implement ECS archetype-based storage for better cache performance.
// TODO: Add ECS component pooling system to reduce allocations.
// TODO: Implement ECS query system for efficient component filtering.
// TODO: Add ECS event system for component change notifications.
// TODO: Implement ECS serialization system for save/load functionality.
// TODO: Add ECS component validation system to catch invalid states.
// TODO: Implement ECS system dependency graph for execution order.
// TODO: Add ECS multi-threading support for parallel system execution.
// TODO: Implement ECS component versioning for hot-reload support.
// TODO: Add ECS memory layout optimization for cache efficiency.
// TODO: Implement ECS component archetype migration system.
// TODO: Add ECS performance metrics and profiling hooks.
#include <ecs/ecs.h>
#include <stdlib.h>
#include <string.h>

void ecs_world_init(ECSWorld *world, u32 max_entities, u32 max_component_types,
                    u32 max_systems) {
  world->next_entity_id = 1;
  world->entity_capacity = max_entities;

  world->component_type_capacity = max_component_types;
  world->component_type_count = 0;

  world->system_capacity = max_systems;
  world->system_count = 0;
  world->entity_active = (bool *)calloc(max_entities, sizeof(bool));
  world->components =
      (ComponentArray *)calloc(max_component_types, sizeof(ComponentArray));
  world->systems = (System *)malloc(sizeof(System) * max_systems);

  if (!world->entity_active || !world->components || !world->systems) {
    LOG_ERROR("ECS init failed: out of memory");
    free(world->entity_active);
    free(world->components);
    free(world->systems);
    memset(world, 0, sizeof(*world));
    return;
  }
}

void ecs_world_free(ECSWorld *world) {
  // Free component arrays
  for (u32 i = 0; i < world->component_type_count; i++) {
    ComponentArray *array = &world->components[i];
    if (array->data)
      free(array->data);
    if (array->entities)
      free(array->entities);
    if (array->active)
      free(array->active);
  }

  free(world->components);
  free(world->systems);
  free(world->entity_active);
}

EntityID ecs_create_entity(ECSWorld *world) {
  for (EntityID id = 1; id < world->next_entity_id; id++) {
    if (!world->entity_active[id]) {
      world->entity_active[id] = true;
      return id;
    }
  }
  if (world->next_entity_id >= world->entity_capacity) {
    return 0;
  }
  EntityID id = world->next_entity_id++;
  world->entity_active[id] = true;
  return id;
}

void ecs_destroy_entity(ECSWorld *world, EntityID entity) {
  if (!ecs_entity_exists(world, entity)) {
    return;
  }

  // Remove all components
  for (u32 i = 0; i < world->component_type_count; i++) {
    ComponentArray *array = &world->components[i];
    for (u32 j = 0; j < array->count; j++) {
      if (array->entities[j] == entity && array->active[j]) {
        u32 last = array->count - 1;
        if (j != last) {
          void *dst = (u8 *)array->data + (j * array->component_size);
          void *src = (u8 *)array->data + (last * array->component_size);
          memcpy(dst, src, array->component_size);
          array->entities[j] = array->entities[last];
          array->active[j] = array->active[last];
        }
        array->count--;
        break;
      }
    }
  }

  world->entity_active[entity] = false;
}

bool ecs_entity_exists(ECSWorld *world, EntityID entity) {
  return entity < world->entity_capacity && world->entity_active[entity];
}

ComponentTypeID ecs_register_component(ECSWorld *world, ComponentType type) {
  if (world->component_type_count >= world->component_type_capacity) {
    LOG_ERROR("ECS register component failed: capacity exhausted");
    return MAX_COMPONENTS; // Error
  }

  ComponentTypeID id = world->component_type_count++;
  ComponentArray *array = &world->components[id];

  array->type_id = id;
  array->component_size = type.size;
  array->count = 0;
  array->capacity = 0;
  array->data = NULL;
  array->entities = NULL;
  array->active = NULL;

  return id;
}

void *ecs_add_component(ECSWorld *world, EntityID entity,
                        ComponentTypeID type_id) {
  if (type_id >= world->component_type_count) {
    return NULL;
  }

  ComponentArray *array = &world->components[type_id];

  // Check if entity already has this component
  for (u32 i = 0; i < array->count; i++) {
    if (array->entities[i] == entity && array->active[i]) {
      return (u8 *)array->data + (i * array->component_size);
    }
  }

  // Add new component
  if (array->count >= array->capacity) {
    u32 new_capacity = array->capacity == 0 ? 16 : array->capacity * 2;
    void *new_data = malloc(array->component_size * new_capacity);
    EntityID *new_entities =
        (EntityID *)malloc(sizeof(EntityID) * new_capacity);
    bool *new_active = (bool *)malloc(sizeof(bool) * new_capacity);

    if (!new_data || !new_entities || !new_active) {
      free(new_data);
      free(new_entities);
      free(new_active);
      LOG_ERROR("ECS add component failed: out of memory");
      return NULL;
    }

    if (array->count > 0) {
      memcpy(new_data, array->data, array->component_size * array->count);
      memcpy(new_entities, array->entities, sizeof(EntityID) * array->count);
      memcpy(new_active, array->active, sizeof(bool) * array->count);
    }

    free(array->data);
    free(array->entities);
    free(array->active);

    array->data = new_data;
    array->entities = new_entities;
    array->active = new_active;
    array->capacity = new_capacity;
  }

  u32 index = array->count++;
  array->entities[index] = entity;
  array->active[index] = true;

  void *component = (u8 *)array->data + (index * array->component_size);
  memset(component, 0, array->component_size);
  return component;
}

void *ecs_get_component(ECSWorld *world, EntityID entity,
                        ComponentTypeID type_id) {
  if (type_id >= world->component_type_count) {
    return NULL;
  }

  ComponentArray *array = &world->components[type_id];
  for (u32 i = 0; i < array->count; i++) {
    if (array->entities[i] == entity && array->active[i]) {
      return (u8 *)array->data + (i * array->component_size);
    }
  }

  return NULL;
}

bool ecs_has_component(ECSWorld *world, EntityID entity,
                       ComponentTypeID type_id) {
  return ecs_get_component(world, entity, type_id) != NULL;
}

void ecs_remove_component(ECSWorld *world, EntityID entity,
                          ComponentTypeID type_id) {
  if (type_id >= world->component_type_count) {
    return;
  }

  ComponentArray *array = &world->components[type_id];
  for (u32 i = 0; i < array->count; i++) {
    if (array->entities[i] == entity && array->active[i]) {
      array->active[i] = false;
      // Could optimize with swap-and-pop
      break;
    }
  }
}

void ecs_register_system(ECSWorld *world, System system) {
  if (world->system_count >= world->system_capacity) {
    return;
  }

  world->systems[world->system_count++] = system;
}

void ecs_update_systems(ECSWorld *world, f32 delta_time) {
  // Sort systems by priority (simple bubble sort for now)
  for (u32 i = 0; i < world->system_count - 1; i++) {
    for (u32 j = 0; j < world->system_count - i - 1; j++) {
      if (world->systems[j].priority > world->systems[j + 1].priority) {
        System temp = world->systems[j];
        world->systems[j] = world->systems[j + 1];
        world->systems[j + 1] = temp;
      }
    }
  }

  // Run systems
  for (u32 i = 0; i < world->system_count; i++) {
    System *system = &world->systems[i];

    // Find entities with required components
    // Simplified - would need proper query system
    ComponentArray *first_array = NULL;
    ComponentTypeID first_type = 0;
    if (system->component_count > 0) {
      first_type = system->required_components[0];
      if (first_type < world->component_type_count) {
        first_array = &world->components[first_type];
      }
    }

    if (first_array) {
      EntityQuery query;
      ecs_query_init(&query, first_array->count > 0 ? first_array->count : 1);
      ecs_query_entities(world, &query, system->required_components,
                         system->component_count);

      if (query.count > 0 && query.entities) {
        void *packed = malloc(first_array->component_size * query.count);
        if (packed) {
          for (u32 k = 0; k < query.count; k++) {
            void *component =
                ecs_get_component(world, query.entities[k], first_type);
            memcpy((u8 *)packed + (k * first_array->component_size), component,
                   first_array->component_size);
          }
          system->function(packed, query.count, delta_time);
          free(packed);
        } else {
          LOG_ERROR("ECS system update skipped: out of memory");
        }
      }

      ecs_query_free(&query);
    }
  }
}

void ecs_query_init(EntityQuery *query, u32 capacity) {
  query->entities = (EntityID *)malloc(sizeof(EntityID) * capacity);
  query->count = 0;
  query->capacity = capacity;
  if (!query->entities) {
    query->capacity = 0;
    LOG_ERROR("ECS query init failed: out of memory");
  }
}

void ecs_query_free(EntityQuery *query) {
  if (query->entities) {
    free(query->entities);
    query->entities = NULL;
  }
  query->count = 0;
  query->capacity = 0;
}

void ecs_query_entities(ECSWorld *world, EntityQuery *query,
                        ComponentTypeID *components, u32 component_count) {
  query->count = 0;

  if (component_count == 0 || !query->entities || query->capacity == 0) {
    return;
  }

  // Get first component array
  ComponentTypeID first_type = components[0];
  if (first_type >= world->component_type_count) {
    return;
  }

  ComponentArray *first_array = &world->components[first_type];

  // Check each entity in first array
  for (u32 i = 0; i < first_array->count; i++) {
    if (!first_array->active[i])
      continue;

    EntityID entity = first_array->entities[i];
    bool has_all = true;

    // Check if entity has all required components
    for (u32 j = 1; j < component_count; j++) {
      if (!ecs_has_component(world, entity, components[j])) {
        has_all = false;
        break;
      }
    }

    if (has_all) {
      if (query->count >= query->capacity) {
        u32 new_capacity = query->capacity * 2;
        EntityID *new_entities = (EntityID *)realloc(
            query->entities, sizeof(EntityID) * new_capacity);
        if (!new_entities) {
          LOG_ERROR("ECS query grow failed: out of memory");
          return;
        }
        query->entities = new_entities;
        query->capacity = new_capacity;
      }
      query->entities[query->count++] = entity;
    }
  }
}
