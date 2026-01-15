// projectile.c - Projectile System Implementation
#include <include/core/logger.h>
#include <include/ecs/component_ids.h>
#include <include/ecs/ecs.h>
#include <include/gameplay/combat/damage.h>
#include <include/gameplay/combat/projectile.h>
#include <include/math/vec3.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Include status effects for projectile effects
#include "status_effects.h"

#define MAX_PROJECTILES 2048
#define PROJECTILE_UPDATE_BATCH_SIZE 64
#define PROJECTILE_RAYCAST_STEPS 8
#define PROJECTILE_MIN_LIFETIME 0.1f
#define PROJECTILE_MAX_SPEED 1000.0f
#define PROJECTILE_DEFAULT_GRAVITY -9.81f

typedef struct {
  Entity entity;
  ProjectileComponent projectile;
  Vec3 previous_positions[PROJECTILE_RAYCAST_STEPS];
  u32 position_index;
  bool is_active;
  f64 creation_time;
} ProjectileInstance;

typedef struct {
  ProjectileInstance instances[MAX_PROJECTILES];
  u32 instance_count;
  bool is_initialized;
  f64 current_time;
  Vec3 gravity;
} ProjectileSystem;

static ProjectileSystem g_projectile_system = {0};

// Helper functions
static Vec3 calculate_homing_direction(const Vec3 *current_pos,
                                       const Vec3 *target_pos,
                                       const Vec3 *current_vel,
                                       f32 homing_strength) {
  Vec3 to_target = vec3_sub(*target_pos, *current_pos);
  Vec3 desired_direction = vec3_normalize(to_target);

  // Interpolate between current velocity direction and desired direction
  Vec3 current_direction = vec3_normalize(*current_vel);
  Vec3 homing_direction =
      vec3_lerp(current_direction, desired_direction, homing_strength);

  return vec3_normalize(homing_direction);
}

static bool projectile_raycast(const Vec3 *start, const Vec3 *end,
                               Entity source_entity, Vec3 *out_hit_point,
                               Vec3 *out_hit_normal, Entity *out_hit_entity) {
  // Simplified raycast - would integrate with physics system
  // For now, we'll just check if we hit the ground (y = 0)
  if (start->y > 0.0f && end->y <= 0.0f) {
    // Calculate intersection with ground plane
    f32 t = start->y / (start->y - end->y);
    out_hit_point->x = start->x + t * (end->x - start->x);
    out_hit_point->y = 0.0f;
    out_hit_point->z = start->z + t * (end->z - start->z);
    out_hit_normal->x = 0.0f;
    out_hit_normal->y = 1.0f;
    out_hit_normal->z = 0.0f;
    *out_hit_entity = INVALID_ENTITY;
    return true;
  }

  return false;
}

static void projectile_apply_explosion(const Vec3 *position, f32 radius,
                                       f32 damage, Entity source) {
  // Find all entities in explosion radius
  // This would integrate with the hitbox system
  LOGD("Explosion at (%.2f, %.2f, %.2f) radius %.2f damage %.1f", position->x,
       position->y, position->z, radius, damage);

  // For now, just log the explosion
  // In a full implementation, this would:
  // 1. Query for entities in radius using hitbox system
  // 2. Apply damage with falloff if enabled
  // 3. Apply knockback forces
  // 4. Create visual effects
}

