#include "gameplay/combat/hitbox.h"
#include <core/memory.h>
#include <include/math/math.h>
#include <string.h>

// ============================================================================
// HITBOX CREATION
// ============================================================================

HitboxComponent hitbox_create_sphere(f32 radius, u32 team_id) {
  HitboxComponent hitbox = {0};
  hitbox.shape = HITBOX_SHAPE_SPHERE;
  hitbox.data.sphere.radius = radius;
  hitbox.offset = vec3_zero();
  hitbox.team_id = team_id;
  hitbox.active = true;
  hitbox.is_trigger = true;
  hitbox.damage_multiplier = 1.0f;
  hitbox.last_update_time = 0.0;
  return hitbox;
}

HitboxComponent hitbox_create_box(Vec3 half_extents, u32 team_id) {
  HitboxComponent hitbox = {0};
  hitbox.shape = HITBOX_SHAPE_BOX;
  hitbox.data.box.half_extents = half_extents;
  hitbox.offset = vec3_zero();
  hitbox.team_id = team_id;
  hitbox.active = true;
  hitbox.is_trigger = true;
  hitbox.damage_multiplier = 1.0f;
  hitbox.last_update_time = 0.0;
  return hitbox;
}

HitboxComponent hitbox_create_capsule(f32 radius, f32 height, u32 team_id) {
  HitboxComponent hitbox = {0};
  hitbox.shape = HITBOX_SHAPE_CAPSULE;
  hitbox.data.capsule.radius = radius;
  hitbox.data.capsule.height = height;
  hitbox.offset = vec3_zero();
  hitbox.team_id = team_id;
  hitbox.active = true;
  hitbox.is_trigger = true;
  hitbox.damage_multiplier = 1.0f;
  hitbox.last_update_time = 0.0;
  return hitbox;
}

// ============================================================================
// HITBOX ACTIVATION
// ============================================================================

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

// ============================================================================
// HITBOX PROPERTIES
// ============================================================================

void hitbox_set_offset(HitboxComponent *hitbox, Vec3 offset) {
  if (hitbox)
    hitbox->offset = offset;
}

void hitbox_set_team(HitboxComponent *hitbox, u32 team_id) {
  if (hitbox)
    hitbox->team_id = team_id;
}

void hitbox_set_damage_multiplier(HitboxComponent *hitbox, f32 multiplier) {
  if (hitbox)
    hitbox->damage_multiplier = multiplier;
}

