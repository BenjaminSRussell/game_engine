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
    if (world->archetypes) {
      ArchetypeData *ad = (ArchetypeData *)world->archetypes;
      for (SimpleArchetype *arch = ad->head; arch; arch = arch->next)
        stats.archetype_count++;
    }
  }
  return stats;
}

// ============================================================================
// World save/load
// ============================================================================
//
// Format version 1, native byte order. Systems hold code, so they are not
// saved. Layout:
//   "ECSW" magic, u32 version
//   u32 slot_count, then per component slot 0..slot_count-1:
//     u32 has_name; if has_name: u32 size, u32 alignment, u32 name_length,
//     then the name bytes (no terminator)
//   u32 id_count (next entity id), then per entity id 1..id_count-1:
//     u32 generation, u32 active; if active: u32 component_count, then per
//     component: u32 component id and size bytes of component data
// Loading requires the target world to register every saved component at the
// same id with the same name, size and alignment. A mismatch or a corrupt file
// returns false and leaves the world unchanged. Out-of-memory while applying a
// validated image can leave a partial load.

#define ECS_SAVE_MAGIC "ECSW"
#define ECS_SAVE_VERSION 1u
#define ECS_SAVE_MAX_COMPONENTS 32u // matches SimpleArchetype::component_types

typedef struct {
  const u8 *data;
  size_t size;
  size_t pos;
} SaveReader;

static bool save_write(FILE *f, const void *src, size_t n) {
  return n == 0 || fwrite(src, 1, n, f) == n;
}

static bool save_write_u32(FILE *f, u32 value) {
  return save_write(f, &value, sizeof value);
}

static bool save_take(SaveReader *r, size_t n, const u8 **out) {
  if (n > r->size - r->pos)
    return false;
  *out = r->data + r->pos;
  r->pos += n;
  return true;
}

static bool save_read_u32(SaveReader *r, u32 *out) {
  const u8 *bytes;
  if (!save_take(r, sizeof *out, &bytes))
    return false;
  memcpy(out, bytes, sizeof *out);
  return true;
}

// Writes the component list and data of one live entity from its record.
static bool save_entity(FILE *f, const ComponentDataInternal *cd, u32 id,
                        const EntityRecord *rec) {
  SimpleArchetype *arch = (SimpleArchetype *)rec->archetype_ptr;
  // Refuse a record whose archetype slot does not point back at it.
  if (!arch || rec->index_in_chunk >= arch->entity_count ||
      arch->entities[rec->index_in_chunk].id != id)
    return false;
  if (!save_write_u32(f, arch->component_count))
    return false;
  for (u32 j = 0; j < arch->component_count; j++) {
    ECSComponentID type = arch->component_types[j];
    if (type >= cd->count)
      return false;
    size_t size = cd->infos[type].size;
    const u8 *value =
        size ? (const u8 *)arch->component_data[j] +
                   (size_t)rec->index_in_chunk * size
             : NULL;
    if (!save_write_u32(f, type) || !save_write(f, value, size))
      return false;
  }
  return true;
}

bool ecs_world_save(World *world, const char *path) {
  if (!world || !path || !world->entities || !world->component_info ||
      !world->archetypes)
    return false;
  EntityData *ed = (EntityData *)world->entities;
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  FILE *f = fopen(path, "wb");
  if (!f) {
    LOG_ERROR("[ECS] Cannot open %s for writing", path);
    return false;
  }

  bool ok = save_write(f, ECS_SAVE_MAGIC, 4) &&
            save_write_u32(f, ECS_SAVE_VERSION) &&
            save_write_u32(f, cd->count);
  for (u32 slot = 0; ok && slot < cd->count; slot++) {
    const ComponentInfo *info = &cd->infos[slot];
    ok = save_write_u32(f, info->name != NULL);
    if (ok && info->name) {
      u32 name_len = (u32)strlen(info->name);
      ok = save_write_u32(f, info->size) &&
           save_write_u32(f, info->alignment) &&
           save_write_u32(f, name_len) && save_write(f, info->name, name_len);
    }
  }

  ok = ok && save_write_u32(f, ed->next_entity_id);
  for (u32 id = 1; ok && id < ed->next_entity_id; id++) {
    const EntityRecord *rec = &ed->records[id];
    ok = save_write_u32(f, rec->generation) && save_write_u32(f, rec->active);
    if (ok && rec->active)
      ok = save_entity(f, cd, id, rec);
  }

  if (fclose(f) != 0)
    ok = false;
  if (!ok) {
    LOG_ERROR("[ECS] Failed to save world to %s", path);
    remove(path);
    return false;
  }
  LOG_INFO("[ECS] Saved world to %s (%u entities)", path,
           (unsigned)world->entity_count);
  return true;
}