static void projectile_handle_collision(ProjectileInstance *instance,
                                        const Vec3 *hit_point,
                                        const Vec3 *hit_normal,
                                        Entity hit_entity) {
  if (!instance || !hit_point || !hit_normal)
    return;

  ProjectileComponent *proj = &instance->projectile;
  proj->has_collided = true;
  proj->last_hit_entity = hit_entity;

  LOGD("Projectile collision: entity %u hit entity %u at (%.2f, %.2f, %.2f)",
       instance->entity.id, hit_entity.id, hit_point->x, hit_point->y,
       hit_point->z);

  switch (proj->behavior) {
  case PROJECTILE_BEHAVIOR_DESTROY:
    // Destroy projectile immediately
    instance->is_active = false;
    break;

  case PROJECTILE_BEHAVIOR_PIERCE:
    // Continue if we haven't exceeded penetration count
    proj->current_penetrations++;
    if (proj->current_penetrations >= proj->penetration_count) {
      instance->is_active = false;
    }
    break;

  case PROJECTILE_BEHAVIOR_BOUNCE:
    // Reflect velocity
    Vec3 reflected = vec3_reflect(proj->velocity, *hit_normal);
    proj->velocity =
        vec3_scale(reflected, vec3(0.8f, 0.8f, 0.8f)); // Energy loss on bounce
    break;

  case PROJECTILE_BEHAVIOR_EXPLODE:
    // Create explosion
    projectile_apply_explosion(hit_point, proj->explosion_radius,
                               proj->explosion_damage, proj->source);
    instance->is_active = false;
    break;

  case PROJECTILE_BEHAVIOR_STICK:
    // Stop projectile and stick to surface
    proj->velocity = vec3(0.0f, 0.0f, 0.0f);
    break;
  }

  // Apply damage to hit entity
  if (hit_entity.id != 0) {
    // Create damage event and emit to damage system
    damage_event_create(proj->source, hit_entity, proj->damage,
                        proj->damage_type);

    LOGD("Applied %.1f damage (type: %d) to entity %u", proj->damage,
         proj->damage_type, hit_entity.id);

    // Apply type-specific effects based on projectile type
    if (proj->damage_type == DAMAGE_TYPE_FIRE) {
      // Apply burning effect for fire projectiles
      status_sys_apply_effect_with_source(hit_entity.id, EFFECT_BURNING, 5.0f,
                                          1.0f, proj->source.id);
      LOGD("Applied BURNING effect to entity %u from fire projectile",
           hit_entity.id);
    } else if (proj->damage_type == DAMAGE_TYPE_ICE) {
      // Apply freezing effect for ice projectiles
      status_sys_apply_effect_with_source(hit_entity.id, EFFECT_FREEZING, 3.0f,
                                          0.5f, proj->source.id);
      LOGD("Applied FREEZING effect to entity %u from ice projectile",
           hit_entity.id);
    } else if (proj->damage_type == DAMAGE_TYPE_POISON) {
      // Apply poison effect for poison projectiles
      status_sys_apply_effect_with_source(hit_entity.id, EFFECT_POISON, 10.0f,
                                          0.5f, proj->source.id);
      LOGD("Applied POISON effect to entity %u from poison projectile",
           hit_entity.id);
    }
  }
}

static void projectile_update_physics(ProjectileInstance *instance,
                                      f32 delta_time) {
  if (!instance)
    return;

  ProjectileComponent *proj = &instance->projectile;

  // Store previous position for raycast
  instance->previous_positions[instance->position_index] = proj->last_position;
  instance->position_index =
      (instance->position_index + 1) % PROJECTILE_RAYCAST_STEPS;

  // Apply gravity
  if (proj->gravity_scale > 0.0f) {
    Vec3 gravity_force = vec3_scale(
        g_projectile_system.gravity,
        vec3(proj->gravity_scale, proj->gravity_scale, proj->gravity_scale));
    proj->velocity = vec3_add(
        proj->velocity,
        vec3_scale(gravity_force, vec3(delta_time, delta_time, delta_time)));
  }

  // Apply drag
  if (proj->drag > 0.0f) {
    f32 drag_factor = 1.0f - (proj->drag * delta_time);
    drag_factor = fmaxf(0.0f, drag_factor);
    proj->velocity =
        vec3_scale(proj->velocity, vec3(drag_factor, drag_factor, drag_factor));
  }

  // Clamp to max speed
  f32 current_speed = vec3_length(proj->velocity);
  if (current_speed > proj->max_speed) {
    proj->velocity =
        vec3_scale(vec3_normalize(proj->velocity),
                   vec3(proj->max_speed, proj->max_speed, proj->max_speed));
  }

  // Update position
  proj->last_position =
      proj->last_position; // Would get from transform component
  Vec3 new_position = vec3_add(
      proj->last_position,
      vec3_scale(proj->velocity, vec3(delta_time, delta_time, delta_time)));

  // Handle homing
  if (proj->is_homing && proj->homing_target.id != 0 &&
      proj->age >= proj->homing_delay) {
    // Get target position (simplified - would use transform component)
    Vec3 target_position = {0.0f, 0.0f, 0.0f}; // Placeholder

    Vec3 homing_dir =
        calculate_homing_direction(&proj->last_position, &target_position,
                                   &proj->velocity, proj->homing_strength);
    proj->velocity = vec3_scale(
        homing_dir, vec3(current_speed, current_speed, current_speed));
  }

  // Perform raycast collision detection
  if (proj->check_terrain || proj->collision_radius > 0.0f) {
    Vec3 hit_point, hit_normal;
    Entity hit_entity;

    if (projectile_raycast(&proj->last_position, &new_position, proj->source,
                           &hit_point, &hit_normal, &hit_entity)) {
      projectile_handle_collision(instance, &hit_point, &hit_normal,
                                  hit_entity);
    }
  }

  // Update age
  proj->age += delta_time;

  // Check lifetime
  if (proj->age >= proj->lifetime) {
    instance->is_active = false;
    LOGD("Projectile %u expired after %.2f seconds", instance->entity.id,
         proj->age);
  }
}

