#include "gameplay/combat/projectile.h"
#include "gameplay/combat/damage.h"
#include "gameplay/combat/hitbox.h"
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <include/math/math.h>
#include <physics/physics.h>
#include <string.h>

// ============================================================================
// PROJECTILE CREATION
// ============================================================================

ProjectileComponent projectile_create(Vec3 velocity, f32 lifetime) {
  ProjectileComponent proj = {0};
  proj.velocity = velocity;
  proj.speed = vec3_length(velocity);
  proj.max_speed = proj.speed * 2.0f;
  proj.gravity_scale = 1.0f;
  proj.drag = 0.01f;
  proj.lifetime = lifetime;
  proj.age = 0.0f;
  proj.behavior = PROJECTILE_BEHAVIOR_DESTROY;
  proj.penetration_count = 0;
  proj.current_penetrations = 0;
  proj.collision_radius = 0.1f;
  proj.check_terrain = true;
  proj.damage = 10.0f;
  proj.damage_type = DAMAGE_TYPE_PHYSICAL;
  proj.knockback = 5.0f;
  proj.trail_length = 1.0f;
  proj.is_homing = false;
  proj.has_collided = false;
  return proj;
}

ProjectileComponent projectile_create_arrow(Vec3 velocity, f32 damage) {
  ProjectileComponent proj = projectile_create(velocity, 10.0f);
  proj.damage = damage;
  proj.gravity_scale = 1.0f;
  proj.collision_radius = 0.05f;
  proj.behavior = PROJECTILE_BEHAVIOR_STICK;
  return proj;
}

ProjectileComponent projectile_create_fireball(Vec3 velocity, f32 damage,
                                               f32 explosion_radius) {
  ProjectileComponent proj = projectile_create(velocity, 5.0f);
  proj.damage = damage;
  proj.damage_type = DAMAGE_TYPE_FIRE;
  proj.gravity_scale = 0.0f;
  proj.behavior = PROJECTILE_BEHAVIOR_EXPLODE;
  proj.explosion_radius = explosion_radius;
  proj.explosion_damage = damage * 1.5f;
  proj.explosion_falloff = true;
  proj.emit_light = true;
  proj.light_color = (Vec3){1.0f, 0.5f, 0.1f};
  return proj;
}

ProjectileComponent projectile_create_homing(Vec3 velocity, Entity target,
                                             f32 damage) {
  ProjectileComponent proj = projectile_create(velocity, 8.0f);
  proj.damage = damage;
  proj.is_homing = true;
  proj.homing_target = target;
  proj.homing_strength = 0.8f;
  proj.homing_delay = 0.2f;
  proj.gravity_scale = 0.0f;
  return proj;
}

// ============================================================================
// PROJECTILE PROPERTIES
// ============================================================================

void projectile_set_homing(ProjectileComponent *proj, Entity target,
                           f32 strength) {
  if (!proj)
    return;
  proj->is_homing = true;
  proj->homing_target = target;
  proj->homing_strength = strength;
}

void projectile_set_pierce(ProjectileComponent *proj, u32 pierce_count) {
  if (!proj)
    return;
  proj->behavior = PROJECTILE_BEHAVIOR_PIERCE;
  proj->penetration_count = pierce_count;
}

void projectile_set_explosion(ProjectileComponent *proj, f32 radius,
                              f32 damage) {
  if (!proj)
    return;
  proj->behavior = PROJECTILE_BEHAVIOR_EXPLODE;
  proj->explosion_radius = radius;
  proj->explosion_damage = damage;
  proj->explosion_falloff = true;
}

void projectile_set_gravity(ProjectileComponent *proj, f32 gravity_scale) {
  if (!proj)
    return;
  proj->gravity_scale = gravity_scale;
}

// ============================================================================
// PROJECTILE SYSTEM
// ============================================================================

static bool g_projectile_system_initialized = false;

void projectile_system_init(void) { g_projectile_system_initialized = true; }

