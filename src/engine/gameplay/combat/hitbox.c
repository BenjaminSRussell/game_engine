// hitbox.c - Hitbox System Implementation
#include <include/gameplay/combat/hitbox.h>
#include <include/gameplay/combat/damage.h>
#include <include/core/logger.h>
#include <include/ecs/component_ids.h>
#include <include/ecs/ecs.h>
#include <include/math/vec3.h>
#include <include/math/aabb.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// TimerComponent implementation for hitbox timing
typedef struct {
  f32 duration;
  f32 time_remaining;
  bool is_active;
  bool loops;
  f32 elapsed_time;
} TimerComponent;

// Timer management functions
TimerComponent* timer_create(f32 duration, bool loops) {
  TimerComponent* timer = malloc(sizeof(TimerComponent));
  if (timer) {
    timer->duration = duration;
    timer->time_remaining = duration;
    timer->is_active = true;
    timer->loops = loops;
    timer->elapsed_time = 0.0f;
  }
  return timer;
}

void timer_update(TimerComponent* timer, f32 delta_time) {
  if (!timer || !timer->is_active) return;
  
  timer->elapsed_time += delta_time;
  timer->time_remaining -= delta_time;
  
  if (timer->time_remaining <= 0.0f) {
    if (timer->loops) {
      timer->time_remaining = timer->duration - timer->time_remaining;
    } else {
      timer->time_remaining = 0.0f;
      timer->is_active = false;
    }
  }
}

void timer_reset(TimerComponent* timer) {
  if (timer) {
    timer->time_remaining = timer->duration;
    timer->elapsed_time = 0.0f;
    timer->is_active = true;
  }
}

void timer_set_duration(TimerComponent* timer, f32 duration) {
  if (timer) {
    timer->duration = duration;
    if (timer->time_remaining > duration) {
      timer->time_remaining = duration;
    }
  }
}

f32 timer_get_progress(const TimerComponent* timer) {
  if (!timer || timer->duration <= 0.0f) return 0.0f;
  return 1.0f - (timer->time_remaining / timer->duration);
}

bool timer_is_expired(const TimerComponent* timer) {
  return timer ? !timer->is_active && timer->time_remaining <= 0.0f : true;
}

void timer_destroy(TimerComponent* timer) {
  if (timer) {
    free(timer);
  }
}

// Forward declarations - removed HitboxInstance since it's defined locally

// Internal hit tracking for hitboxes to prevent double-hits
typedef struct {
  Entity hitbox_entity;
  Entity hit_entities[64];
  u32 hit_count;
} HitboxHitTracker;

static HitboxHitTracker g_hitbox_trackers[MAX_HITBOXES] = {0};

#define MAX_HITBOXES 1024
#define HITBOX_QUERY_BUFFER_SIZE 256
#define HITBOX_COLLISION_THRESHOLD 0.001f

typedef struct {
  Entity entity;
  HitboxComponent hitbox;
  Vec3 last_position;
  bool is_active;
  f64 creation_time;
  f64 destruction_time;
} HitboxInstance;

// Forward declaration for hitbox_handle_collision
static void hitbox_handle_collision(World *world, HitboxInstance *a, HitboxInstance *b, const HitboxCollision *collision);

typedef struct {
  HitboxInstance instances[MAX_HITBOXES];
  u32 instance_count;
  bool is_initialized;
  f64 current_time;
} HitboxSystem;

static HitboxSystem g_hitbox_system = {0};

// Helper functions
static bool sphere_vs_sphere(const Vec3 *center1, f32 radius1, const Vec3 *center2, f32 radius2) {
  Vec3 diff = vec3_sub(*center1, *center2);
  f32 distance_sq = vec3_dot(diff, diff);
  f32 combined_radius = radius1 + radius2;
  return distance_sq <= combined_radius * combined_radius;
}

