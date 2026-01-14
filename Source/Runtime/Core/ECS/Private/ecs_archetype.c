#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <stdlib.h> // For realloc (unified realloc might be missing, check headers later)
#include <string.h>

SimpleArchetype *find_or_create_archetype(World *world, ECSComponentID *types,
                                          u32 count) {
  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *curr = ad->head;

  // Search existing
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

  // Create New
  SimpleArchetype *new_arch = UNIFIED_ALLOC(sizeof(SimpleArchetype));
  memset(new_arch, 0, sizeof(SimpleArchetype));

  new_arch->component_count = count;
  if (count > 0) {
    memcpy(new_arch->component_types, types, count * sizeof(ECSComponentID));
  }

  new_arch->entity_capacity = 64;
  new_arch->entities =
      UNIFIED_ALLOC(sizeof(Entity) * new_arch->entity_capacity);

  if (count > 0) {
    new_arch->component_data = UNIFIED_ALLOC(sizeof(void *) * count);
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

    for (u32 i = 0; i < count; i++) {
      ECSComponentID type = types[i];
      u32 size = cd->infos[type].size;
      // Allocate dense array for this component
      new_arch->component_data[i] =
          UNIFIED_ALLOC(size * new_arch->entity_capacity);
      memset(new_arch->component_data[i], 0, size * new_arch->entity_capacity);
    }
  } else {
    new_arch->component_data = NULL;
  }

  new_arch->next = ad->head;
  ad->head = new_arch;
  ad->count++;

  return new_arch;
}

void move_entity_to_archetype(World *world, EntityRecord *rec,
                              SimpleArchetype *old_arch,
                              SimpleArchetype *new_arch) {
  u32 old_index = rec->index_in_chunk;
  u32 new_index = new_arch->entity_count++;

  // Resize new archetype if full
  if (new_index >= new_arch->entity_capacity) {
    u32 old_cap = new_arch->entity_capacity;
    u32 new_cap = old_cap * 2;

    // Resize entities array
    Entity *new_ents = UNIFIED_ALLOC(new_cap * sizeof(Entity));
    memcpy(new_ents, new_arch->entities, old_cap * sizeof(Entity));
    UNIFIED_FREE(new_arch->entities);
    new_arch->entities = new_ents;

    // Resize component arrays
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    for (u32 i = 0; i < new_arch->component_count; i++) {
      ECSComponentID type = new_arch->component_types[i];
      u32 size = cd->infos[type].size;

      void *new_data = UNIFIED_ALLOC(size * new_cap);
      memcpy(new_data, new_arch->component_data[i], size * old_cap);
      UNIFIED_FREE(new_arch->component_data[i]);
      new_arch->component_data[i] = new_data;
    }

    new_arch->entity_capacity = new_cap;
  }

  // Move Entity Logic
  new_arch->entities[new_index] = old_arch->entities[old_index];

  // Move overlapping component data
  ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;

  for (u32 i = 0; i < new_arch->component_count; i++) {
    ECSComponentID type = new_arch->component_types[i];

    // Find in old
    void *old_data_array = NULL;
    for (u32 j = 0; j < old_arch->component_count; j++) {
      if (old_arch->component_types[j] == type) {
        old_data_array = old_arch->component_data[j];
        break;
      }
    }

    if (old_data_array) {
      u32 size = cd->infos[type].size;
      void *new_data_array = new_arch->component_data[i];
      memcpy((u8 *)new_data_array + (new_index * size),
             (u8 *)old_data_array + (old_index * size), size);
    }
    // If not in old (new component added), it remains zero-initialized from
    // alloc/resize or caller sets it immediately after move.
  }

  // Update Record
  rec->archetype_ptr = new_arch;
  rec->index_in_chunk = new_index;

  // Remove from old archetype (Swap & Pop)
  // Only if old archetype isn't empty (it shouldn't be, we just moved from it)
  if (old_arch->entity_count > 0) {
    u32 last_index = --old_arch->entity_count;
    if (old_index != last_index) {
      // Move last entity into hole
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

      // Update record of moved last entity
      EntityData *ed = (EntityData *)world->entities;
      ed->records[last_entity.id].index_in_chunk = old_index;
    }
  }
}
