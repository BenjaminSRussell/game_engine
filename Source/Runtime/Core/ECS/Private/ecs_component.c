#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// Forward declaration from ecs_archetype.c (to be created)
SimpleArchetype *find_or_create_archetype(World *world, ECSComponentID *types,
                                          u32 count);
void move_entity_to_archetype(World *world, EntityRecord *rec,
                              SimpleArchetype *old_arch,
                              SimpleArchetype *new_arch);

ECSComponentID ecs_register_component(World *world, const ComponentInfo *info) {
  if (!world || !world->component_info || !info)
    return 0;

  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  // Simple check for existing name
  for (u32 i = 1; i < cd->count; i++) { // Start at 1, 0 is invalid
    if (cd->infos[i].name && strcmp(cd->infos[i].name, info->name) == 0) {
      return i;
    }
  }

  if (cd->count >= cd->capacity) {
    LOG_ERROR(LOG_CAT_GENERAL, "ECS Error: Max components reached");
    return 0;
  }

  // Register new
  if (cd->count == 0)
    cd->count = 1; // Ensure 0 is skip

  u32 id = cd->count++;
  cd->infos[id] = *info;

  // Duplicate name string to own memory? For now refer to string literal
  // assumption But safer to strdup if we had it, or assume persistent strings.
  return id;
}

bool ecs_add_component(World *world, Entity entity, ECSComponentID type,
                       const void *data) {
  if (!ecs_is_valid(world, entity))
    return false;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *old_arch = (SimpleArchetype *)rec->archetype_ptr;

  // Check if component exists
  for (u32 i = 0; i < old_arch->component_count; i++) {
    if (old_arch->component_types[i] == type) {
      // Already has component, update data
      if (data) {
        ComponentDataInternal *cd =
            (ComponentDataInternal *)world->component_info;
        u32 size = cd->infos[type].size;
        void *arr = old_arch->component_data[i];
        memcpy((u8 *)arr + (rec->index_in_chunk * size), data, size);
      }
      return true;
    }
  }

  // Create new component list
  u32 new_count = old_arch->component_count + 1;
  ECSComponentID *new_types = UNIFIED_ALLOC(sizeof(ECSComponentID) * new_count);

  if (old_arch->component_count > 0) {
    memcpy(new_types, old_arch->component_types,
           old_arch->component_count * sizeof(ECSComponentID));
  }
  new_types[old_arch->component_count] = type;

  // Find/Create Archetype
  SimpleArchetype *new_arch =
      find_or_create_archetype(world, new_types, new_count);
  UNIFIED_FREE(new_types);

  if (!new_arch)
    return false;

  // Move Entity
  move_entity_to_archetype(world, rec, old_arch, new_arch);

  // Initialize new component data
  if (data) {
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    u32 size = cd->infos[type].size;

    // Find index of new component in new archetype
    u32 comp_idx = 0;
    for (u32 i = 0; i < new_arch->component_count; i++) {
      if (new_arch->component_types[i] == type) {
        comp_idx = i;
        break;
      }
    }

    void *arr = new_arch->component_data[comp_idx];
    memcpy((u8 *)arr + (rec->index_in_chunk * size), data, size);
  }

  return true;
}

bool ecs_remove_component(World *world, Entity entity, ECSComponentID type) {
  if (!ecs_is_valid(world, entity))
    return false;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *old_arch = (SimpleArchetype *)rec->archetype_ptr;

  // Check if component exists
  bool found = false;
  for (u32 i = 0; i < old_arch->component_count; i++) {
    if (old_arch->component_types[i] == type) {
      found = true;
      break;
    }
  }
  if (!found)
    return false;

  // Create new list
  u32 new_count = old_arch->component_count - 1;
  ECSComponentID *new_types = NULL;
  if (new_count > 0) {
    new_types = UNIFIED_ALLOC(sizeof(ECSComponentID) * new_count);
    u32 dst = 0;
    for (u32 i = 0; i < old_arch->component_count; i++) {
      if (old_arch->component_types[i] != type) {
        new_types[dst++] = old_arch->component_types[i];
      }
    }
  }

  SimpleArchetype *new_arch =
      find_or_create_archetype(world, new_types, new_count);
  if (new_types)
    UNIFIED_FREE(new_types);

  move_entity_to_archetype(world, rec, old_arch, new_arch);

  return true;
}

void *ecs_get_component(World *world, Entity entity, ECSComponentID type) {
  if (!ecs_is_valid(world, entity))
    return NULL;

  EntityData *ed = (EntityData *)world->entities;
  EntityRecord *rec = &ed->records[entity.id];
  SimpleArchetype *arch = (SimpleArchetype *)rec->archetype_ptr;

  for (u32 i = 0; i < arch->component_count; i++) {
    if (arch->component_types[i] == type) {
      ComponentDataInternal *cd =
          (ComponentDataInternal *)world->component_info;
      u32 size = cd->infos[type].size;
      return (u8 *)arch->component_data[i] + (rec->index_in_chunk * size);
    }
  }
  return NULL;
}

bool ecs_has_component(World *world, Entity entity, ECSComponentID type) {
  return ecs_get_component(world, entity, type) != NULL;
}

bool ecs_set_component(World *world, Entity entity, ECSComponentID type,
                       const void *data) {
  void *ptr = ecs_get_component(world, entity, type);
  if (ptr) {
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    u32 size = cd->infos[type].size;
    memcpy(ptr, data, size);
    return true;
  }
  // Component doesn't exist, we could add it? Standard API usually implies set
  // = replace if exists or fail. ecs_add_component handles set-if-exists logic
  // too.
  return ecs_add_component(world, entity, type, data);
}
