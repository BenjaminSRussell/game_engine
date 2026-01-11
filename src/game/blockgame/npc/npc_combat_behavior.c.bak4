// Combat behavior extensions for NPCs (Milestone 4).
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <npc/npc_types.h>
#include <stdlib.h>

void npc_combat_behavior_update(NPCSystem *system, EntityID entity,
                                NPCComponent *npc, f32 delta_time) {
  if (!system || !npc)
    return;

  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, HEALTH_COMPONENT_ID);
  if (!health)
    return;

  // Flee/surrender logic
  if (npc->state == NPC_STATE_FLEEING) {
    if (health->health < health->max_health * 0.2f) {
      // Surrender when very low health
      npc->state = NPC_STATE_IDLE;
      npc->panic_timer = 0.0f;
      npc->flee_target = 0;
      LOG_INFO("NPC %u surrenders", entity);
    }
  }

  // Militia behavior: assist nearby allies
  if (npc->job == NPC_JOB_GUARD) {
    TransformComponent *self = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
    if (!self)
      return;

    // Find nearby allies in combat
    ComponentTypeID comps[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID,
                               HEALTH_COMPONENT_ID};
    EntityQuery q;
    ecs_query_init(&q, 64);
    ecs_query_entities((World *)system->ecs, &q, comps, 3);
    for (u32 i = 0; i < q.count; i++) {
      EntityID other = q.entities[i];
      if (other == entity)
        continue;
      NPCComponent *other_npc = (NPCComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){other, 0}, NPC_COMPONENT_ID);
      TransformComponent *other_t = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){other, 0}, TRANSFORM_COMPONENT_ID);
      HealthComponent *other_h = (HealthComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){other, 0}, HEALTH_COMPONENT_ID);
      if (!other_npc || !other_t || !other_h)
        continue;
      if (other_npc->state == NPC_STATE_FLEEING &&
          vec3_length(vec3_sub(other_t->position, self->position)) < 12.0f) {
        // Assist fleeing ally
        npc->state = NPC_STATE_CHASING;
        npc->target = other_npc->flee_target;
        LOG_INFO("Guard %u assists NPC %u", entity, other);
        break;
      }
    }
    ecs_query_free(&q);
  }
}

void npc_equip_weapon(NPCSystem *system, EntityID npc, u32 weapon_type) {
  if (!system)
    return;
  NPCComponent *npc_comp = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, NPC_COMPONENT_ID);
  if (!npc_comp)
    return;
  // Placeholder: store weapon type
  LOG_INFO("NPC %u equips weapon type %u", npc, weapon_type);
}

void npc_equip_armor(NPCSystem *system, EntityID npc, u32 armor_type) {
  if (!system)
    return;
  NPCComponent *npc_comp = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, NPC_COMPONENT_ID);
  if (!npc_comp)
    return;
  // Placeholder: store armor type
  LOG_INFO("NPC %u equips armor type %u", npc, armor_type);
}