void projectile_system_shutdown(void) {
  g_projectile_system_initialized = false;
}

void projectile_system_update(World *world, f64 delta_time) {
  if (!world || !g_projectile_system_initialized)
    return;

  // Query all entities with projectile components
  QueryDesc query_desc = {0};
  ComponentType components[] = {PROJECTILE_COMPONENT_ID};
  query_desc.all_components = components;
  query_desc.all_count = 1;

  Query *query = ecs_query_create(world, &query_desc);
  if (!query)
    return;

  Entity entity;
  void *component_ptrs[1];

  while (ecs_query_next(query, &entity, component_ptrs)) {
    ProjectileComponent *proj = (ProjectileComponent *)component_ptrs[0];
    if (!proj)
      continue;

    // Get transform component
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        world, entity, TRANSFORM_COMPONENT_ID);
    if (!transform)
      continue;

    Vec3 position = transform->position;

    // Update age
    proj->age += (f32)delta_time;

    // Check lifetime
    if (proj->age >= proj->lifetime) {
      ecs_destroy_entity(world, entity);
      continue;
    }

    // Save last position for raycast
    proj->last_position = position;

    // Apply homing
    if (proj->is_homing && proj->age >= proj->homing_delay) {
      projectile_apply_homing(proj, world, delta_time);
    }

    // Update movement
    projectile_update_movement(proj, &position, delta_time);

    // Check collision
    Entity hit_entity = INVALID_ENTITY;
    Vec3 hit_point = {0};
    Vec3 hit_normal = {0};

    bool collision =
        projectile_check_collision(proj, proj->last_position, position, world,
                                   &hit_entity, &hit_point, &hit_normal);

    if (collision) {
      projectile_on_impact(proj, world, entity, hit_entity, hit_point,
                           hit_normal);

      // Destroy or continue based on behavior
      if (proj->behavior == PROJECTILE_BEHAVIOR_DESTROY ||
          proj->behavior == PROJECTILE_BEHAVIOR_EXPLODE ||
          proj->behavior == PROJECTILE_BEHAVIOR_STICK) {
        ecs_destroy_entity(world, entity);
        continue;
      }
    }

    // Update transform
    transform->position = position;
  }

  ecs_query_destroy(world, query);
}

Entity projectile_spawn(World *world, Vec3 position, Vec3 direction, f32 speed,
                        Entity source, f32 damage) {
  if (!world)
    return INVALID_ENTITY;

  Entity entity = ecs_create_entity(world);

  // Add transform
  TransformComponent transform = {0};
  transform.position = position;
  transform.rotation = quat_identity();
  ecs_add_component(world, entity, TRANSFORM_COMPONENT_ID, &transform);

  // Add projectile component
  Vec3 velocity = vec3_mul(vec3_normalize(direction), speed);
  ProjectileComponent proj = projectile_create(velocity, 10.0f);
  proj.source = source;
  proj.damage = damage;
  ecs_add_component(world, entity, PROJECTILE_COMPONENT_ID, &proj);

  // Add hitbox for collision
  HitboxComponent hitbox = hitbox_create_sphere(0.1f, 0);
  ecs_add_component(world, entity, HITBOX_COMPONENT_ID, &hitbox);

  return entity;
}

// ============================================================================
// PROJECTILE PHYSICS
// ============================================================================

void projectile_update_movement(ProjectileComponent *proj, Vec3 *position,
                                f64 delta_time) {
  if (!proj || !position)
    return;

  f32 dt = (f32)delta_time;

  // Apply gravity
  if (proj->gravity_scale > 0.0f) {
    Vec3 gravity = {0.0f, -9.81f * proj->gravity_scale, 0.0f};
    proj->velocity = vec3_add(proj->velocity, vec3_mul(gravity, dt));
  }

  // Apply drag
  if (proj->drag > 0.0f) {
    f32 drag_factor = 1.0f - proj->drag * dt;
    proj->velocity = vec3_mul(proj->velocity, drag_factor);
  }

  // Clamp to max speed
  f32 current_speed = vec3_length(proj->velocity);
  if (current_speed > proj->max_speed) {
    proj->velocity = vec3_mul(vec3_normalize(proj->velocity), proj->max_speed);
  }

  // Update position
  *position = vec3_add(*position, vec3_mul(proj->velocity, dt));
}