static ProjectileInstance *projectile_find_instance(Entity entity) {
  for (u32 i = 0; i < g_projectile_system.instance_count; i++) {
    if (g_projectile_system.instances[i].entity.id == entity.id) {
      return &g_projectile_system.instances[i];
    }
  }
  return NULL;
}

static void projectile_remove_instance(u32 index) {
  if (index >= g_projectile_system.instance_count)
    return;

  // Move last instance to this position
  if (index < g_projectile_system.instance_count - 1) {
    g_projectile_system.instances[index] =
        g_projectile_system.instances[g_projectile_system.instance_count - 1];
  }

  g_projectile_system.instance_count--;
}

// Public API
bool projectile_system_init(World *world) {
  if (g_projectile_system.is_initialized) {
    LOGW("Projectile system already initialized");
    return true;
  }

  memset(&g_projectile_system, 0, sizeof(ProjectileSystem));
  g_projectile_system.is_initialized = true;
  g_projectile_system.current_time = 0.0;
  g_projectile_system.gravity = vec3(0.0f, PROJECTILE_DEFAULT_GRAVITY, 0.0f);

  LOGI("Projectile system initialized");
  return true;
}

void projectile_system_shutdown(void) {
  if (!g_projectile_system.is_initialized)
    return;

  memset(&g_projectile_system, 0, sizeof(ProjectileSystem));
  g_projectile_system.is_initialized = false;

  LOGI("Projectile system shutdown");
}

void projectile_system_update(World *world, f32 delta_time) {
  if (!g_projectile_system.is_initialized)
    return;

  g_projectile_system.current_time += delta_time;

  // Update all projectile instances
  for (u32 i = 0; i < g_projectile_system.instance_count; i++) {
    ProjectileInstance *instance = &g_projectile_system.instances[i];

    if (!instance->is_active) {
      // Destroy the projectile entity
      ecs_destroy_entity(world, instance->entity);
      projectile_remove_instance(i);
      i--; // Adjust index since we removed an element
      continue;
    }

    projectile_update_physics(instance, delta_time);
  }
}

Entity projectile_spawn(World *world, Vec3 position, Vec3 direction, f32 speed,
                        Entity source, f32 damage) {
  if (!world || g_projectile_system.instance_count >= MAX_PROJECTILES) {
    return INVALID_ENTITY;
  }

  Entity entity = ecs_create_entity(world);
  if (entity.id == 0) {
    return INVALID_ENTITY;
  }

  // Normalize direction and apply speed
  direction = vec3_normalize(direction);
  Vec3 velocity = vec3_scale(direction, vec3(speed, speed, speed));

  // Create projectile component
  ProjectileComponent proj =
      projectile_create(velocity, 5.0f); // 5 second default lifetime
  proj.speed = speed;
  proj.max_speed = speed * 2.0f;
  proj.source = source;
  proj.damage = damage;
  proj.damage_type = DAMAGE_TYPE_PROJECTILE;
  proj.behavior = PROJECTILE_BEHAVIOR_DESTROY;
  proj.check_terrain = true;
  proj.collision_radius = 0.1f;
  proj.last_position = position;
  proj.gravity_scale = 0.1f; // Light gravity
  proj.drag = 0.01f;         // Light air resistance

  // Add projectile component to entity
  ecs_add_component(world, entity, PROJECTILE_COMPONENT_ID, &proj);

  // Create projectile instance
  ProjectileInstance *instance =
      &g_projectile_system.instances[g_projectile_system.instance_count++];
  memset(instance, 0, sizeof(ProjectileInstance));

  instance->entity = entity;
  instance->projectile = proj;
  instance->position_index = 0;
  instance->is_active = true;
  instance->creation_time = g_projectile_system.current_time;

  // Initialize previous positions
  for (u32 i = 0; i < PROJECTILE_RAYCAST_STEPS; i++) {
    instance->previous_positions[i] = position;
  }

  LOGD("Spawned projectile entity %u (speed: %.1f, damage: %.1f)", entity.id,
       speed, damage);

  return entity;
}

