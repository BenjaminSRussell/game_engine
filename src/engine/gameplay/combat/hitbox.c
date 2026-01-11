#include "include/gameplay/combat/hitbox.h"
#include "include/core/logger.h"
#include "include/ecs/component_ids.h"
#include "include/ecs/ecs.h"
#include <string.h>

bool hitbox_system_init(World *world) {
  LOG_INFO("Hitbox system initialized");
  return true;
}

void hitbox_system_shutdown(void) { LOG_INFO("Hitbox system shutdown"); }

void hitbox_system_update(World *world, f32 delta_time) {
  // Update hitbox world positions, check for triggers, etc.
}

Entity hitbox_create_temporary(World *world, Vec3 position, Vec3 direction,
                               f32 range, f32 duration) {
  Entity entity = ecs_create_entity(world);
  if (entity.id == 0)
    return INVALID_ENTITY;

  // Set transform component (assuming it exists and we have it)
  // For now, we'll just focus on the HitboxComponent

  HitboxComponent hitbox = {.shape = HITBOX_SHAPE_SPHERE,
                            .active = true,
                            .is_trigger = true,
                            .team_id = 0,
                            .damage_multiplier = 1.0f,
                            .world_position = position};
  hitbox.data.sphere.radius = range;

  ecs_add_component(world, entity, HITBOX_COMPONENT_ID, &hitbox);

  // TODO: Add a TimerComponent to destroy the entity after 'duration'

  return entity;
}

HitboxComponent hitbox_create_sphere(f32 radius, u32 team_id) {
  HitboxComponent hb = {0};
  hb.shape = HITBOX_SHAPE_SPHERE;
  hb.data.sphere.radius = radius;
  hb.team_id = team_id;
  hb.active = true;
  hb.damage_multiplier = 1.0f;
  return hb;
}

void hitbox_activate(HitboxComponent *hitbox) {
  if (hitbox)
    hitbox->active = true;
}
void hitbox_deactivate(HitboxComponent *hitbox) {
  if (hitbox)
    hitbox->active = false;
}
void hitbox_set_active(HitboxComponent *hitbox, bool active) {
  if (hitbox)
    hitbox->active = active;
}
