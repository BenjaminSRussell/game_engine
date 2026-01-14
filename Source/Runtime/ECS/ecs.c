/**
 * ECS Implementation - Archetype-Based Entity Component System
 * Matches ecs.h API with safe internal state management
 */

#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/ecs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Internal Data Structures
// ============================================================================

// Entity Record
typedef struct {
  bool active;
  void *archetype_ptr;
  u32 chunk_idx;
  u32 index_in_chunk;
  u8 generation;
} EntityRecord;

// Entity Manager (Stored in world->entities)
typedef struct {
  EntityRecord *records;
  u32 next_entity_id;
  u32 *free_entity_ids;
  u32 free_count;
  u32 capacity;
} EntityData;

// Component info (Stored in world->component_info)
typedef struct {
  ComponentInfo *infos;
  u32 count;
  u32 capacity;
} ComponentDataInternal;

// Archetypes (Stored in world->archetypes)
typedef struct SimpleArchetype {
  ComponentType component_types[32]; // Hardcoded max for now
  u32 component_count;
  u32 entity_capacity;
  u32 entity_count;
  Entity *entities;
  void **component_data;
  struct SimpleArchetype *next;
} SimpleArchetype;

typedef struct {
  SimpleArchetype *head;
  SimpleArchetype *empty_archetype;
  u32 count;
} ArchetypeData;

// Global world pointer for singleton access if needed
World *g_ecs_world = NULL;

// ============================================================================
// Implementation
// ============================================================================

void ecs_world_init(World *world, u32 max_entities, u32 max_components,
                    u32 max_systems) {
  if (!world)
    return;

  // Initialize Config
  world->config.max_entities = max_entities;
  world->config.max_components = max_components;
  world->config.max_systems = max_systems;

  // Initialize Entity Data
  EntityData *ed = calloc(1, sizeof(EntityData));
  ed->capacity = max_entities;
  ed->records = calloc(max_entities, sizeof(EntityRecord));
  ed->free_entity_ids = calloc(max_entities, sizeof(u32));
  ed->next_entity_id = 1; // 0 is invalid
  world->entities = ed;

  // Initialize Component Data
  ComponentDataInternal *cd = calloc(1, sizeof(ComponentDataInternal));
  cd->capacity = max_components;
  cd->infos = calloc(max_components, sizeof(ComponentInfo));
  world->component_info = cd;

  // Initialize Archetype Data
  ArchetypeData *ad = calloc(1, sizeof(ArchetypeData));
  world->archetypes = ad;

  // CRITICAL: Create empty archetype (archetype 0) for new entities
  // All entities start with no components in this archetype
  SimpleArchetype *empty = calloc(1, sizeof(SimpleArchetype));
  empty->component_count = 0;
  memset(empty->component_types, 0, sizeof(empty->component_types));
  empty->component_data = NULL;
  empty->entity_capacity = 16; // Start small
  empty->entity_count = 0;
  empty->entities = calloc(16, sizeof(Entity));
  empty->next = NULL;
  ad->head = empty;
  ad->empty_archetype = empty;
  ad->count = 1;

  // Initialize counts
  world->entity_count = 0;
  world->component_count = 0;
  world->system_count = 0;

  g_ecs_world = world;

  LOG_INFO("[ECS] World Initialized (Capacity: %d entities)", max_entities);
}

void ecs_world_free(World *world) {
  if (!world)
    return;

  // Free Entity Data
  if (world->entities) {
    EntityData *ed = (EntityData *)world->entities;
    if (ed->records)
      free(ed->records);
    if (ed->free_entity_ids)
      free(ed->free_entity_ids);
    free(ed);
    world->entities = NULL;
  }

  // Free Component Data
  if (world->component_info) {
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    if (cd->infos)
      free(cd->infos);
    free(cd);
    world->component_info = NULL;
  }

  // Free Archetypes (Deep free needed)
  if (world->archetypes) {
    ArchetypeData *ad = (ArchetypeData *)world->archetypes;
    SimpleArchetype *curr = ad->head;
    while (curr) {
      SimpleArchetype *next = curr->next;
      if (curr->entities)
        free(curr->entities);
      if (curr->component_data) {
        for (u32 i = 0; i < curr->component_count; i++) {
          if (curr->component_data[i])
            free(curr->component_data[i]);
        }
        free(curr->component_data);
      }
      free(curr);
      curr = next;
    }
    free(ad);
    world->archetypes = NULL;
  }

  if (g_ecs_world == world) {
    g_ecs_world = NULL;
  }
}

