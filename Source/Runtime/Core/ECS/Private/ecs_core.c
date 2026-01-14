#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// Global pointer for legacy/convenience access if needed, though we strive for
// instance-based
World *g_ecs_world = NULL;

World *ecs_world_create(const WorldConfig *config) {
  World *world = UNIFIED_ALLOC(sizeof(World));
  if (!world)
    return NULL;
  memset(world, 0, sizeof(World));

  WorldConfig cfg = config ? *config : ecs_world_create_default_config();
  world->config = cfg;

  // Initialize Entity Data
  EntityData *ed = UNIFIED_ALLOC(sizeof(EntityData));
  memset(ed, 0, sizeof(EntityData));
  ed->capacity = cfg.max_entities;
  ed->records = UNIFIED_ALLOC(sizeof(EntityRecord) * cfg.max_entities);
  memset(ed->records, 0, sizeof(EntityRecord) * cfg.max_entities);

  ed->free_entity_ids = UNIFIED_ALLOC(sizeof(u32) * cfg.max_entities);
  memset(ed->free_entity_ids, 0, sizeof(u32) * cfg.max_entities);

  ed->next_entity_id = 1; // 0 is invalid
  world->entities = ed;

  // Initialize Component Info Data
  ComponentDataInternal *cd = UNIFIED_ALLOC(sizeof(ComponentDataInternal));
  memset(cd, 0, sizeof(ComponentDataInternal));
  cd->capacity = cfg.max_components;
  cd->infos = UNIFIED_ALLOC(sizeof(ComponentInfo) * cfg.max_components);
  memset(cd->infos, 0, sizeof(ComponentInfo) * cfg.max_components);
  world->component_info = cd;

  // Initialize Archetype Data
  ArchetypeData *ad = UNIFIED_ALLOC(sizeof(ArchetypeData));
  memset(ad, 0, sizeof(ArchetypeData));
  world->archetypes = ad;

  // Create Empty Archetype (Archetype 0)
  SimpleArchetype *empty = UNIFIED_ALLOC(sizeof(SimpleArchetype));
  memset(empty, 0, sizeof(SimpleArchetype));
  empty->component_count = 0;
  empty->entity_capacity = 64; // Initial capacity
  empty->entity_count = 0;
  empty->entities = UNIFIED_ALLOC(sizeof(Entity) * empty->entity_capacity);
  empty->component_data = NULL; // No components
  empty->next = NULL;

  ad->head = empty;
  ad->empty_archetype = empty;
  ad->count = 1;

  world->entity_count = 0;
  world->component_count = 0;
  world->system_count = 0;

  g_ecs_world = world;

  LOG_INFO(LOG_CAT_GENERAL, "ECS World Initialized (Capacity: %d entities)",
           cfg.max_entities);
  return world;
}

void ecs_world_destroy(World *world) {
  if (!world)
    return;

  // Free Entity Data
  if (world->entities) {
    EntityData *ed = (EntityData *)world->entities;
    if (ed->records)
      UNIFIED_FREE(ed->records);
    if (ed->free_entity_ids)
      UNIFIED_FREE(ed->free_entity_ids);
    UNIFIED_FREE(ed);
  }

  // Free Component Data
  if (world->component_info) {
    ComponentDataInternal *cd = (ComponentDataInternal *)world->component_info;
    if (cd->infos)
      UNIFIED_FREE(cd->infos);
    UNIFIED_FREE(cd);
  }

  // Free Archetypes
  if (world->archetypes) {
    ArchetypeData *ad = (ArchetypeData *)world->archetypes;
    SimpleArchetype *curr = ad->head;
    while (curr) {
      SimpleArchetype *next = curr->next;
      if (curr->entities)
        UNIFIED_FREE(curr->entities);
      if (curr->component_data) {
        // Free each component array
        for (u32 i = 0; i < curr->component_count; i++) {
          if (curr->component_data[i]) {
            UNIFIED_FREE(curr->component_data[i]);
          }
        }
        UNIFIED_FREE(curr->component_data);
      }
      UNIFIED_FREE(curr);
      curr = next;
    }
    UNIFIED_FREE(ad);
  }

  if (g_ecs_world == world) {
    g_ecs_world = NULL;
  }

  UNIFIED_FREE(world);
  LOG_INFO(LOG_CAT_GENERAL, "ECS World Destroyed");
}

WorldConfig ecs_world_create_default_config(void) {
  WorldConfig config;
  memset(&config, 0, sizeof(WorldConfig));
  config.max_entities = 10000;
  config.max_components = 100;
  config.max_archetypes = 100;
  config.max_systems = 50;
  return config;
}
