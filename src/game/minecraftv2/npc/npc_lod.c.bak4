// Performance: LOD and culling for NPCs (Milestone 5).
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <stdlib.h>

#define NPC_CULL_DISTANCE 96.0f
#define NPC_LOD_DISTANCE 48.0f

typedef struct {
  EntityID entity;
  f32 distance_sq;
} NPCLodEntry;

static NPCLodEntry g_lod_entries[1024];
static u32 g_lod_count = 0;

void npc_lod_update(NPCSystem *system, Vec3 player_pos) {
  if (!system)
    return;

  g_lod_count = 0;
  ComponentTypeID comps[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  EntityQuery q;
  ecs_query_init(&q, 1024);
  ecs_query_entities((World *)system->ecs, &q, comps, 2);
  f32 cull_dist_sq = NPC_CULL_DISTANCE * NPC_CULL_DISTANCE;
  for (u32 i = 0; i < q.count; i++) {
    EntityID e = q.entities[i];
    TransformComponent *t = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){e, 0}, TRANSFORM_COMPONENT_ID);
    if (!t)
      continue;
    Vec3 diff = vec3_sub(t->position, player_pos);
    f32 dist_sq = vec3_dot(diff, diff);
    if (dist_sq <= cull_dist_sq && g_lod_count < 1024) {
      g_lod_entries[g_lod_count].entity = e;
      g_lod_entries[g_lod_count].distance_sq = dist_sq;
      g_lod_count++;
    }
  }
  ecs_query_free(&q);
}

bool npc_should_update(EntityID entity, f32 dt) {
  for (u32 i = 0; i < g_lod_count; i++) {
    if (g_lod_entries[i].entity == entity) {
      f32 dist_sq = g_lod_entries[i].distance_sq;
      f32 lod_dist_sq = NPC_LOD_DISTANCE * NPC_LOD_DISTANCE;
      if (dist_sq > lod_dist_sq) {
        // Update less frequently when far
        return ((u32)(dt * 10.0f)) % 3 == 0;
      }
      return true;
    }
  }
  return false;
}
