#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

Entity ecs_create_entity(World *world) {
  if (!world || !world->entities)
    return (Entity){0, 0};

  EntityData *ed = (EntityData *)world->entities;
  u32 id;

  if (ed->free_count > 0) {
    id = ed->free_entity_ids[--ed->free_count];
  } else {
    if (ed->next_entity_id >= ed->capacity) {
      LOG_ERROR(LOG_CAT_GENERAL, "ECS Error: Max entities reached (%d)",
                ed->capacity);
      return (Entity){0, 0};
    }
    id = ed->next_entity_id++;
  }

  // Init Record
  EntityRecord *rec = &ed->records[id];
  rec->active = true;
  rec->generation++;

  // Add entity to empty archetype (Archetype 0)
  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *empty_arch = ad->empty_archetype;

  if (empty_arch) {
    // Ensure capacity in empty archetype
    if (empty_arch->entity_count >= empty_arch->entity_capacity) {
      u32 new_cap = empty_arch->entity_capacity * 2;
      Entity *new_entities = (Entity *)UNIFIED_ALLOC(sizeof(Entity) * new_cap);
      if (!new_entities) {
        LOG_ERROR(LOG_CAT_GENERAL,
                  "ECS Error: Failed to realloc empty archetype entities");
        return (Entity){0, 0};
      }

      if (empty_arch->entities) {
        memcpy(new_entities, empty_arch->entities,
               empty_arch->entity_capacity * sizeof(Entity));
        UNIFIED_FREE(empty_arch->entities);
      }
      empty_arch->entities = new_entities;
      empty_arch->entity_capacity = new_cap;
    }

    rec->archetype_ptr = empty_arch;
    rec->index_in_chunk = empty_arch->entity_count; // Add to end
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

  // Validate generation
  if (!rec->active || rec->generation != entity.generation)
    return;

  SimpleArchetype *arch = (SimpleArchetype *)rec->archetype_ptr;
  if (arch) {
    // Remove from archetype (Swap and Pop)
    u32 index = rec->index_in_chunk;
    u32 last_index = arch->entity_count - 1;

    if (index != last_index) {
      // Swap with last entity
      Entity last_entity = arch->entities[last_index];
      arch->entities[index] = last_entity;

      // Move component data
      ComponentDataInternal *cd =
          (ComponentDataInternal *)world->component_info;
      for (u32 i = 0; i < arch->component_count; i++) {
        ECSComponentID type = arch->component_types[i];
        u32 size = cd->infos[type].size;
        void *data_array = arch->component_data[i];

        // memcpy(dest, src, size)
        memcpy((u8 *)data_array + (index * size),
               (u8 *)data_array + (last_index * size), size);
      }

      // Update record of the moved entity
      ed->records[last_entity.id].index_in_chunk = index;
    }

    // Decrease count
    arch->entity_count--;
  }

  rec->active = false;
  ed->free_entity_ids[ed->free_count++] = entity.id;
  world->entity_count--;
}

bool ecs_is_valid(World *world, Entity entity) {
  if (!world || !world->entities)
    return false;
  EntityData *ed = (EntityData *)world->entities;
  if (entity.id == 0 || entity.id >= ed->capacity)
    return false;

  EntityRecord *rec = &ed->records[entity.id];
  return rec->active && rec->generation == entity.generation;
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