static bool sphere_vs_box(const Vec3 *sphere_center, f32 sphere_radius, const Vec3 *box_center, const Vec3 *box_half_extents) {
  Vec3 closest_point = {
    fmaxf(box_center->x - box_half_extents->x, fminf(sphere_center->x, box_center->x + box_half_extents->x)),
    fmaxf(box_center->y - box_half_extents->y, fminf(sphere_center->y, box_center->y + box_half_extents->y)),
    fmaxf(box_center->z - box_half_extents->z, fminf(sphere_center->z, box_center->z + box_half_extents->z))
  };
  
  Vec3 diff = vec3_sub(*sphere_center, closest_point);
  f32 distance_sq = vec3_dot(diff, diff);
  return distance_sq <= sphere_radius * sphere_radius;
}

static bool box_vs_box(const Vec3 *center1, const Vec3 *half_extents1, const Vec3 *center2, const Vec3 *half_extents2) {
  Vec3 diff = vec3_sub(*center1, *center2);
  return fabsf(diff.x) <= (half_extents1->x + half_extents2->x) &&
         fabsf(diff.y) <= (half_extents1->y + half_extents2->y) &&
         fabsf(diff.z) <= (half_extents1->z + half_extents2->z);
}

static bool capsule_vs_sphere(const Vec3 *capsule_center, f32 capsule_radius, f32 capsule_height,
                             const Vec3 *sphere_center, f32 sphere_radius) {
  // Simplified capsule-sphere collision
  Vec3 capsule_top = {capsule_center->x, capsule_center->y + capsule_height * 0.5f, capsule_center->z};
  Vec3 capsule_bottom = {capsule_center->x, capsule_center->y - capsule_height * 0.5f, capsule_center->z};
  
  // Check sphere against capsule line segment
  Vec3 ab = vec3_sub(capsule_top, capsule_bottom);
  Vec3 ap = vec3_sub(*sphere_center, capsule_bottom);
  
  f32 t = fmaxf(0.0f, fminf(1.0f, vec3_dot(ap, ab) / vec3_dot(ab, ab)));
  Vec3 closest_point = vec3_add(capsule_bottom, vec3_scale(ab, vec3(t, t, t)));
  
  Vec3 diff = vec3_sub(*sphere_center, closest_point);
  f32 distance_sq = vec3_dot(diff, diff);
  f32 combined_radius = capsule_radius + sphere_radius;
  
  return distance_sq <= combined_radius * combined_radius;
}

static HitboxInstance* hitbox_find_instance(Entity entity) {
  for (u32 i = 0; i < g_hitbox_system.instance_count; i++) {
    if (g_hitbox_system.instances[i].entity.id == entity.id) {
      return &g_hitbox_system.instances[i];
    }
  }
  return NULL;
}

// Helper: Check if hitbox has already hit this entity (prevent double-hits)
static bool hitbox_has_hit_entity(Entity hitbox_entity, Entity target_entity) {
  for (u32 i = 0; i < MAX_HITBOXES; i++) {
    if (g_hitbox_trackers[i].hitbox_entity.id == hitbox_entity.id) {
      for (u32 j = 0; j < g_hitbox_trackers[i].hit_count; j++) {
        if (g_hitbox_trackers[i].hit_entities[j].id == target_entity.id) {
          return true;
        }
      }
      return false;
    }
  }
  return false;
}

// Helper: Mark entity as hit by this hitbox
static void hitbox_mark_hit_entity(Entity hitbox_entity, Entity target_entity) {
  for (u32 i = 0; i < MAX_HITBOXES; i++) {
    if (g_hitbox_trackers[i].hitbox_entity.id == hitbox_entity.id) {
      if (g_hitbox_trackers[i].hit_count < 64) {
        g_hitbox_trackers[i].hit_entities[g_hitbox_trackers[i].hit_count++] = target_entity;
      }
      return;
    }
  }

  // Create new tracker if not found
  for (u32 i = 0; i < MAX_HITBOXES; i++) {
    if (g_hitbox_trackers[i].hitbox_entity.id == 0) {
      g_hitbox_trackers[i].hitbox_entity = hitbox_entity;
      g_hitbox_trackers[i].hit_entities[0] = target_entity;
      g_hitbox_trackers[i].hit_count = 1;
      return;
    }
  }
}