// ============================================================================
// Entity Functions
// ============================================================================

Entity ecs_create_entity(World *world) {
  if (!world || !world->entities)
    return (Entity){0, 0};

  EntityData *ed = (EntityData *)world->entities;
  u32 id;

  if (ed->free_count > 0) {
    id = ed->free_entity_ids[--ed->free_count];
  } else {
    if (ed->next_entity_id >= ed->capacity) {
      LOG_ERROR("[ECS] Error: Max entities reached");
      return (Entity){0, 0};
    }
    id = ed->next_entity_id++;
  }

  // Init record
  EntityRecord *rec = &ed->records[id];
  rec->active = true;
  rec->generation++;

  // Add entity to empty archetype (archetype 0)
  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *empty_arch = ad->empty_archetype;

  if (empty_arch) {
    // Ensure capacity
    if (empty_arch->entity_count >= empty_arch->entity_capacity) {
      u32 new_cap = empty_arch->entity_capacity * 2;
      Entity *new_entities = calloc(new_cap, sizeof(Entity));
      memcpy(new_entities, empty_arch->entities,
             empty_arch->entity_capacity * sizeof(Entity));
      free(empty_arch->entities);
      empty_arch->entities = new_entities;
      empty_arch->entity_capacity = new_cap;
    }

    rec->archetype_ptr = empty_arch;
    rec->index_in_chunk = empty_arch->entity_count;
    empty_arch->entities[empty_arch->entity_count++] =
        (Entity){id, rec->generation};
  }

  world->entity_count++;

  return (Entity){id, rec->generation};
}

void ecs_destroy_entity(World *world, Entity entity) {
  if (!world || !world->entities)
    return;
  EntityData *ed = (EntityData *)world->entities;

  if (entity.id == 0 || entity.id >= ed->capacity)
    return;

  EntityRecord *rec = &ed->records[entity.id];
  if (!rec->active || rec->generation != entity.generation)
    return;

  rec->active = false;
  // Add to free list
  ed->free_entity_ids[ed->free_count++] = entity.id;
  world->entity_count--;
}

Entity ecs_get_entity_by_id(World *world, u32 id) {
  if (!world || !world->entities)
    return (Entity){0, 0};
  EntityData *ed = (EntityData *)world->entities;
  if (id == 0 || id >= ed->capacity)
    return (Entity){0, 0};
  return (Entity){id, ed->records[id].generation};
}