// Places a loaded entity in the empty archetype, as ecs_create_entity does,
// but with the saved id and generation.
static bool place_loaded_entity(World *world, u32 id, u32 generation) {
  EntityData *ed = (EntityData *)world->entities;
  SimpleArchetype *empty =
      ((ArchetypeData *)world->archetypes)->empty_archetype;
  if (empty->entity_count >= empty->entity_capacity) {
    u32 new_cap = empty->entity_capacity * 2;
    Entity *grown = realloc(empty->entities, new_cap * sizeof(Entity));
    if (!grown)
      return false;
    empty->entities = grown;
    empty->entity_capacity = new_cap;
  }
  EntityRecord *rec = &ed->records[id];
  rec->active = true;
  rec->generation = (u8)generation;
  rec->archetype_ptr = empty;
  rec->index_in_chunk = empty->entity_count;
  empty->entities[empty->entity_count++] = (Entity){id, generation};
  world->entity_count++;
  return true;
}

// Drops every entity and archetype. The component registry and systems stay.
static bool reset_entity_state(World *world) {
  EntityData *ed = (EntityData *)world->entities;
  ArchetypeData *ad = (ArchetypeData *)world->archetypes;

  // Allocate the replacement first so failure leaves the world untouched.
  SimpleArchetype *empty = calloc(1, sizeof(SimpleArchetype));
  Entity *empty_entities = calloc(16, sizeof(Entity));
  if (!empty || !empty_entities) {
    free(empty);
    free(empty_entities);
    return false;
  }
  empty->entity_capacity = 16;
  empty->entities = empty_entities;

  for (SimpleArchetype *curr = ad->head; curr;) {
    SimpleArchetype *next = curr->next;
    free(curr->entities);
    if (curr->component_data) {
      for (u32 i = 0; i < curr->component_count; i++)
        free(curr->component_data[i]);
      free(curr->component_data);
    }
    free(curr);
    curr = next;
  }
  ad->head = empty;
  ad->empty_archetype = empty;
  ad->count = 1;

  memset(ed->records, 0, (size_t)ed->capacity * sizeof(EntityRecord));
  ed->next_entity_id = 1;
  ed->free_count = 0;
  world->entity_count = 0;
  return true;
}

