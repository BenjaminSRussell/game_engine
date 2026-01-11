// include/gameplay/combat/hitbox.h
//
// Purpose: Defines the HitboxComponent for collision detection in combat
// scenarios. Hitboxes can have different shapes (sphere, box, capsule) and are
// used to detect when attacks connect with targets. Supports team-based damage
// filtering.
//
// Public APIs:
// - HitboxShape: Enum defining collision shape types
// - HitboxComponent: Component structure with shape, dimensions, offset, and
// team
// - Hitbox management functions for activation, deactivation, and collision
// testing
//
// Ownership: HitboxComponent instances are owned by the ECS framework
//
// Invariants:
// - radius/half_extents must be positive values
// - team_id is used for friendly fire prevention
// - active flag determines if hitbox participates in collision detection
//
#ifndef HITBOX_COMPONENT_H
#define HITBOX_COMPONENT_H

#include <common.h>
#include <ecs/ecs.h>
#include <math/vec3.h>

// Hitbox shape types
typedef enum {
  HITBOX_SHAPE_SPHERE,
  HITBOX_SHAPE_BOX,
  HITBOX_SHAPE_CAPSULE,
  HITBOX_SHAPE_COUNT
} HitboxShape;

// Hitbox component for collision detection
typedef struct {
  HitboxShape shape;
  Vec3 offset; // Offset from entity position

  // Shape-specific data
  union {
    struct {
      f32 radius;
    } sphere;

    struct {
      Vec3 half_extents;
    } box;

    struct {
      f32 radius;
      f32 height;
    } capsule;
  } data;

  // Combat properties
  u32 team_id;           // Team/faction ID for friendly fire prevention
  bool active;           // Whether hitbox is currently active
  bool is_trigger;       // True for damage zones, false for blocking
  f32 damage_multiplier; // Multiplier for damage dealt through this hitbox

  // Runtime data
  Vec3 world_position;  // Cached world position
  f64 last_update_time; // Last time this hitbox was updated
} HitboxComponent;

// Hitbox collision result
typedef struct {
  bool hit;
  Vec3 contact_point;
  Vec3 contact_normal;
  f32 penetration_depth;
  Entity hit_entity;
} HitboxCollision;

// ============================================================================
// HITBOX MANAGEMENT
// ============================================================================

// System lifecycle
bool hitbox_system_init(World *world);
void hitbox_system_shutdown(void);
void hitbox_system_update(World *world, f32 delta_time);

// Create hitbox components
HitboxComponent hitbox_create_sphere(f32 radius, u32 team_id);
HitboxComponent hitbox_create_box(Vec3 half_extents, u32 team_id);
HitboxComponent hitbox_create_capsule(f32 radius, f32 height, u32 team_id);

// Create a temporary hitbox entity that lasts for a short duration
Entity hitbox_create_temporary(World *world, Vec3 position, Vec3 direction,
                               f32 range, f32 duration);

// Hitbox activation (for attack frames)
void hitbox_activate(HitboxComponent *hitbox);
void hitbox_deactivate(HitboxComponent *hitbox);
void hitbox_set_active(HitboxComponent *hitbox, bool active);

// Hitbox collision detection
bool hitbox_test_collision(const HitboxComponent *a, const Vec3 *pos_a,
                           const HitboxComponent *b, const Vec3 *pos_b,
                           HitboxCollision *out_collision);

// Hitbox properties
void hitbox_set_offset(HitboxComponent *hitbox, Vec3 offset);
void hitbox_set_team(HitboxComponent *hitbox, u32 team_id);
void hitbox_set_damage_multiplier(HitboxComponent *hitbox, f32 multiplier);

// Update hitbox world position
void hitbox_update_world_position(HitboxComponent *hitbox,
                                  Vec3 entity_position);

// ============================================================================
// HITBOX QUERIES
// ============================================================================

// Get hitbox world bounds
typedef struct {
  Vec3 min;
  Vec3 max;
} AABB;

AABB hitbox_get_world_bounds(const HitboxComponent *hitbox);

// Calculate approximate volume (for damage scaling)
f32 hitbox_get_volume(const HitboxComponent *hitbox);

#endif // HITBOX_COMPONENT_H
