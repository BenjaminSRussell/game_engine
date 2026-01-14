#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

Query *ecs_query_create(World *world, const QueryDesc *desc) {
  if (!world || !desc)
    return NULL;

  Query *query = UNIFIED_ALLOC(sizeof(Query));
  if (!query)
    return NULL;
  memset(query, 0, sizeof(Query));

  // Copy requested component types
  if (desc->all_count > 0) {
    query->component_count = desc->all_count;
    query->component_types =
        UNIFIED_ALLOC(sizeof(ECSComponentID) * desc->all_count);
    memcpy(query->component_types, desc->all_components,
           sizeof(ECSComponentID) * desc->all_count);

    query->component_offsets = UNIFIED_ALLOC(sizeof(u32) * desc->all_count);
  }

  // Pre-calculate matching archetypes
  // This simple implementation scans all archetypes at creation.
  // In a real optimized ECS, queries are cached and updated when new archetypes
  // are created.

  ArchetypeData *ad = (ArchetypeData *)world->archetypes;
  SimpleArchetype *curr = ad->head;

  u32 match_cap = 16;
  query->matching_archetypes =
      UNIFIED_ALLOC(sizeof(SimpleArchetype *) * match_cap);
  query->archetype_count = 0;

  while (curr) {
    bool matches = true;

    // Check ALL
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

    // Check NONE
    if (matches && desc->none_count > 0) {
      for (u32 i = 0; i < desc->none_count; i++) {
        for (u32 j = 0; j < curr->component_count; j++) {
          if (curr->component_types[j] == desc->none_components[i]) {
            matches = false;
            break;
          }
        }
        if (!matches)
          break;
      }
    }

    // Check ANY (if any provided, must match at least one)
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

    if (matches &&
        curr->entity_count >
            0) { // Only add non-empty archetypes? No, add all matching types
                 // for future. But iteration checks count.
      if (query->archetype_count >= match_cap) {
        match_cap *= 2;
        // Realloc manually if needed
        SimpleArchetype **new_arr =
            UNIFIED_ALLOC(sizeof(SimpleArchetype *) * match_cap);
        if (new_arr) {
          memcpy(new_arr, query->matching_archetypes,
                 sizeof(SimpleArchetype *) * query->archetype_count);
          UNIFIED_FREE(query->matching_archetypes);
          query->matching_archetypes = new_arr;
        }
      }
      query->matching_archetypes[query->archetype_count++] = curr;
    }

    curr = curr->next;
  }

  return query;
}

void ecs_query_destroy(World *world, Query *query) {
  (void)world;
  if (!query)
    return;

  if (query->component_types)
    UNIFIED_FREE(query->component_types);
  if (query->matching_archetypes)
    UNIFIED_FREE(query->matching_archetypes);
  if (query->component_offsets)
    UNIFIED_FREE(query->component_offsets);

  UNIFIED_FREE(query);
}

void ecs_query_reset(Query *query) {
  if (!query)
    return;
  query->current_archetype = 0;
  query->current_entity = 0;
}

bool ecs_query_next(Query *query, Entity *entity, void **components) {
  if (!query || query->archetype_count == 0)
    return false;

  while (query->current_archetype < query->archetype_count) {
    SimpleArchetype *arch =
        query->matching_archetypes[query->current_archetype];

    if (query->current_entity < arch->entity_count) {
      // Found valid entity
      u32 idx = query->current_entity;

      if (entity) {
        *entity = arch->entities[idx];
      }

      if (components) {
        // Populate component pointers
        // NOTE: This lookup is slow if done every entity. Should be cached per
        // archetype switch. For this implementation valid.

        // We need world to get sizes efficiently... or we just search.
        // We don't have world passed here.
        // Wait, SimpleArchetype stores component data arrays.
        // But we need to know WHICH array corresponds to WHICH requested
        // component index.

        // Let's iterate requested components
        for (u32 i = 0; i < query->component_count; i++) {
          ECSComponentID req_type = query->component_types[i];

          // Find this type in archetype
          // Optimization: Do this only when switching archetype
          for (u32 j = 0; j < arch->component_count; j++) {
            if (arch->component_types[j] == req_type) {
              // Found it
              // We need size to index
              // CRITICAL: We don't have component size here if access is purely
              // by SimpleArchetype But SimpleArchetype component_data[j] is the
              // head of the array. Wait, we need the element size to offset by
              // `idx`.

              // Hack: We need access to component info or store element size in
              // archetype. Let's assume we can rely on g_ecs_world global if
              // necessary, or modify design. Or better: The archetype creation
              // stored `size` somewhere? No.

              // Check ecs_core.c: ComponentInfo is in world.
              // Check ecs_types.h: SimpleArchetype doesn't store component
              // sizes.

              // Workaround: Use g_ecs_world global for now as specified in
              // ecs_core.c
              extern World *g_ecs_world;
              if (g_ecs_world) {
                ComponentDataInternal *cd =
                    (ComponentDataInternal *)g_ecs_world->component_info;
                u32 size = cd->infos[req_type].size;
                components[i] = (u8 *)arch->component_data[j] + (idx * size);
              }
              break;
            }
          }
        }
      }

      query->current_entity++;
      return true;
    }

    // Move to next archetype
    query->current_archetype++;
    query->current_entity = 0;
  }

  return false;
}
