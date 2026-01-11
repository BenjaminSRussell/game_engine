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
  QueryDesc desc = {0};
  ComponentType components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *q = ecs_query_create((World *)system->ecs, &desc);
  f32 cull_dist_sq = NPC_CULL_DISTANCE * NPC_CULL_DISTANCE;

  Entity e_ent;
  void *c[2];
  while (ecs_query_next(q, &e_ent, c) && g_lod_count < 1024) {
    EntityID e = e_ent.id;
    TransformComponent *t = (TransformComponent *)c[1];
    if (!t)
      continue;
    Vec3 diff = vec3_sub(t->position, player_pos);
    f32 dist_sq = vec3_dot(diff, diff);
    if (dist_sq <= cull_dist_sq) {
      g_lod_entries[g_lod_count].entity = e;
      g_lod_entries[g_lod_count].distance_sq = dist_sq;
      g_lod_count++;
    }
  }
  ecs_query_destroy((World *)system->ecs, q);
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
