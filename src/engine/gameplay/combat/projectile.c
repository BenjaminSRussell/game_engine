#include "include/gameplay/combat/projectile.h"
#include "include/core/logger.h"
#include "include/ecs/component_ids.h"
#include "include/ecs/ecs.h"
#include <string.h>

bool projectile_system_init(World *world) {
  LOG_INFO("Projectile system initialized");
  return true;
}

void projectile_system_shutdown(void) {
  LOG_INFO("Projectile system shutdown");
}

void projectile_system_update(World *world, f32 delta_time) {
  // Move projectiles, check collisions
}

Entity projectile_spawn(World *world, Vec3 position, Vec3 direction, f32 speed,
                        Entity source, f32 damage) {
  Entity entity = ecs_create_entity(world);
  if (entity.id == 0)
    return INVALID_ENTITY;

  ProjectileComponent proj = {.velocity = {direction.x * speed,
                                           direction.y * speed,
                                           direction.z * speed},
                              .speed = speed,
                              .max_speed = speed * 2.0f,
                              .lifetime = 5.0f, // 5 second default lifetime
                              .source = source,
                              .damage = damage,
                              .damage_type = DAMAGE_TYPE_PROJECTILE,
                              .behavior = PROJECTILE_BEHAVIOR_DESTROY,
                              .check_terrain = true,
                              .collision_radius = 0.1f};

  ecs_add_component(world, entity, PROJECTILE_COMPONENT_ID, &proj);

  return entity;
}

ProjectileComponent projectile_create(Vec3 velocity, f32 lifetime) {
  ProjectileComponent proj = {0};
  proj.velocity = velocity;
  proj.lifetime = lifetime;
  proj.behavior = PROJECTILE_BEHAVIOR_DESTROY;
  return proj;
}