ProjectileComponent projectile_create(Vec3 velocity, f32 lifetime) {
  ProjectileComponent proj = {0};
  proj.velocity = velocity;
  proj.speed = vec3_length(velocity);
  proj.max_speed = proj.speed * 2.0f;
  proj.lifetime = fmaxf(PROJECTILE_MIN_LIFETIME, lifetime);
  proj.age = 0.0f;
  proj.behavior = PROJECTILE_BEHAVIOR_DESTROY;
  proj.penetration_count = 1;
  proj.current_penetrations = 0;
  proj.collision_radius = 0.1f;
  proj.check_terrain = true;
  proj.gravity_scale = 1.0f;
  proj.drag = 0.0f;
  proj.damage = 10.0f;
  proj.damage_type = DAMAGE_TYPE_PROJECTILE;
  proj.knockback = 0.0f;
  proj.trail_length = 1.0f;
  proj.emit_light = false;
  proj.light_color = (Vec3){1.0f, 1.0f, 1.0f};
  proj.explosion_radius = 0.0f;
  proj.explosion_damage = 0.0f;
  proj.explosion_falloff = true;
  proj.last_position = (Vec3){0.0f, 0.0f, 0.0f};
  proj.has_collided = false;
  proj.last_hit_entity = INVALID_ENTITY;

  return proj;
}

ProjectileComponent projectile_create_arrow(Vec3 velocity, f32 damage) {
  ProjectileComponent proj =
      projectile_create(velocity, 3.0f); // 3 second lifetime
  proj.damage = damage;
  proj.damage_type = DAMAGE_TYPE_PHYSICAL;
  proj.behavior = PROJECTILE_BEHAVIOR_STICK;
  proj.gravity_scale = 0.5f;     // Arrows affected by gravity
  proj.drag = 0.02f;             // More drag for arrows
  proj.collision_radius = 0.05f; // Smaller collision radius
  proj.trail_length = 0.5f;

  return proj;
}

ProjectileComponent projectile_create_fireball(Vec3 velocity, f32 damage,
                                               f32 explosion_radius) {
  ProjectileComponent proj =
      projectile_create(velocity, 2.0f); // 2 second lifetime
  proj.damage = damage;
  proj.damage_type = DAMAGE_TYPE_FIRE;
  proj.behavior = PROJECTILE_BEHAVIOR_EXPLODE;
  proj.gravity_scale = 0.0f;    // No gravity for fireball
  proj.drag = 0.005f;           // Less drag
  proj.collision_radius = 0.3f; // Larger collision radius
  proj.explosion_radius = explosion_radius;
  proj.explosion_damage = damage * 0.5f; // Explosion does half damage
  proj.explosion_falloff = true;
  proj.emit_light = true;
  proj.light_color = (Vec3){1.0f, 0.5f, 0.0f}; // Orange light
  proj.trail_length = 2.0f;

  return proj;
}

ProjectileComponent projectile_create_homing(Vec3 velocity, Entity target,
                                             f32 damage) {
  ProjectileComponent proj =
      projectile_create(velocity, 5.0f); // 5 second lifetime
  proj.damage = damage;
  proj.damage_type = DAMAGE_TYPE_MAGIC;
  proj.behavior = PROJECTILE_BEHAVIOR_DESTROY;
  proj.gravity_scale = 0.0f; // No gravity for homing projectiles
  proj.drag = 0.01f;
  proj.is_homing = true;
  proj.homing_target = target;
  proj.homing_strength = 0.5f;
  proj.homing_delay = 0.5f; // Start homing after 0.5 seconds
  proj.emit_light = true;
  proj.light_color = vec3(0.5f, 0.0f, 1.0f); // Purple light
  proj.trail_length = 1.5f;

  return proj;
}

void projectile_set_homing(ProjectileComponent *proj, Entity target,
                           f32 strength) {
  if (!proj)
    return;

  proj->is_homing = true;
  proj->homing_target = target;
  proj->homing_strength = fmaxf(0.0f, fminf(1.0f, strength));

  LOGD("Set projectile homing to entity %u with strength %.2f", target.id,
       strength);
}

void projectile_set_gravity(ProjectileComponent *proj, f32 gravity_scale) {
  if (!proj)
    return;

  proj->gravity_scale = fmaxf(0.0f, gravity_scale);

  LOGD("Set projectile gravity scale to %.2f", gravity_scale);
}

void projectile_set_drag(ProjectileComponent *proj, f32 drag) {
  if (!proj)
    return;

  proj->drag = fmaxf(0.0f, fminf(1.0f, drag));

  LOGD("Set projectile drag to %.3f", drag);
}

