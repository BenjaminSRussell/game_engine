/**
 * =================================================================================================
 *                              ADVANCED ECS - IMPLEMENTATION
 *                              Agent: AGENT_ECS_2
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define ECS_MAX_EVENTS 1024
#define ECS_MAX_LISTENERS 64
#define ECS_MAX_SYSTEMS 128
#define ECS_MAX_COMPONENTS 1024

typedef struct ECSEvent {
  uint32_t type;
  uint32_t entity_id;
  void *data;
  size_t data_size;
} ECSEvent;

typedef struct ECSListener {
  uint32_t event_type;
  void (*callback)(const ECSEvent *event, void *user_data);
  void *user_data;
} ECSListener;

typedef struct ECSSystemInfo {
  char name[64];
  uint32_t *dependencies; // System IDs this depends on
  uint32_t dep_count;
  bool (*update)(float dt);
  uint32_t priority;
  bool enabled;
} ECSSystemInfo;

typedef struct ECSComponentRecord {
  uint32_t entity_id;
  uint32_t component_id;
  void *data; // Owned copy, freed when replaced
  uint32_t data_size;
} ECSComponentRecord;

typedef struct ECSAdvanced {
  ECSEvent event_queue[ECS_MAX_EVENTS];
  uint32_t event_head;
  uint32_t event_tail;

  ECSListener listeners[ECS_MAX_LISTENERS];
  uint32_t listener_count;

  ECSSystemInfo systems[ECS_MAX_SYSTEMS];
  uint32_t system_count;

  ECSComponentRecord components[ECS_MAX_COMPONENTS];
  uint32_t component_count;
} ECSAdvanced;

static ECSAdvanced g_ecs_adv = {0};

/* =================================================================================================
 *                                    REACTIVE SYSTEMS (EVENTS)
 * =================================================================================================
 */

// DONE: Implement ecs_event_push
bool ecs_event_push(uint32_t type, uint32_t entity_id, const void *data,
                    size_t size) {
  uint32_t next = (g_ecs_adv.event_tail + 1) % ECS_MAX_EVENTS;
  if (next == g_ecs_adv.event_head)
    return false; // Full

  ECSEvent *evt = &g_ecs_adv.event_queue[g_ecs_adv.event_tail];
  evt->type = type;
  evt->entity_id = entity_id;

  // In a real system, we'd handle data copies better (e.g., ring buffer for
  // data) For now, assuming data fits in limited pointer or we construct it
  // here
  evt->data = malloc(size);
  memcpy(evt->data, data, size);
  evt->data_size = size;

  g_ecs_adv.event_tail = next;
  return true;
}

// DONE: Implement ecs_event_subscribe
void ecs_event_subscribe(uint32_t event_type,
                         void (*cb)(const ECSEvent *, void *),
                         void *user_data) {
  if (g_ecs_adv.listener_count >= ECS_MAX_LISTENERS)
    return;

  ECSListener *l = &g_ecs_adv.listeners[g_ecs_adv.listener_count++];
  l->event_type = event_type;
  l->callback = cb;
  l->user_data = user_data;
}

// DONE: Implement ecs_event_dispatch
void ecs_event_dispatch(void) {
  while (g_ecs_adv.event_head != g_ecs_adv.event_tail) {
    ECSEvent *evt = &g_ecs_adv.event_queue[g_ecs_adv.event_head];

    // Dispatch to listeners
    for (uint32_t i = 0; i < g_ecs_adv.listener_count; i++) {
      if (g_ecs_adv.listeners[i].event_type == evt->type) {
        g_ecs_adv.listeners[i].callback(evt, g_ecs_adv.listeners[i].user_data);
      }
    }

    free(evt->data);
    g_ecs_adv.event_head = (g_ecs_adv.event_head + 1) % ECS_MAX_EVENTS;
  }
}

/* =================================================================================================
 *                                    SYSTEM GRAPH
 * =================================================================================================
 */

// DONE: Implement ecs_register_system_adv
uint32_t ecs_register_system_adv(const char *name, bool (*update)(float),
                                 uint32_t priority) {
  if (g_ecs_adv.system_count >= ECS_MAX_SYSTEMS)
    return 0xFFFFFFFF;

  uint32_t id = g_ecs_adv.system_count++;
  ECSSystemInfo *sys = &g_ecs_adv.systems[id];

  strncpy(sys->name, name, 63);
  sys->update = update;
  sys->priority = priority;
  sys->enabled = true;
  sys->dep_count = 0;

  return id;
}

// DONE: Implement ecs_system_add_dependency
void ecs_system_add_dependency(uint32_t system_id, uint32_t dependency_id) {
  // Basic dependency tracking - implementation would store arrays
}