// Helper: Clear hit list for hitbox
static void hitbox_clear_hits(Entity hitbox_entity) {
  for (u32 i = 0; i < MAX_HITBOXES; i++) {
    if (g_hitbox_trackers[i].hitbox_entity.id == hitbox_entity.id) {
      memset(&g_hitbox_trackers[i], 0, sizeof(HitboxHitTracker));
      return;
    }
  }
}

static void hitbox_remove_instance(u32 index) {
  if (index >= g_hitbox_system.instance_count) return;
  
  // Move last instance to this position
  if (index < g_hitbox_system.instance_count - 1) {
    g_hitbox_system.instances[index] = g_hitbox_system.instances[g_hitbox_system.instance_count - 1];
  }
  
  g_hitbox_system.instance_count--;
}

static void hitbox_update_world_position_instance(HitboxInstance *instance) {
  if (!instance) return;
  
  // Get entity position (simplified - would use transform component)
  // For now, we'll just focus on the HitboxComponent

  // Update hitbox world positions, check for triggers, etc.
  Vec3 entity_position = instance->last_position; // Use cached position for now
  
  // Calculate world position
  instance->hitbox.world_position = vec3_add(entity_position, instance->hitbox.offset);
  instance->last_position = entity_position;
}

static bool hitbox_check_collision(const HitboxInstance *a, const HitboxInstance *b, HitboxCollision *out_collision) {
  if (!a || !b || !out_collision) return false;
  
  // Skip if same team and friendly fire is disabled
  if (a->hitbox.team_id == b->hitbox.team_id && a->hitbox.team_id != 0) {
    return false;
  }
  
  // Skip if either hitbox is inactive
  if (!a->hitbox.active || !b->hitbox.active) {
    return false;
  }
  
  bool collision = false;
  Vec3 contact_point = {0.0f, 0.0f, 0.0f};
  Vec3 contact_normal = {0.0f, 0.0f, 0.0f};
  f32 penetration_depth = 0.0f;
  
  // Check collision based on shape types
  switch (a->hitbox.shape) {
    case HITBOX_SHAPE_SPHERE:
      switch (b->hitbox.shape) {
        case HITBOX_SHAPE_SPHERE:
          collision = sphere_vs_sphere(&a->hitbox.world_position, a->hitbox.data.sphere.radius,
                                      &b->hitbox.world_position, b->hitbox.data.sphere.radius);
          if (collision) {
            contact_point = vec3_scale(vec3_add(a->hitbox.world_position, b->hitbox.world_position), vec3(0.5f, 0.5f, 0.5f));
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = a->hitbox.data.sphere.radius + b->hitbox.data.sphere.radius - 
                              vec3_distance(a->hitbox.world_position, b->hitbox.world_position);
          }
          break;
          
        case HITBOX_SHAPE_BOX:
          collision = sphere_vs_box(&a->hitbox.world_position, a->hitbox.data.sphere.radius,
                                     &b->hitbox.world_position, &b->hitbox.data.box.half_extents);
          if (collision) {
            contact_point = b->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(a->hitbox.world_position, b->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = a->hitbox.data.sphere.radius;
          }
          break;
          
        case HITBOX_SHAPE_CAPSULE:
          collision = capsule_vs_sphere(&b->hitbox.world_position, b->hitbox.data.capsule.radius, b->hitbox.data.capsule.height,
                                       &a->hitbox.world_position, a->hitbox.data.sphere.radius);
          if (collision) {
            contact_point = a->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = a->hitbox.data.sphere.radius;
          }
          break;
      }
      break;
      
    case HITBOX_SHAPE_BOX:
      switch (b->hitbox.shape) {
        case HITBOX_SHAPE_SPHERE:
          collision = sphere_vs_box(&b->hitbox.world_position, b->hitbox.data.sphere.radius,
                                     &a->hitbox.world_position, &a->hitbox.data.box.half_extents);
          if (collision) {
            contact_point = a->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = b->hitbox.data.sphere.radius;
          }
          break;
          
        case HITBOX_SHAPE_BOX:
          collision = box_vs_box(&a->hitbox.world_position, &a->hitbox.data.box.half_extents,
                                 &b->hitbox.world_position, &b->hitbox.data.box.half_extents);
          if (collision) {
            contact_point = vec3_scale(vec3_add(a->hitbox.world_position, b->hitbox.world_position), vec3(0.5f, 0.5f, 0.5f));
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = 1.0f; // Simplified
          }
          break;
          
        case HITBOX_SHAPE_CAPSULE:
          // Box vs capsule (simplified)
          collision = sphere_vs_box(&b->hitbox.world_position, b->hitbox.data.capsule.radius,
                                     &a->hitbox.world_position, &a->hitbox.data.box.half_extents);
          if (collision) {
            contact_point = a->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = b->hitbox.data.capsule.radius;
          }
          break;
      }
      break;
      
    case HITBOX_SHAPE_CAPSULE:
      switch (b->hitbox.shape) {
        case HITBOX_SHAPE_SPHERE:
          collision = capsule_vs_sphere(&a->hitbox.world_position, a->hitbox.data.capsule.radius, a->hitbox.data.capsule.height,
                                       &b->hitbox.world_position, b->hitbox.data.sphere.radius);
          if (collision) {
            contact_point = b->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(a->hitbox.world_position, b->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = b->hitbox.data.sphere.radius;
          }
          break;
          
        case HITBOX_SHAPE_BOX:
          // Capsule vs box (simplified)
          collision = sphere_vs_box(&a->hitbox.world_position, a->hitbox.data.capsule.radius,
                                     &b->hitbox.world_position, &b->hitbox.data.box.half_extents);
          if (collision) {
            contact_point = b->hitbox.world_position;
            Vec3 normal = vec3_normalize(vec3_sub(a->hitbox.world_position, b->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = a->hitbox.data.capsule.radius;
          }
          break;
          
        case HITBOX_SHAPE_CAPSULE:
          // Capsule vs capsule (simplified)
          collision = sphere_vs_sphere(&a->hitbox.world_position, a->hitbox.data.capsule.radius,
                                      &b->hitbox.world_position, b->hitbox.data.capsule.radius);
          if (collision) {
            contact_point = vec3_scale(vec3_add(a->hitbox.world_position, b->hitbox.world_position), vec3(0.5f, 0.5f, 0.5f));
            Vec3 normal = vec3_normalize(vec3_sub(b->hitbox.world_position, a->hitbox.world_position));
            contact_normal = normal;
            penetration_depth = a->hitbox.data.capsule.radius + b->hitbox.data.capsule.radius - 
                              vec3_distance(a->hitbox.world_position, b->hitbox.world_position);
          }
          break;
      }
      break;
  }
  
  if (collision && penetration_depth > HITBOX_COLLISION_THRESHOLD) {
    out_collision->hit = true;
    out_collision->contact_point = contact_point;
    out_collision->contact_normal = contact_normal;
    out_collision->penetration_depth = penetration_depth;
    out_collision->hit_entity = b->entity;
    return true;
  }
  
  return false;
}

// Public API
bool hitbox_system_init(World *world) {
  if (g_hitbox_system.is_initialized) {
    LOG_WARN("Hitbox system already initialized");
    return true;
  }
  
  memset(&g_hitbox_system, 0, sizeof(HitboxSystem));
  g_hitbox_system.is_initialized = true;
  g_hitbox_system.current_time = 0.0;
  
  LOG_INFO("Hitbox system initialized");
  return true;
}

void hitbox_system_shutdown(void) {
  if (!g_hitbox_system.is_initialized) return;
  
  memset(&g_hitbox_system, 0, sizeof(HitboxSystem));
  g_hitbox_system.is_initialized = false;
  
  LOG_INFO("Hitbox system shutdown");
}

void hitbox_system_update(World *world, f32 delta_time) {
  if (!g_hitbox_system.is_initialized) return;

  g_hitbox_system.current_time += delta_time;

  // Update all hitbox instances
  for (u32 i = 0; i < g_hitbox_system.instance_count; i++) {
    HitboxInstance *instance = &g_hitbox_system.instances[i];

    // Update world position
    hitbox_update_world_position_instance(instance);

    // Check for temporary hitbox destruction
    if (instance->destruction_time > 0.0 && g_hitbox_system.current_time >= instance->destruction_time) {
      // Clear hit tracking for this hitbox
      hitbox_clear_hits(instance->entity);
      // Destroy the entity
      ecs_destroy_entity(world, instance->entity);
      hitbox_remove_instance(i);
      i--; // Adjust index since we removed an element
      continue;
    }
  }

  // Perform collision detection
  for (u32 i = 0; i < g_hitbox_system.instance_count; i++) {
    HitboxInstance *instance_a = &g_hitbox_system.instances[i];

    if (!instance_a->hitbox.active) continue;

    for (u32 j = i + 1; j < g_hitbox_system.instance_count; j++) {
      HitboxInstance *instance_b = &g_hitbox_system.instances[j];

      if (!instance_b->hitbox.active) continue;

      HitboxCollision collision;
      if (hitbox_check_collision(instance_a, instance_b, &collision)) {
        // Handle collision
        hitbox_handle_collision(world, instance_a, instance_b, &collision);
      }
    }
  }
}

void hitbox_handle_collision(World *world, HitboxInstance *a, HitboxInstance *b, const HitboxCollision *collision) {
  if (!world || !a || !b || !collision) return;

  LOG_DEBUG("Hitbox collision: entity %u hit entity %u", a->entity.id, b->entity.id);

  // Apply damage if this is a trigger hitbox (e.g., attack hitbox)
  if (a->hitbox.is_trigger) {
    // Check if this hitbox has already hit target b
    if (!hitbox_has_hit_entity(a->entity, b->entity)) {
      // Calculate damage with multiplier
      f32 base_damage = 10.0f;
      f32 final_damage = base_damage * a->hitbox.damage_multiplier;

      // Create damage event from hitbox owner to target
      Entity source = a->entity;
      Entity target = b->entity;
      damage_event_create(source, target, final_damage, DAMAGE_TYPE_PHYSICAL);

      // Mark that this hitbox has hit target b
      hitbox_mark_hit_entity(a->entity, b->entity);

      LOG_DEBUG("Hitbox %u dealt %.1f damage to entity %u (multiplier: %.1f)",
               a->entity.id, final_damage, b->entity.id, a->hitbox.damage_multiplier);
    }
  }

  // Apply damage if target hitbox is also a trigger (two-way collision)
  if (b->hitbox.is_trigger) {
    // Check if this hitbox has already hit target a
    if (!hitbox_has_hit_entity(b->entity, a->entity)) {
      // Calculate damage with multiplier
      f32 base_damage = 10.0f;
      f32 final_damage = base_damage * b->hitbox.damage_multiplier;

      // Create damage event from hitbox owner to target
      Entity source = b->entity;
      Entity target = a->entity;
      damage_event_create(source, target, final_damage, DAMAGE_TYPE_PHYSICAL);

      // Mark that this hitbox has hit target a
      hitbox_mark_hit_entity(b->entity, a->entity);

      LOG_DEBUG("Hitbox %u dealt %.1f damage to entity %u (multiplier: %.1f)",
               b->entity.id, final_damage, a->entity.id, b->hitbox.damage_multiplier);
    }
  }
}

Entity hitbox_create_temporary(World *world, Vec3 position, Vec3 direction, f32 range, f32 duration) {
  if (!world || g_hitbox_system.instance_count >= MAX_HITBOXES) {
    return INVALID_ENTITY;
  }
  
  Entity entity = ecs_create_entity(world);
  if (entity.id == 0) {
    return INVALID_ENTITY;
  }
  
  // Create hitbox component
  HitboxComponent hitbox = hitbox_create_sphere(range, 0); // Team 0 for neutral
  hitbox.active = true;
  hitbox.is_trigger = true;
  hitbox.world_position = position;
  hitbox.last_update_time = g_hitbox_system.current_time;
  
  // Add hitbox component to entity
  ecs_add_component(world, entity, HITBOX_COMPONENT_ID, &hitbox);
  
  // Create hitbox instance
  HitboxInstance *instance = &g_hitbox_system.instances[g_hitbox_system.instance_count++];
  memset(instance, 0, sizeof(HitboxInstance));
  
  instance->entity = entity;
  instance->hitbox = hitbox;
  instance->last_position = position;
  instance->is_active = true;
  instance->creation_time = g_hitbox_system.current_time;
  instance->destruction_time = g_hitbox_system.current_time + duration;
  
  // Add timer component for automatic destruction
  // TODO: Implement TimerComponent
  
  LOG_DEBUG("Created temporary hitbox entity %u (range: %.2f, duration: %.2f)", 
           entity.id, range, duration);
  
  return entity;
}

HitboxComponent hitbox_create_sphere(f32 radius, u32 team_id) {
  HitboxComponent hb = {0};
  hb.shape = HITBOX_SHAPE_SPHERE;
  hb.data.sphere.radius = fmaxf(0.001f, radius);
  hb.team_id = team_id;
  hb.active = true;
  hb.is_trigger = false;
  hb.damage_multiplier = 1.0f;
  hb.offset = (Vec3){0.0f, 0.0f, 0.0f};
  hb.world_position = (Vec3){0.0f, 0.0f, 0.0f};
  hb.last_update_time = g_hitbox_system.current_time;
  
  return hb;
}

HitboxComponent hitbox_create_box(Vec3 half_extents, u32 team_id) {
  HitboxComponent hb = {0};
  hb.shape = HITBOX_SHAPE_BOX;
  hb.data.box.half_extents = half_extents;
  hb.team_id = team_id;
  hb.active = true;
  hb.is_trigger = false;
  hb.damage_multiplier = 1.0f;
  hb.offset = (Vec3){0.0f, 0.0f, 0.0f};
  hb.world_position = (Vec3){0.0f, 0.0f, 0.0f};
  hb.last_update_time = g_hitbox_system.current_time;
  
  return hb;
}

HitboxComponent hitbox_create_capsule(f32 radius, f32 height, u32 team_id) {
  HitboxComponent hb = {0};
  hb.shape = HITBOX_SHAPE_CAPSULE;
  hb.data.capsule.radius = fmaxf(0.001f, radius);
  hb.data.capsule.height = fmaxf(0.001f, height);
  hb.team_id = team_id;
  hb.active = true;
  hb.is_trigger = false;
  hb.damage_multiplier = 1.0f;
  hb.offset = (Vec3){0.0f, 0.0f, 0.0f};
  hb.world_position = (Vec3){0.0f, 0.0f, 0.0f};
  hb.last_update_time = g_hitbox_system.current_time;
  
  return hb;
}

void hitbox_activate(HitboxComponent *hitbox) {
  if (!hitbox) return;
  
  hitbox->active = true;
  hitbox->last_update_time = g_hitbox_system.current_time;
  
  LOG_DEBUG("Activated hitbox");
}

void hitbox_deactivate(HitboxComponent *hitbox) {
  if (hitbox)
    hitbox->active = false;
}
void hitbox_set_active(HitboxComponent *hitbox, bool active) {
  if (hitbox)
    hitbox->active = active;
}
