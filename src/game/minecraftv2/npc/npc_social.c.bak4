// Social interactions between NPCs (Milestone 3 Phase B).
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <stdlib.h>

void npc_socialize(NPCSystem *system, EntityID entity, NPCComponent *npc,
                   f32 delta_time) {
  if (!system || !npc || npc->current_task != NPC_TASK_SOCIALIZE)
    return;

  TransformComponent *self = (TransformComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
  if (!self)
    return;

  // Find nearby NPC to socialize with
  EntityID target = 0;
  f32 best_dist = 6.0f;
  ComponentTypeID comps[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  EntityQuery q;
  ecs_query_init(&q, 64);
  ecs_query_entities((World *)system->ecs, &q, comps, 2);
  for (u32 i = 0; i < q.count; i++) {
    EntityID other = q.entities[i];
    if (other == entity)
      continue;
    TransformComponent *t = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){other, 0}, TRANSFORM_COMPONENT_ID);
    if (!t)
      continue;
    f32 dist = vec3_length(vec3_sub(t->position, self->position));
    if (dist < best_dist) {
      best_dist = dist;
      target = other;
    }
  }
  ecs_query_free(&q);

  if (target) {
    NPCComponent *other_npc = (NPCComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){target, 0}, NPC_COMPONENT_ID);
    if (other_npc) {
      // Move toward target
      if (best_dist > 2.0f) {
        TransformComponent *t = (TransformComponent *)ecs_get_component(
            (World *)system->ecs, (Entity){target, 0}, TRANSFORM_COMPONENT_ID);
        if (t) {
          npc->path[0] = t->position;
          npc->path_length = 1;
          npc->current_path_index = 0;
        }
      } else {
        // Socialize: improve relationship and social need
        npc->behavior_timer -= delta_time;
        if (npc->behavior_timer <= 0.0f) {
          i16 rel = npc_get_relation(system, entity, target);
          npc_adjust_relation(system, entity, target, 5);
          npc_adjust_relation(system, target, entity, 5);
          npc->social_need = MIN(100.0f, npc->social_need + 15.0f);
          other_npc->social_need = MIN(100.0f, other_npc->social_need + 15.0f);
          LOG_INFO("NPC %u socializes with %u (rel %d)", entity, target, rel);
          npc->behavior_timer = 6.0f;
        }
      }
    }
  }
}