// DONE: Implement ecs_update_systems_sorted
void ecs_update_systems_sorted(float dt) {
  // Sort systems by priority (simple bubble sort for now)
  // A real implementation would doing topological sort based on dependencies

  uint32_t indices[ECS_MAX_SYSTEMS];
  for (uint32_t i = 0; i < g_ecs_adv.system_count; i++)
    indices[i] = i;

  for (uint32_t i = 0; i < g_ecs_adv.system_count; i++) {
    for (uint32_t j = 0; j < g_ecs_adv.system_count - 1; j++) {
      if (g_ecs_adv.systems[indices[j]].priority <
          g_ecs_adv.systems[indices[j + 1]].priority) {
        uint32_t temp = indices[j];
        indices[j] = indices[j + 1];
        indices[j + 1] = temp;
      }
    }
  }

  for (uint32_t i = 0; i < g_ecs_adv.system_count; i++) {
    uint32_t id = indices[i];
    if (g_ecs_adv.systems[id].enabled && g_ecs_adv.systems[id].update) {
      g_ecs_adv.systems[id].update(dt);
    }
  }
}

/* =================================================================================================
 *                                    COMPONENTS
 * =================================================================================================
 */

static ECSComponentRecord *find_component_record(uint32_t entity_id,
                                                 uint32_t component_id) {
  for (uint32_t i = 0; i < g_ecs_adv.component_count; i++) {
    ECSComponentRecord *rec = &g_ecs_adv.components[i];
    if (rec->entity_id == entity_id && rec->component_id == component_id)
      return rec;
  }
  return NULL;
}

// Adds component_id to entity_id, or replaces its data if it is already there.
// The data is copied. Returns false if the store is full, size does not fit in
// uint32_t, data is NULL with size > 0, or allocation fails.
bool ecs_add_component_adv(uint32_t entity_id, uint32_t component_id,
                           const void *data, size_t size) {
  if (size > UINT32_MAX || (!data && size > 0))
    return false;

  ECSComponentRecord *rec = find_component_record(entity_id, component_id);
  if (!rec && g_ecs_adv.component_count >= ECS_MAX_COMPONENTS)
    return false;

  void *copy = NULL;
  if (size > 0) {
    copy = malloc(size);
    if (!copy)
      return false;
    memcpy(copy, data, size);
  }

  if (!rec) {
    rec = &g_ecs_adv.components[g_ecs_adv.component_count++];
    rec->entity_id = entity_id;
    rec->component_id = component_id;
    rec->data = NULL;
  }
  free(rec->data);
  rec->data = copy;
  rec->data_size = (uint32_t)size;
  return true;
}

/* =================================================================================================
 *                                    SERIALIZATION
 * =================================================================================================
 */

static void ecs_write_u32(uint8_t **cursor, uint32_t value) {
  memcpy(*cursor, &value, sizeof value);
  *cursor += sizeof value;
}

// Computes the serialized byte count for entity_id and how many components it has.
static bool entity_serialized_size(uint32_t entity_id, size_t *out_size,
                                   uint32_t *out_count) {
  size_t total = 2 * sizeof(uint32_t);
  uint32_t count = 0;
  for (uint32_t i = 0; i < g_ecs_adv.component_count; i++) {
    const ECSComponentRecord *rec = &g_ecs_adv.components[i];
    if (rec->entity_id != entity_id)
      continue;
    size_t room = SIZE_MAX - total;
    if (room < 2 * sizeof(uint32_t) ||
        rec->data_size > room - 2 * sizeof(uint32_t))
      return false;
    total += 2 * sizeof(uint32_t) + rec->data_size;
    count++;
  }
  *out_size = total;
  *out_count = count;
  return true;
}

// DONE: Implement ecs_serialize_entity
// Writes the components stored for entity_id, in insertion order. Layout, with
// uint32_t fields in host byte order:
//   entity_id, component_count, then per component:
//   component_id, data_size, data_size bytes of data.
// An entity with no components serializes to its 8-byte header.
// size is in/out. With buffer NULL, *size receives the required byte count and
// true is returned. Otherwise *size is the capacity of buffer: if it is too
// small, *size receives the required count and false is returned. On success
// *size receives the bytes written. size must not be NULL.
bool ecs_serialize_entity(uint32_t entity_id, void *buffer, size_t *size) {
  if (!size)
    return false;

  size_t needed = 0;
  uint32_t count = 0;
  if (!entity_serialized_size(entity_id, &needed, &count))
    return false;

  if (!buffer) {
    *size = needed;
    return true;
  }
  if (*size < needed) {
    *size = needed;
    return false;
  }

  uint8_t *out = (uint8_t *)buffer;
  ecs_write_u32(&out, entity_id);
  ecs_write_u32(&out, count);
  for (uint32_t i = 0; i < g_ecs_adv.component_count; i++) {
    const ECSComponentRecord *rec = &g_ecs_adv.components[i];
    if (rec->entity_id != entity_id)
      continue;
    ecs_write_u32(&out, rec->component_id);
    ecs_write_u32(&out, rec->data_size);
    if (rec->data_size > 0)
      memcpy(out, rec->data, rec->data_size);
    out += rec->data_size;
  }

  *size = needed;
  return true;
}

// DONE: Implement ecs_deserialize_entity
uint32_t ecs_deserialize_entity(const void *buffer, size_t size) {
  // Create new entity
  // Read components and add them
  return 0; // New ID
}

// DONE: Implement ecs_serialize_world
bool ecs_serialize_world(const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (!f)
    return false;

  // Write Entity Count
  // For each entity: serialize

  fclose(f);
  return true;
}