void projectile_set_behavior(ProjectileComponent *proj,
                             ProjectileBehavior behavior) {
  if (!proj)
    return;

  proj->behavior = behavior;

  LOGD("Set projectile behavior to %d", behavior);
}

void projectile_set_penetration(ProjectileComponent *proj, u32 count) {
  if (!proj)
    return;

  proj->penetration_count = count;
  proj->current_penetrations = 0;

  LOGD("Set projectile penetration count to %u", count);
}

void projectile_set_explosion(ProjectileComponent *proj, f32 radius,
                              f32 damage) {
  if (!proj)
    return;

  proj->explosion_radius = fmaxf(0.0f, radius);
  proj->explosion_damage = fmaxf(0.0f, damage);
  proj->explosion_falloff = true; // Default to enabled

  LOGD("Set projectile explosion: radius %.2f, damage %.1f", radius, damage);
}

void projectile_set_light(ProjectileComponent *proj, bool emit_light,
                          Vec3 color) {
  if (!proj)
    return;

  proj->emit_light = emit_light;
  proj->light_color = color;

  LOGD("Set projectile light: %s, color (%.2f, %.2f, %.2f)",
       emit_light ? "enabled" : "disabled", color.x, color.y, color.z);
}

bool projectile_get_active(Entity entity) {
  ProjectileInstance *instance = projectile_find_instance(entity);
  return instance ? instance->is_active : false;
}

Vec3 projectile_get_position(Entity entity) {
  ProjectileInstance *instance = projectile_find_instance(entity);
  return instance ? instance->projectile.last_position : vec3(0.0f, 0.0f, 0.0f);
}

Vec3 projectile_get_velocity(Entity entity) {
  ProjectileInstance *instance = projectile_find_instance(entity);
  return instance ? instance->projectile.velocity : vec3(0.0f, 0.0f, 0.0f);
}

f32 projectile_get_age(Entity entity) {
  ProjectileInstance *instance = projectile_find_instance(entity);
  return instance ? instance->projectile.age : 0.0f;
}

void projectile_get_statistics(u32 *out_active_projectiles,
                               u32 *out_total_spawned, f32 *out_average_speed) {
  if (!out_active_projectiles || !out_total_spawned || !out_average_speed)
    return;

  *out_active_projectiles = 0;
  *out_total_spawned = 0;
  *out_average_speed = 0.0f;

  f32 total_speed = 0.0f;

  for (u32 i = 0; i < g_projectile_system.instance_count; i++) {
    ProjectileInstance *instance = &g_projectile_system.instances[i];

    if (instance->is_active) {
      (*out_active_projectiles)++;
      total_speed += vec3_length(instance->projectile.velocity);
    }

    (*out_total_spawned)++;
  }

  if (*out_active_projectiles > 0) {
    *out_average_speed = total_speed / (*out_active_projectiles);
  }
}

void projectile_debug_render(World *world) {
  if (!world)
    return;

  // This would integrate with the debug rendering system
  // For now, we'll just log the projectile positions
  LOGD("=== Projectile Debug Render ===");

  for (u32 i = 0; i < g_projectile_system.instance_count; i++) {
    ProjectileInstance *instance = &g_projectile_system.instances[i];

    if (!instance->is_active)
      continue;

    const char *behavior_name = "Unknown";
    switch (instance->projectile.behavior) {
    case PROJECTILE_BEHAVIOR_DESTROY:
      behavior_name = "Destroy";
      break;
    case PROJECTILE_BEHAVIOR_PIERCE:
      behavior_name = "Pierce";
      break;
    case PROJECTILE_BEHAVIOR_BOUNCE:
      behavior_name = "Bounce";
      break;
    case PROJECTILE_BEHAVIOR_EXPLODE:
      behavior_name = "Explode";
      break;
    case PROJECTILE_BEHAVIOR_STICK:
      behavior_name = "Stick";
      break;
    }

    LOGD("Projectile %u: pos (%.2f, %.2f, %.2f) vel (%.2f, %.2f, %.2f) age "
         "%.2f [%s]",
         instance->entity.id, instance->projectile.last_position.x,
         instance->projectile.last_position.y,
         instance->projectile.last_position.z, instance->projectile.velocity.x,
         instance->projectile.velocity.y, instance->projectile.velocity.z,
         instance->projectile.age, behavior_name);
  }

  LOGD("==============================");
}

bool projectile_is_initialized(void) {
  return g_projectile_system.is_initialized;
}
