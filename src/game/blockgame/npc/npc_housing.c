// Housing allocation system for NPCs (Milestone 3 Phase B).
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <npc/npc_jobs.h>
#include <stdlib.h>

typedef struct {
  EntityID house_entity;
  EntityID resident;
  bool occupied;
} HouseSlot;

static HouseSlot g_houses[128];
static u8 g_house_count = 0;

void housing_register_house(EntityID house_entity) {
  if (g_house_count >= 128)
    return;
  g_houses[g_house_count].house_entity = house_entity;
  g_houses[g_house_count].resident = 0;
  g_houses[g_house_count].occupied = false;
  g_house_count++;
}

void housing_assign_to_nearest(NPCSystem *system, EntityID npc) {
  if (!system || g_house_count == 0)
    return;
  NPCComponent *npc_comp = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, NPC_COMPONENT_ID);
  TransformComponent *npc_t = (TransformComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, TRANSFORM_COMPONENT_ID);
  if (!npc_comp || !npc_t || npc_comp->home != 0)
    return;

  EntityID best_house = 0;
  f32 best_dist = 9999.0f;
  for (u8 i = 0; i < g_house_count; i++) {
    if (g_houses[i].occupied)
      continue;
    TransformComponent *house_t = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){g_houses[i].house_entity, 0},
        TRANSFORM_COMPONENT_ID);
    if (!house_t)
      continue;
    f32 dist = vec3_length(vec3_sub(house_t->position, npc_t->position));
    if (dist < best_dist) {
      best_dist = dist;
      best_house = g_houses[i].house_entity;
    }
  }

  if (best_house) {
    npc_assign_home(system, npc, best_house);
    for (u8 i = 0; i < g_house_count; i++) {
      if (g_houses[i].house_entity == best_house) {
        g_houses[i].resident = npc;
        g_houses[i].occupied = true;
        break;
      }
    }
  }
}

void housing_update(NPCSystem *system) {
  if (!system)
    return;
  // Assign homes to homeless NPCs with jobs
  QueryDesc desc = {0};
  ComponentType components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *q = ecs_query_create((World *)system->ecs, &desc);

  Entity e;
  void *c[2];
  while (ecs_query_next(q, &e, c)) {
    NPCComponent *npc = (NPCComponent *)c[0];
    if (npc && npc->job != NPC_JOB_NONE &&
        !(npc->behavior_flags & NPC_FLAG_HAS_HOME)) {
      housing_assign_to_nearest(system, e.id);
    }
  }
  ecs_query_destroy((World *)system->ecs, q);
}