void projectile_apply_homing(ProjectileComponent *proj, World *world,
                             f64 delta_time) {
  if (!proj || !world || !proj->is_homing)
    return;

  // Get target transform
  TransformComponent *target_transform =
      (TransformComponent *)ecs_get_component(world, proj->homing_target,
                                              TRANSFORM_COMPONENT_ID);

  if (!target_transform)
    return;

  // Calculate direction to target
  Vec3 current_pos;
  // We need current projectile position, but we don't have it here
  // This would need to be passed in or stored
  // For now, skip actual homing calculation

  f32 dt = (f32)delta_time;
  f32 turn_rate = proj->homing_strength * dt * 5.0f; // Arbitrary turn speed

  // This is simplified - real implementation would use proper vector rotation
  (void)turn_rate; // Suppress unused warning
}

bool projectile_check_collision(const ProjectileComponent *proj, Vec3 start,
                                Vec3 end, World *world, Entity *hit_entity,
                                Vec3 *hit_point, Vec3 *hit_normal) {
  if (!proj || !world)
    return false;

  // Simple raycast between start and end
  Vec3 direction = vec3_subtract(end, start);
  f32 distance = vec3_length(direction);

  if (distance < 0.0001f)
    return false;

  // Use actual physics raycast
  // TODO: Fix physics raycast integration
  // PhysicsRaycastResult result;
  // Check against both world and entities
  // u32 layer_mask = PHYS_LAYER_WORLD | PHYS_LAYER_DEFAULT; 
  
  // if (physics_raycast(start, vec3_normalize(direction), distance, layer_mask, &result)) {
  //     if (hit_entity) *hit_entity = result.entity;
  //     if (hit_point) *hit_point = result.point;
  //     if (hit_normal) *hit_normal = result.normal;
  //     return true;
  // }

  return false;
}

void projectile_on_impact(ProjectileComponent *proj, World *world,
                          Entity projectile_entity, Entity hit_entity,
                          Vec3 hit_point, Vec3 hit_normal) {
  if (!proj || !world)
    return;

  switch (proj->behavior) {
  case PROJECTILE_BEHAVIOR_DESTROY:
    // Just destroy
    break;

  case PROJECTILE_BEHAVIOR_PIERCE:
    // Check if we can pierce
    if (proj->current_penetrations < proj->penetration_count) {
      proj->current_penetrations++;
      proj->last_hit_entity = hit_entity;

      // Deal damage and continue
      if (hit_entity.id != 0) {
        damage_event_emit_simple(proj->source, hit_entity, proj->damage);
      }
      return; // Don't destroy
    }
    break;

  case PROJECTILE_BEHAVIOR_EXPLODE:
    projectile_create_explosion(world, hit_point, proj->explosion_radius,
                                proj->explosion_damage, proj->source);
    break;

  case PROJECTILE_BEHAVIOR_STICK:
    // TODO: Attach to hit entity
    break;

  case PROJECTILE_BEHAVIOR_BOUNCE:
    // TODO: Reflect velocity
    break;
  }

  // Deal damage to hit entity
  if (hit_entity.id != 0 && proj->behavior != PROJECTILE_BEHAVIOR_EXPLODE) {
    damage_event_emit_simple(proj->source, hit_entity, proj->damage);
  }
}

void projectile_create_explosion(World *world, Vec3 position, f32 radius,
                                 f32 damage, Entity source) {
  if (!world)
    return;

  // Query all entities in radius
  // TODO: Implement spatial query
  // For now, simplified version

  // Deal damage to all entities in radius
  // This would need proper spatial partitioning
  (void)position;
  (void)radius;
  (void)damage;
  (void)source;
}
