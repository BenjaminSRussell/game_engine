// include/gameplay/combat/projectile.h
//
// Purpose: Defines projectile component for ranged combat. Projectiles move
// through the world using physics simulation and can detect collisions via
// raycasting. Supports gravity, homing, penetration, and various collision
// behaviors.
//
// Public APIs:
// - ProjectileComponent: Component for projectile entities
// - ProjectileBehavior: Enum for collision behavior (pierce, explode, bounce)
// - Projectile creation and update functions
//
// Ownership: ProjectileComponent instances are owned by the ECS framework
//
// Invariants:
// - lifetime must be positive
// - penetration_count must be non-negative
// - max_speed must be positive
//
#ifndef PROJECTILE_COMPONENT_H
#define PROJECTILE_COMPONENT_H

#include "include/common.h"
#include "include/ecs/ecs.h"
#include "include/math/vec3.h"
#include "include/gameplay/combat/damage.h"
#include "include/gameplay/combat/hitbox.h"

// Projectile collision behavior
typedef enum {
  PROJECTILE_BEHAVIOR_DESTROY, // Destroy on first hit
  PROJECTILE_BEHAVIOR_PIERCE,  // Pass through targets
  PROJECTILE_BEHAVIOR_BOUNCE,  // Bounce off surfaces
  PROJECTILE_BEHAVIOR_EXPLODE, // Explode on impact
  PROJECTILE_BEHAVIOR_STICK,   // Stick to surface
} ProjectileBehavior;

// Projectile component
typedef struct {
  // Movement
  Vec3 velocity;     // Current velocity
  f32 speed;         // Base speed
  f32 max_speed;     // Maximum speed
  f32 gravity_scale; // Gravity multiplier (1.0 = normal, 0.0 = no gravity)
  f32 drag;          // Air resistance (0-1)

  // Tracking/Homing
  bool is_homing;       // Does projectile track targets?
  Entity homing_target; // Target to home in on
  f32 homing_strength;  // How strongly it homes (0-1)
  f32 homing_delay;     // Delay before homing activates

  // Lifetime
  f32 lifetime; // Time before auto-destroy (seconds)
  f32 age;      // Current age (seconds)

  // Collision
  ProjectileBehavior behavior;
  u32 penetration_count;    // How many targets it can pierce
  u32 current_penetrations; // Penetrations so far
  f32 collision_radius;     // Radius for collision detection
  bool check_terrain;       // Collide with terrain?

  // Damage
  Entity source;          // Entity that fired projectile
  f32 damage;             // Damage on hit
  DamageType damage_type; // Type of damage
  f32 knockback;          // Knockback force

  // Visual
  f32 trail_length; // Length of visual trail
  bool emit_light;  // Does it emit light?
  Vec3 light_color; // Light color if emitting

  // Explosion (if PROJECTILE_BEHAVIOR_EXPLODE)
  f32 explosion_radius;   // Radius of explosion
  f32 explosion_damage;   // Damage in explosion center
  bool explosion_falloff; // Does damage fall off with distance?

  // Runtime
  Vec3 last_position;     // Previous position for raycast
  bool has_collided;      // Has hit something this frame?
  Entity last_hit_entity; // Last entity hit (for pierce)
} ProjectileComponent;

// ============================================================================
// PROJECTILE MANAGEMENT
// ============================================================================

// Create projectile components
ProjectileComponent projectile_create(Vec3 velocity, f32 lifetime);
ProjectileComponent projectile_create_arrow(Vec3 velocity, f32 damage);
ProjectileComponent projectile_create_fireball(Vec3 velocity, f32 damage,
                                               f32 explosion_radius);
ProjectileComponent projectile_create_homing(Vec3 velocity, Entity target,
                                             f32 damage);

// Projectile properties
void projectile_set_homing(ProjectileComponent *proj, Entity target,
                           f32 strength);
void projectile_set_pierce(ProjectileComponent *proj, u32 pierce_count);
void projectile_set_explosion(ProjectileComponent *proj, f32 radius,
                              f32 damage);
void projectile_set_gravity(ProjectileComponent *proj, f32 gravity_scale);

// ============================================================================
// PROJECTILE SYSTEM
// ============================================================================

// Initialize projectile system
bool projectile_system_init(World *world);
void projectile_system_shutdown(void);

// Update projectiles (movement, collision, lifetime)
void projectile_system_update(World *world, f32 delta_time);

// Spawn projectile entity
Entity projectile_spawn(World *world, Vec3 position, Vec3 direction, f32 speed,
                        Entity source, f32 damage);

// ============================================================================
// PROJECTILE PHYSICS
// ============================================================================

// Update projectile movement
void projectile_update_movement(ProjectileComponent *proj, Vec3 *position,
                                f64 delta_time);

// Apply homing behavior
void projectile_apply_homing(ProjectileComponent *proj, World *world,
                             f64 delta_time);

// Check projectile collision
bool projectile_check_collision(const ProjectileComponent *proj, Vec3 start,
                                Vec3 end, World *world, Entity *hit_entity,
                                Vec3 *hit_point, Vec3 *hit_normal);

// Handle projectile impact
void projectile_on_impact(ProjectileComponent *proj, World *world,
                          Entity projectile_entity, Entity hit_entity,
                          Vec3 hit_point, Vec3 hit_normal);

// Create explosion
void projectile_create_explosion(World *world, Vec3 position, f32 radius,
                                 f32 damage, Entity source);

#endif // PROJECTILE_COMPONENT_H