void hitbox_update_world_position(HitboxComponent *hitbox,
                                  Vec3 entity_position) {
  if (!hitbox)
    return;
  hitbox->world_position = vec3_add(entity_position, hitbox->offset);
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

static bool sphere_sphere_collision(Vec3 center_a, f32 radius_a, Vec3 center_b,
                                    f32 radius_b, HitboxCollision *out) {
  Vec3 delta = vec3_subtract(center_b, center_a);
  f32 distance_sq = vec3_length_squared(delta);
  f32 radius_sum = radius_a + radius_b;
  f32 radius_sum_sq = radius_sum * radius_sum;

  if (distance_sq >= radius_sum_sq) {
    return false;
  }

  if (out) {
    f32 distance = sqrtf(distance_sq);
    out->hit = true;
    out->penetration_depth = radius_sum - distance;

    if (distance > 0.0001f) {
      Vec3 normal = vec3_mul(delta, 1.0f / distance);
      out->contact_normal = normal;
      out->contact_point = vec3_add(
          center_a, vec3_mul(normal, radius_a - out->penetration_depth * 0.5f));
    } else {
      out->contact_normal = vec3_up();
      out->contact_point = center_a;
    }
  }

  return true;
}

static bool sphere_box_collision(Vec3 sphere_center, f32 sphere_radius,
                                 Vec3 box_center, Vec3 box_half_extents,
                                 HitboxCollision *out) {
  // Find closest point on box to sphere
  Vec3 closest_point;
  closest_point.x =
      fmaxf(box_center.x - box_half_extents.x,
            fminf(sphere_center.x, box_center.x + box_half_extents.x));
  closest_point.y =
      fmaxf(box_center.y - box_half_extents.y,
            fminf(sphere_center.y, box_center.y + box_half_extents.y));
  closest_point.z =
      fmaxf(box_center.z - box_half_extents.z,
            fminf(sphere_center.z, box_center.z + box_half_extents.z));

  Vec3 delta = vec3_subtract(closest_point, sphere_center);
  f32 distance_sq = vec3_length_squared(delta);

  if (distance_sq >= sphere_radius * sphere_radius) {
    return false;
  }

  if (out) {
    f32 distance = sqrtf(distance_sq);
    out->hit = true;
    out->penetration_depth = sphere_radius - distance;
    out->contact_point = closest_point;

    if (distance > 0.0001f) {
      out->contact_normal =
          vec3_normalize(vec3_subtract(sphere_center, closest_point));
    } else {
      // Sphere center is inside box, find nearest face
      Vec3 local = vec3_subtract(sphere_center, box_center);
      f32 min_dist = INFINITY;
      Vec3 normal = vec3_up();

      f32 dist_x = box_half_extents.x - fabsf(local.x);
      f32 dist_y = box_half_extents.y - fabsf(local.y);
      f32 dist_z = box_half_extents.z - fabsf(local.z);

      if (dist_x < min_dist) {
        min_dist = dist_x;
        normal = (Vec3){local.x > 0 ? 1.0f : -1.0f, 0, 0};
      }
      if (dist_y < min_dist) {
        min_dist = dist_y;
        normal = (Vec3){0, local.y > 0 ? 1.0f : -1.0f, 0};
      }
      if (dist_z < min_dist) {
        min_dist = dist_z;
        normal = (Vec3){0, 0, local.z > 0 ? 1.0f : -1.0f};
      }

      out->contact_normal = normal;
    }
  }

  return true;
}

static bool box_box_collision(Vec3 center_a, Vec3 half_extents_a, Vec3 center_b,
                              Vec3 half_extents_b, HitboxCollision *out) {
  // Simple AABB collision
  Vec3 min_a = vec3_subtract(center_a, half_extents_a);
  Vec3 max_a = vec3_add(center_a, half_extents_a);
  Vec3 min_b = vec3_subtract(center_b, half_extents_b);
  Vec3 max_b = vec3_add(center_b, half_extents_b);

  bool overlap_x = min_a.x <= max_b.x && max_a.x >= min_b.x;
  bool overlap_y = min_a.y <= max_b.y && max_a.y >= min_b.y;
  bool overlap_z = min_a.z <= max_b.z && max_a.z >= min_b.z;

  if (!(overlap_x && overlap_y && overlap_z)) {
    return false;
  }

  if (out) {
    out->hit = true;

    // Calculate penetration depths for each axis
    f32 pen_x = fminf(max_a.x - min_b.x, max_b.x - min_a.x);
    f32 pen_y = fminf(max_a.y - min_b.y, max_b.y - min_a.y);
    f32 pen_z = fminf(max_a.z - min_b.z, max_b.z - min_a.z);

    // Find minimum penetration axis
    if (pen_x < pen_y && pen_x < pen_z) {
      out->penetration_depth = pen_x;
      out->contact_normal =
          (Vec3){center_a.x < center_b.x ? -1.0f : 1.0f, 0, 0};
    } else if (pen_y < pen_z) {
      out->penetration_depth = pen_y;
      out->contact_normal =
          (Vec3){0, center_a.y < center_b.y ? -1.0f : 1.0f, 0};
    } else {
      out->penetration_depth = pen_z;
      out->contact_normal =
          (Vec3){0, 0, center_a.z < center_b.z ? -1.0f : 1.0f};
    }

    out->contact_point = vec3_mul(vec3_add(center_a, center_b), 0.5f);
  }

  return true;
}

bool hitbox_test_collision(const HitboxComponent *a, const Vec3 *pos_a,
                           const HitboxComponent *b, const Vec3 *pos_b,
                           HitboxCollision *out_collision) {
  if (!a || !b || !pos_a || !pos_b)
    return false;
  if (!a->active || !b->active)
    return false;

  // Same team check (prevent friendly fire)
  if (a->team_id == b->team_id && a->team_id != 0) {
    return false;
  }

  Vec3 world_pos_a = vec3_add(*pos_a, a->offset);
  Vec3 world_pos_b = vec3_add(*pos_b, b->offset);

  bool collision = false;
  HitboxCollision temp_collision = {0};

  // Dispatch to appropriate collision function
  if (a->shape == HITBOX_SHAPE_SPHERE && b->shape == HITBOX_SHAPE_SPHERE) {
    collision =
        sphere_sphere_collision(world_pos_a, a->data.sphere.radius, world_pos_b,
                                b->data.sphere.radius, &temp_collision);
  } else if (a->shape == HITBOX_SHAPE_SPHERE && b->shape == HITBOX_SHAPE_BOX) {
    collision =
        sphere_box_collision(world_pos_a, a->data.sphere.radius, world_pos_b,
                             b->data.box.half_extents, &temp_collision);
  } else if (a->shape == HITBOX_SHAPE_BOX && b->shape == HITBOX_SHAPE_SPHERE) {
    collision =
        sphere_box_collision(world_pos_b, b->data.sphere.radius, world_pos_a,
                             a->data.box.half_extents, &temp_collision);
    if (collision) {
      temp_collision.contact_normal =
          vec3_negate(temp_collision.contact_normal);
    }
  } else if (a->shape == HITBOX_SHAPE_BOX && b->shape == HITBOX_SHAPE_BOX) {
    collision =
        box_box_collision(world_pos_a, a->data.box.half_extents, world_pos_b,
                          b->data.box.half_extents, &temp_collision);
  }
  // TODO: Add capsule collision support

  if (out_collision && collision) {
    *out_collision = temp_collision;
  }

  return collision;
}

// ============================================================================
// HITBOX QUERIES
// ============================================================================

AABB hitbox_get_world_bounds(const HitboxComponent *hitbox) {
  AABB bounds = {{0}, {0}};
  if (!hitbox)
    return bounds;

  Vec3 center = hitbox->world_position;

  switch (hitbox->shape) {
  case HITBOX_SHAPE_SPHERE: {
    f32 r = hitbox->data.sphere.radius;
    bounds.min = vec3_subtract(center, vec3(r, r, r));
    bounds.max = vec3_add(center, vec3(r, r, r));
    break;
  }
  case HITBOX_SHAPE_BOX: {
    bounds.min = vec3_subtract(center, hitbox->data.box.half_extents);
    bounds.max = vec3_add(center, hitbox->data.box.half_extents);
    break;
  }
  case HITBOX_SHAPE_CAPSULE: {
    f32 r = hitbox->data.capsule.radius;
    f32 h = hitbox->data.capsule.height * 0.5f;
    bounds.min = vec3_subtract(center, vec3(r, h + r, r));
    bounds.max = vec3_add(center, vec3(r, h + r, r));
    break;
  }
  default:
    break;
  }

  return bounds;
}

f32 hitbox_get_volume(const HitboxComponent *hitbox) {
  if (!hitbox)
    return 0.0f;

  switch (hitbox->shape) {
  case HITBOX_SHAPE_SPHERE: {
    f32 r = hitbox->data.sphere.radius;
    return (4.0f / 3.0f) * 3.14159f * r * r * r;
  }
  case HITBOX_SHAPE_BOX: {
    Vec3 ext = hitbox->data.box.half_extents;
    return 8.0f * ext.x * ext.y * ext.z;
  }
  case HITBOX_SHAPE_CAPSULE: {
    f32 r = hitbox->data.capsule.radius;
    f32 h = hitbox->data.capsule.height;
    // Cylinder + two hemispheres
    return 3.14159f * r * r * (h + (4.0f / 3.0f) * r);
  }
  default:
    return 0.0f;
  }
}