u32 ecs_get_max_entity_id(World *world) {
  if (!world || !world->entities)
    return 0;
  EntityData *ed = (EntityData *)world->entities;
  return ed->next_entity_id;
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

static void *get_component_array(SimpleArchetype *arch, ECSComponentID type) {
  for (u32 i = 0; i < arch->component_count; i++) {
    if (arch->component_types[i] == type) {
      return arch->component_data[i];
    }
  }
  return NULL;
}

static SimpleArchetype *
find_or_create_archetype(World *world, ComponentType *types, u32 count) {
  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *curr = ad->head;

  // Search for existing archetype
  while (curr) {
    if (curr->component_count == count) {
      bool match = true;
      for (u32 i = 0; i < count; i++) {
        bool found = false;
        for (u32 j = 0; j < curr->component_count; j++) {
          if (curr->component_types[j] == types[i]) {
            found = true;
            break;
          }
        }
        if (!found) {
          match = false;
          break;
        }
      }
      if (match)
        return curr;
    }
    curr = curr->next;
  }

  // Create new archetype
  SimpleArchetype *new_arch = calloc(1, sizeof(SimpleArchetype));
  new_arch->component_count = count;
  memcpy(new_arch->component_types, types, count * sizeof(ComponentType));

  // Initialize storage
  new_arch->entity_capacity = 64; // Start small
  new_arch->entities = malloc(new_arch->entity_capacity * sizeof(Entity));
  new_arch->component_data = malloc(count * sizeof(void *));

  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  for (u32 i = 0; i < count; i++) {
    ECSComponentID type = types[i];
    u32 size = cd->infos[type].size;
    new_arch->component_data[i] = calloc(new_arch->entity_capacity, size);
  }

  // Link
  new_arch->next = ad->head;
  ad->head = new_arch;
  ad->count++;

  return new_arch;
}

static void move_entity(World *world, EntityRecord *rec,
                        SimpleArchetype *old_arch, SimpleArchetype *new_arch) {
  u32 old_index = rec->index_in_chunk;
  u32 new_index = new_arch->entity_count++;

  // Resize new archetype if needed
  if (new_index >= new_arch->entity_capacity) {
    u32 new_cap = new_arch->entity_capacity * 2;
    new_arch->entities = realloc(new_arch->entities, new_cap * sizeof(Entity));

    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    for (u32 i = 0; i < new_arch->component_count; i++) {
      u32 size = cd->infos[new_arch->component_types[i]].size;
      void *new_data = calloc(new_cap, size);
      memcpy(new_data, new_arch->component_data[i],
             new_arch->entity_capacity * size);
      free(new_arch->component_data[i]);
      new_arch->component_data[i] = new_data;
    }
    new_arch->entity_capacity = new_cap;
  }

  // Move ID
  new_arch->entities[new_index] = old_arch->entities[old_index];

  // Move Components (intersection)
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
  for (u32 i = 0; i < new_arch->component_count; i++) {
    ECSComponentID type = new_arch->component_types[i];

    // Find matching component in old archetype
    void *old_data_array = get_component_array(old_arch, type);
    if (old_data_array) {
      u32 size = cd->infos[type].size;
      void *new_data_array = new_arch->component_data[i];

      memcpy((u8 *)new_data_array + (new_index * size),
             (u8 *)old_data_array + (old_index * size), size);
    }
  }

  // Update Record
  rec->archetype_ptr = new_arch;
  rec->index_in_chunk = new_index;

  // Swap-remove from old archetype to keep packed
  if (old_arch->entity_count > 0) {
    u32 last_index = --old_arch->entity_count;
    if (old_index != last_index) {
      // Move last entity to hole
      Entity last_entity = old_arch->entities[last_index];
      old_arch->entities[old_index] = last_entity;

      // Move components
      for (u32 i = 0; i < old_arch->component_count; i++) {
        ECSComponentID type = old_arch->component_types[i];
        u32 size = cd->infos[type].size;
        void *data = old_arch->component_data[i];
        memcpy((u8 *)data + (old_index * size),
               (u8 *)data + (last_index * size), size);
      }

      // Update record of moved entity
      EntityData *ed = (EntityData *)world->entities;
      ed->records[last_entity.id].index_in_chunk = old_index;
    }
  }
}

// ============================================================================
// Component Functions
// ============================================================================

bool ecs_is_valid(World *world, Entity entity) {
  if (!world || !world->entities)
    return false;
  EntityData *ed = (EntityData *)world->entities;
  if (entity.id == 0 || entity.id >= ed->capacity)
    return false;
  return ed->records[entity.id].active &&
         ed->records[entity.id].generation == entity.generation;
}

bool ecs_add_component(World *world, Entity entity, ECSComponentID type,
                       const void *data) {
  if (!ecs_is_valid(world, entity))
    return false;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *old_arch = (SimpleArchetype *)rec->archetype_ptr;

  if (!old_arch) {
    // Find empty archetype or create default?
    // For now assume index 0 is valid "Empty" archetype if logic holds
    return false; // Should not happen if initialized correctly
  }

  // Check if already has component
  for (u32 i = 0; i < old_arch->component_count; i++) {
    if (old_arch->component_types[i] == type) {
      if (data) {
        void *comp_array = old_arch->component_data[i];
        ComponentDataInternal *cd =
            (ComponentDataInternal *)world->component_info;
        u32 size = cd->infos[type].size;
        memcpy((u8 *)comp_array + (rec->index_in_chunk * size), data, size);
      }
      return true;
    }
  }

  // Create new type list
  u32 new_count = old_arch->component_count + 1;
  ComponentType *new_types = malloc(new_count * sizeof(ComponentType));
  if (old_arch->component_count > 0) {
    memcpy(new_types, old_arch->component_types,
           old_arch->component_count * sizeof(ComponentType));
  }
  new_types[old_arch->component_count] = type;

  // Find/Create new archetype
  SimpleArchetype *new_arch =
      find_or_create_archetype(world, new_types, new_count);
  free(new_types);

  // Move Entity
  move_entity(world, rec, old_arch, new_arch);

  // Set new component data
  if (data) {
    void *comp_array = get_component_array(new_arch, type);
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    u32 size = cd->infos[type].size;

    if (size > 0) {
      memcpy((u8 *)comp_array + (rec->index_in_chunk * size), data, size);
    }
  }

  return true;
}

void *ecs_get_component(World *world, Entity entity, ECSComponentID type) {
  if (!ecs_is_valid(world, entity))
    return NULL;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *arch = (SimpleArchetype *)rec->archetype_ptr;

  if (!arch)
    return NULL;

  void *array = get_component_array(arch, type);
  if (!array)
    return NULL;

  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
  u32 size = cd->infos[type].size;
  return (u8 *)array + (rec->index_in_chunk * size);
}

bool ecs_remove_component(World *world, Entity entity, ECSComponentID type) {
  if (!ecs_is_valid(world, entity))
    return false;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *old_arch = (SimpleArchetype *)rec->archetype_ptr;
  if (!old_arch)
    return false;

  // Check validity
  bool has_comp = false;
  for (u32 i = 0; i < old_arch->component_count; i++) {
    if (old_arch->component_types[i] == type) {
      has_comp = true;
      break;
    }
  }
  if (!has_comp)
    return false;

  // Create new type list
  u32 new_count = old_arch->component_count - 1;
  ComponentType *new_types = NULL;
  if (new_count > 0) {
    new_types = malloc(new_count * sizeof(ComponentType));
    u32 dst = 0;
    for (u32 src = 0; src < old_arch->component_count; src++) {
      if (old_arch->component_types[src] != type) {
        new_types[dst++] = old_arch->component_types[src];
      }
    }
  }

  // Find/Create
  SimpleArchetype *new_arch =
      find_or_create_archetype(world, new_types, new_count);
  if (new_types)
    free(new_types);

  // Move
  move_entity(world, rec, old_arch, new_arch);

  return true;
}

// Stubs for linker (World Management)
World *ecs_world_create(const WorldConfig *config) {
  World *world = calloc(1, sizeof(World));
  if (config) {
    ecs_world_init(world, config->max_entities, 100, config->max_systems);
  } else {
    ecs_world_init(world, 10000, 100, 50);
  }
  return world;
}

void ecs_world_destroy(World *world) {
  ecs_world_free(world);
  free(world);
}

WorldConfig ecs_world_create_default_config(void) {
  WorldConfig config = {0};
  config.max_entities = 10000;
  config.max_archetypes = 100;
  config.max_systems = 50;
  return config;
}

// Query Stubs
// Query Implementation
Query *ecs_query_create(World *world, const QueryDesc *desc) {
  if (!world || !desc)
    return NULL;

  Query *query = calloc(1, sizeof(Query));

  // Copy component types
  if (desc->all_count > 0) {
    query->component_count = desc->all_count;
    query->component_types = malloc(desc->all_count * sizeof(ComponentType));
    memcpy(query->component_types, desc->all_components,
           desc->all_count * sizeof(ComponentType));
  }

  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *curr = ad->head;

  // Pre-calculate matching archetypes
  // In a real optimized system, this would be updated dynamically as archetypes
  // are created For now, we scan existing archetypes
  u32 match_cap = 16;
  query->matching_archetypes = malloc(match_cap * sizeof(Archetype *));
  query->archetype_count = 0;

  while (curr) {
    // Check ALL requirements
    bool matches = true;
    for (u32 i = 0; i < desc->all_count; i++) {
      bool found = false;
      for (u32 j = 0; j < curr->component_count; j++) {
        if (curr->component_types[j] == desc->all_components[i]) {
          found = true;
          break;
        }
      }
      if (!found) {
        matches = false;
        break;
      }
    }

    // Check NONE requirements
    if (matches && desc->none_count > 0) {
      for (u32 i = 0; i < desc->none_count; i++) {
        for (u32 j = 0; j < curr->component_count; j++) {
          if (curr->component_types[j] == desc->none_components[i]) {
            matches = false; // Has forbidden component
            break;
          }
        }
        if (!matches)
          break;
      }
    }

    // Check ANY requirements (if any exist, must have at least one)
    if (matches && desc->any_count > 0) {
      bool any_found = false;
      for (u32 i = 0; i < desc->any_count; i++) {
        for (u32 j = 0; j < curr->component_count; j++) {
          if (curr->component_types[j] == desc->any_components[i]) {
            any_found = true;
            break;
          }
        }
        if (any_found)
          break;
      }
      if (!any_found)
        matches = false;
    }

    if (matches) {
      if (query->archetype_count >= match_cap) {
        match_cap *= 2;
        query->matching_archetypes = realloc(query->matching_archetypes,
                                             match_cap * sizeof(Archetype *));
      }
      query->matching_archetypes[query->archetype_count++] = (Archetype *)curr;
    }

    curr = curr->next;
  }

  // Allocate offsets array for the iterator
  if (query->component_count > 0) {
    query->component_offsets = calloc(query->component_count, sizeof(u32));
  }

  return query;
}

void ecs_query_destroy(World *world, Query *query) {
  if (!query)
    return;
  if (query->component_types)
    free(query->component_types);
  if (query->matching_archetypes)
    free(query->matching_archetypes);
  if (query->component_offsets)
    free(query->component_offsets);
  free(query);
}

void ecs_query_reset(Query *query) {
  if (!query)
    return;
  query->current_archetype = 0;
  query->current_entity = 0;
  query->current_chunk = NULL;
}

bool ecs_query_next(Query *query, Entity *entity, void **components) {
  if (!query)
    return false;

  if (query->archetype_count == 0)
    return false;

  // Check if we need to advance archetype
  SimpleArchetype *arch =
      (SimpleArchetype *)query->matching_archetypes[query->current_archetype];

  // If current archetype is exhausted or empty, move to next
  while (query->current_entity >= arch->entity_count) {
    query->current_archetype++;
    if (query->current_archetype >= query->archetype_count) {
      // Reset for next frame/iteration if needed, or just return false
      // Standard iterator behavior: return false at end.
      // User must manually reset query if they want to reuse it, or just create
      // new one. Resetting indices for potential reuse:
      query->current_archetype = 0;
      query->current_entity = 0;
      return false;
    }
    arch =
        (SimpleArchetype *)query->matching_archetypes[query->current_archetype];
    query->current_entity = 0;
  }

  // Get Entity
  if (entity) {
    *entity = arch->entities[query->current_entity];
  }

  // Get Components
  if (components) {
    // We need to map query components to archetype components
    // Ideally this mapping is cached per archetype in the query
    // For now, linear search per entity (slow, but works)
    // OPTIMIZATION: Cache offsets in query->component_offsets when switching
    // archetypes

    for (u32 i = 0; i < query->component_count; i++) {
      ComponentType type = query->component_types[i];
      // Find this type in archetype
      void *data = NULL;
      for (u32 j = 0; j < arch->component_count; j++) {
        if (arch->component_types[j] == type) {
          data = arch->component_data[j];

          // Need size to calculate offset
          // We don't have world pointer here easily to check component_info
          // But we can assume SimpleArchetype stored mostly contiguous arrays
          // Wait, arch->component_data is void**. We need component size to
          // offset. Only way is to access global world or if components are
          // stored differently. 'SimpleArchetype' implementation in this file
          // uses component_info size.

          // CRITICAL: We need component size to iterate.
          // Let's use the global g_ecs_world helper for now or change API to
          // pass world The standard API ecs_query_next doesnt take world.

          if (g_ecs_world) {
            ComponentDataInternal *cd =
                (ComponentDataInternal *)g_ecs_world->component_info;
            u32 size = cd->infos[type].size;
            components[i] = (u8 *)data + (query->current_entity * size);
          }
          break;
        }
      }
    }
  }

  query->current_entity++;
  return true;
}

// Linker alias for entity check
bool ecs_has_component(World *world, Entity entity, ECSComponentID type) {
  return ecs_get_component(world, entity, type) != NULL;
}

bool ecs_set_component(World *world, Entity entity, ECSComponentID type,
                       const void *data) {
  // For now we can implement set as add since add handles update
  return ecs_add_component(world, entity, type, data);
}

// Internal helper to map core component names to static IDs
static ECSComponentID get_static_id_for_name(const char *name) {
  if (!name)
    return (ECSComponentID)-1;
  if (strcmp(name, "TransformComponent") == 0)
    return TRANSFORM_COMPONENT_ID;
  if (strcmp(name, "RigidbodyComponent") == 0)
    return RIGIDBODY_COMPONENT_ID;
  if (strcmp(name, "NPCComponent") == 0)
    return NPC_COMPONENT_ID;
  if (strcmp(name, "HealthComponent") == 0)
    return HEALTH_COMPONENT_ID;
  if (strcmp(name, "PlayerComponent") == 0)
    return PLAYER_COMPONENT_ID;
  if (strcmp(name, "HitboxComponent") == 0)
    return HITBOX_COMPONENT_ID;
  if (strcmp(name, "DamageComponent") == 0)
    return DAMAGE_COMPONENT_ID;
  if (strcmp(name, "ResistanceComponent") == 0)
    return RESISTANCE_COMPONENT_ID;
  if (strcmp(name, "ProjectileComponent") == 0)
    return PROJECTILE_COMPONENT_ID;
  if (strcmp(name, "InventoryComponent") == 0)
    return INVENTORY_COMPONENT_ID;
  if (strcmp(name, "ItemComponent") == 0)
    return ITEM_COMPONENT_ID;
  return (ECSComponentID)-1;
}

// Rename to match expected symbol
ECSComponentID ecs_register_component(World *world, const ComponentInfo *info) {
  if (!world || !world->component_info || !info)
    return 0;
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  // Check if component already registered by name
  for (u32 i = 0; i < cd->count; i++) {
    if (cd->infos[i].name && info->name &&
        strcmp(cd->infos[i].name, info->name) == 0) {
      return i;
    }
  }

  ECSComponentID id = get_static_id_for_name(info->name);
  if (id == (ECSComponentID)-1) {
    id = cd->count++;
  }

  if (id >= cd->capacity)
    return 0;

  if (id >= cd->count)
    cd->count = id + 1;

  ComponentInfo *param_info = &cd->infos[id];
  param_info->type = id;
  param_info->name = info->name ? strdup(info->name) : "Unnamed";
  param_info->size = info->size;
  param_info->alignment = info->alignment;

  world->component_count++;
  return id;
}

ECSComponentID ecs_get_component_id(World *world, const char *name) {
  if (!world || !world->component_info || !name)
    return 0;
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  // First try the static mapping for speed
  ECSComponentID static_id = get_static_id_for_name(name);
  if (static_id != (ECSComponentID)-1 && static_id < cd->count &&
      cd->infos[static_id].name) {
    return static_id;
  }

  // Fallback to searching
  for (u32 i = 0; i < cd->count; i++) {
    if (cd->infos[i].name && strcmp(cd->infos[i].name, name) == 0) {
      return i;
    }
  }
  return 0;
}

const ComponentInfo *ecs_get_component_info(World *world, ECSComponentID id) {
  if (!world || !world->component_info)
    return NULL;
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
  if (id >= cd->count)
    return NULL;
  return &cd->infos[id];
}

ECSComponentID ecs_register_component_simple(World *world, const char *name,
                                             u32 size) {
  ComponentInfo info = {0};
  info.name = name;
  info.size = size;
  info.alignment = 4;
  return ecs_register_component(world, &info);
}

WorldStats ecs_world_get_stats(World *world) {
  WorldStats stats = {0};
  if (world) {
    stats.entity_count = world->entity_count;
    stats.component_type_count = world->component_count;
    stats.system_count = world->system_count;
    stats.archetype_count = 0; // Stub
  }
  return stats;
}

bool ecs_world_save(World *world, const char *path) {
  LOG_INFO("[ECS] Saving world to %s (Stub)", path);
  return true;
}

bool ecs_world_load(World *world, const char *path) {
  LOG_INFO("[ECS] Loading world from %s (Stub)", path);
  return true;
}

bool ecs_entity_exists(World *world, Entity entity) {
  return ecs_is_valid(world, entity);
}

// ============================================================================
// System/Query Functions
// ============================================================================

// System Data (internal)
typedef struct {
  System systems[128]; // Fixed max for now matching simple implementation
  u32 count;
} SystemDataInternal;

System *ecs_system_create(World *world, const char *name, SystemFunc execute,
                          const QueryDesc *query_desc) {
  if (!world || !execute)
    return NULL;

  // Initialize system data if needed (stubbed in init but let's be safe)
  if (!world->systems) {
    world->systems = calloc(1, sizeof(SystemDataInternal));
  }

  SystemDataInternal *sd = (SystemDataInternal *)world->systems;
  if (sd->count >= 128)
    return NULL;

  System *sys = &sd->systems[sd->count++];
  sys->name = name ? strdup(name) : "Unnamed System";
  sys->execute = execute;
  sys->priority = 0;
  sys->enabled = true;

  if (query_desc) {
    sys->query = *query_desc;
    // Should we pre-compile the query? The system structure in ecs.h doesn't
    // hold a Query* but just QueryDesc. The SystemContext passed to execute
    // holds a Query*. We will create the query ONCE during system creation or
    // first run usually. But System struct definition in ecs.h only has
    // QueryDesc. Wait, SystemContext has Query*. We need to store the Query
    // object somewhere. Easiest is to change System struct in ecs.c to hold it,
    // but ecs.h defines System struct opaquely? No, it defines it fully (line
    // 152). Line 155 is `QueryDesc query;`. So where do we store the compiled
    // Query*? We can use `user_data` or just re-create it (slow). Or we assume
    // the system function manages it. ACTUALLY: Let's assume we re-create or
    // cache it. Better: Let's abuse the fact we are inside the engine and can
    // store it if we extend the struct or use a parallel array. For now, let's
    // create it every frame (slow but correct) or use static in function? No,
    // let's look at `ecs_world_update` below.
  }

  world->system_count++;
  return sys;
}

void ecs_world_update(World *world, f32 delta_time) {
  if (!world || !world->systems)
    return;

  SystemDataInternal *sd = (SystemDataInternal *)world->systems;

  // Simple serial execution for now
  for (u32 i = 0; i < sd->count; i++) {
    System *sys = &sd->systems[i];
    if (!sys->enabled)
      continue;

    Query *query = NULL;
    // Check if system has a query definition
    if (sys->query.all_count > 0 || sys->query.any_count > 0 ||
        sys->query.none_count > 0) {
      query = ecs_query_create(world, &sys->query);
    }

    SystemContext ctx = {
        .world = world,
        .delta_time = delta_time,
        .query = query,
        .user_data = NULL // Could support this if added to System struct
    };

    sys->execute(&ctx);

    if (query) {
      ecs_query_destroy(world, query);
    }
  }
}

void ecs_system_destroy(World *world, System *system) {
  // Stub - requires removing from array or marking invalid
}

void ecs_system_set_priority(World *world, System *system, u32 priority) {
  if (system)
    system->priority = priority;
  // Should resort systems
}

void ecs_system_set_enabled(World *world, System *system, bool enabled) {
  if (system)
    system->enabled = enabled;
}