// Walks a save image. With apply false it only validates. With apply true it
// rebuilds the entities, so the caller must reset the entity state first.
static bool parse_world_image(World *world, const u8 *image, size_t size,
                              bool apply) {
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
  EntityData *ed = (EntityData *)world->entities;
  SaveReader r = {image, size, 0};
  const u8 *magic;
  u32 version, slot_count, id_count;
  if (!save_take(&r, 4, &magic) || memcmp(magic, ECS_SAVE_MAGIC, 4) != 0 ||
      !save_read_u32(&r, &version) || version != ECS_SAVE_VERSION ||
      !save_read_u32(&r, &slot_count))
    return false;

  for (u32 slot = 0; slot < slot_count; slot++) {
    u32 has_name;
    if (!save_read_u32(&r, &has_name) || has_name > 1)
      return false;
    if (!has_name)
      continue;
    u32 comp_size, alignment, name_len;
    const u8 *name;
    if (!save_read_u32(&r, &comp_size) || !save_read_u32(&r, &alignment) ||
        !save_read_u32(&r, &name_len) || !save_take(&r, name_len, &name))
      return false;
    const ComponentInfo *want = slot < cd->count ? &cd->infos[slot] : NULL;
    if (!want || !want->name || strlen(want->name) != name_len ||
        memcmp(want->name, name, name_len) != 0 || want->size != comp_size ||
        want->alignment != alignment) {
      LOG_ERROR("[ECS] Load: component slot %u does not match this world",
                (unsigned)slot);
      return false;
    }
  }

  if (!save_read_u32(&r, &id_count) || id_count < 1 ||
      id_count > ed->capacity)
    return false;
  if (apply)
    ed->next_entity_id = id_count;

  for (u32 id = 1; id < id_count; id++) {
    u32 generation, active;
    if (!save_read_u32(&r, &generation) || !save_read_u32(&r, &active) ||
        generation > 0xFFu || active > 1)
      return false;
    if (!active) {
      if (apply)
        ed->records[id].generation = (u8)generation;
      continue;
    }

    u32 component_count;
    if (!save_read_u32(&r, &component_count) ||
        component_count > ECS_SAVE_MAX_COMPONENTS)
      return false;
    if (apply && !place_loaded_entity(world, id, generation))
      return false;
    Entity entity = {id, generation};
    u32 seen[ECS_SAVE_MAX_COMPONENTS];
    for (u32 c = 0; c < component_count; c++) {
      u32 type;
      if (!save_read_u32(&r, &type) || type >= cd->count ||
          !cd->infos[type].name)
        return false;
      for (u32 k = 0; k < c; k++) {
        if (seen[k] == type)
          return false;
      }
      seen[c] = type;
      const u8 *value;
      if (!save_take(&r, cd->infos[type].size, &value))
        return false;
      if (apply && !ecs_add_component(world, entity, type, value))
        return false;
    }
  }

  if (r.pos != r.size)
    return false;
  if (apply) {
    for (u32 id = 1; id < id_count; id++) {
      if (!ed->records[id].active)
        ed->free_entity_ids[ed->free_count++] = id;
    }
  }
  return true;
}

bool ecs_world_load(World *world, const char *path) {
  if (!world || !path || !world->entities || !world->component_info ||
      !world->archetypes)
    return false;

  FILE *f = fopen(path, "rb");
  if (!f) {
    LOG_ERROR("[ECS] Cannot open %s for reading", path);
    return false;
  }
  long length = -1;
  if (fseek(f, 0, SEEK_END) == 0)
    length = ftell(f);
  u8 *image = NULL;
  bool read_ok = length > 0 && fseek(f, 0, SEEK_SET) == 0 &&
                 (image = malloc((size_t)length)) != NULL &&
                 fread(image, 1, (size_t)length, f) == (size_t)length;
  fclose(f);

  // Validate the whole image before the world is touched.
  bool ok = read_ok &&
            parse_world_image(world, image, (size_t)length, false) &&
            reset_entity_state(world) &&
            parse_world_image(world, image, (size_t)length, true);
  free(image);
  if (!ok) {
    LOG_ERROR("[ECS] Failed to load world from %s", path);
    return false;
  }
  LOG_INFO("[ECS] Loaded world from %s (%u entities)", path,
           (unsigned)world->entity_count);
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
  // Reuse a slot freed by ecs_system_destroy (execute == NULL), else append.
  u32 slot = 0;
  while (slot < sd->count && sd->systems[slot].execute)
    slot++;
  if (slot >= 128)
    return NULL;
  if (slot == sd->count)
    sd->count++;

  System *sys = &sd->systems[slot];
  *sys = (System){0};
  sys->name = strdup(name ? name : "Unnamed System"); // freed on destroy
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
    if (!sys->enabled || !sys->execute) // execute == NULL: destroyed slot
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
  if (!world || !world->systems || !system)
    return;
  SystemDataInternal *sd = (SystemDataInternal *)world->systems;

  // Accept only a slot inside this world's system table.
  uintptr_t base = (uintptr_t)sd->systems;
  uintptr_t addr = (uintptr_t)system;
  if (addr < base || addr >= base + sd->count * sizeof(System) ||
      (addr - base) % sizeof(System) != 0)
    return;
  if (!system->execute)
    return; // already destroyed

  free((void *)system->name);
  // The slot stays in place so other System pointers remain valid. A NULL
  // execute marks it free for ecs_system_create to reuse.
  *system = (System){0};
  world->system_count--;
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
