#include "physics/continuous_collision.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global CCD world instance
static CCDWorld g_ccd_world = {0};

bool ccd_world_init(CCDWorld *world, uint32_t max_entities) {
  if (!world || max_entities == 0)
    return false;

  memset(world, 0, sizeof(CCDWorld));

  // Initialize broadphase entries
  world->entry_capacity = max_entities;
  world->entries = (CCDBroadphaseEntry *)core_alloc(world->entry_capacity *
                                                    sizeof(CCDBroadphaseEntry));
  if (!world->entries)
    return false;
  memset(world->entries, 0, world->entry_capacity * sizeof(CCDBroadphaseEntry));

  // Initialize spatial hash
  world->spatial_hash.cell_size = 2.0f;
  world->spatial_hash.table_size = next_power_of_two(max_entities * 4);
  world->spatial_hash.hash_table =
      (uint32_t *)core_alloc(world->spatial_hash.table_size * sizeof(uint32_t));
  world->spatial_hash.entry_lists =
      (uint32_t *)core_alloc(world->spatial_hash.table_size * sizeof(uint32_t));

  if (!world->spatial_hash.hash_table || !world->spatial_hash.entry_lists) {
    ccd_world_cleanup(world);
    return false;
  }

  // Initialize hash table with empty values
  for (uint32_t i = 0; i < world->spatial_hash.table_size; i++) {
    world->spatial_hash.hash_table[i] = UINT32_MAX;
    world->spatial_hash.entry_lists[i] = UINT32_MAX;
  }

  // Set default configuration
  world->max_time_step = 1.0f / 60.0f;
  world->ccd_threshold = 0.5f;
  world->enable_speculative_contacts = true;
  world->enable_motion_clamping = true;
  world->max_substeps = 4;
  world->contact_erp = 0.2f;
  world->contact_cfm = 0.0f;

  // Initialize world bounds
  world->spatial_hash.world_min = (Vec3){-1000.0f, -1000.0f, -1000.0f};
  world->spatial_hash.world_max = (Vec3){1000.0f, 1000.0f, 1000.0f};

  LOG_INFO("CCD world initialized with capacity for %u entities", max_entities);
  return true;
}

void ccd_world_cleanup(CCDWorld *world) {
  if (!world)
    return;

  if (world->entries) {
    core_free(world->entries);
    world->entries = NULL;
  }

  if (world->spatial_hash.hash_table) {
    core_free(world->spatial_hash.hash_table);
    world->spatial_hash.hash_table = NULL;
  }

  if (world->spatial_hash.entry_lists) {
    core_free(world->spatial_hash.entry_lists);
    world->spatial_hash.entry_lists = NULL;
  }

  memset(world, 0, sizeof(CCDWorld));
  LOG_INFO("CCD world cleaned up");
}

void ccd_world_update(CCDWorld *world, float time_step) {
  if (!world)
    return;

  float start_time = get_time();

  // Clamp time step to maximum
  float clamped_time_step = fminf(time_step, world->max_time_step);

  // Update broadphase
  ccd_broadphase_update(world);

  // Perform collision detection with sub-stepping
  float sub_step_time = clamped_time_step / world->max_substeps;

  for (int step = 0; step < world->max_substeps; step++) {
    // Update broadphase for sub-step
    ccd_broadphase_update(world);

    // Detect and resolve collisions
    for (uint32_t i = 0; i < world->entry_count; i++) {
      CCDBroadphaseEntry *entry_a = &world->entries[i];
      if (!entry_a || entry_a->entity == 0)
        continue;

      // Get swept bounds for this entry
      Vec3 bounds_min_a, bounds_max_a;
      ccd_calculate_swept_bounds_for_entry(entry_a, sub_step_time,
                                           &bounds_min_a, &bounds_max_a);

      // Query spatial hash for potential collisions
      uint32_t potential_collisions[32];
      uint32_t collision_count = 0;
      ccd_spatial_hash_query(world, bounds_min_a, bounds_max_a,
                             potential_collisions, &collision_count);

      for (uint32_t j = 0; j < collision_count; j++) {
        uint32_t entry_index = potential_collisions[j];
        if (entry_index <= i)
          continue; // Avoid duplicate checks

        CCDBroadphaseEntry *entry_b = &world->entries[entry_index];
        if (!entry_b || entry_b->entity == 0)
          continue;

        // Check collision masks
        if ((entry_a->collision_mask & entry_b->collision_group) == 0)
          continue;
        if ((entry_b->collision_mask & entry_a->collision_group) == 0)
          continue;

        // Perform narrowphase CCD
        CCDCollisionResult result = {0};
        if (ccd_detect_collision_for_entries(entry_a, entry_b, sub_step_time,
                                             &result)) {
          ccd_resolve_collision(world, &result);
          world->collisions_found++;
        }

        world->narrowphase_tests++;
      }
    }

    // Integrate motion for next sub-step
    for (uint32_t i = 0; i < world->entry_count; i++) {
      CCDBroadphaseEntry *entry = &world->entries[i];
      if (entry && entry->entity != 0 && !entry->base.is_static) {
        entry->position =
            vec3_add(entry->position, vec3_mul(entry->velocity, sub_step_time));
      }
    }
  }

  world->ccd_time = get_time() - start_time;
}

bool ccd_world_add_sphere(CCDWorld *world, EntityID entity,
                          const CCDSphere *sphere) {
  if (!world || !sphere || world->entry_count >= world->entry_capacity)
    return false;

  CCDBroadphaseEntry *entry = &world->entries[world->entry_count];
  entry->entity = entity;
  entry->shape_type = CCD_SHAPE_SPHERE;
  entry->position = sphere->base.position;
  entry->velocity = sphere->base.linear_velocity;
  entry->broadphase_index = world->entry_count;

  // Calculate bounds
  float radius = sphere->radius + sphere->base.margin;
  entry->bounds_min = vec3_sub(entry->position, (Vec3){radius, radius, radius});
  entry->bounds_max = vec3_add(entry->position, (Vec3){radius, radius, radius});

  // Insert into spatial hash
  ccd_spatial_hash_insert(world, world->entry_count);

  world->entry_count++;
  return true;
}

bool ccd_world_add_box(CCDWorld *world, EntityID entity, const CCDBox *box) {
  if (!world || !box || world->entry_count >= world->entry_capacity)
    return false;

  CCDBroadphaseEntry *entry = &world->entries[world->entry_count];
  entry->entity = entity;
  entry->shape_type = CCD_SHAPE_BOX;
  entry->position = box->base.position;
  entry->velocity = box->base.linear_velocity;
  entry->broadphase_index = world->entry_count;

  // Calculate bounds (simplified - should account for rotation)
  entry->bounds_min = vec3_sub(entry->position, box->half_extents);
  entry->bounds_max = vec3_add(entry->position, box->half_extents);

  // Insert into spatial hash
  ccd_spatial_hash_insert(world, world->entry_count);

  world->entry_count++;
  return true;
}

bool ccd_detect_collision(const CCDShape *shape_a, const CCDShape *shape_b,
                          float time_step, CCDCollisionResult *result) {
  if (!shape_a || !shape_b || !result)
    return false;

  // Check if shapes are moving fast enough to require CCD
  if (!ccd_should_use_ccd(shape_a, time_step) &&
      !ccd_should_use_ccd(shape_b, time_step)) {
    return false;
  }

  // Dispatch to appropriate collision function
  switch (shape_a->type) {
  case CCD_SHAPE_SPHERE:
    switch (shape_b->type) {
    case CCD_SHAPE_SPHERE:
      return ccd_sphere_vs_sphere((const CCDSphere *)shape_a,
                                  (const CCDSphere *)shape_b, time_step,
                                  result);
    case CCD_SHAPE_BOX:
      return ccd_sphere_vs_box((const CCDSphere *)shape_a,
                               (const CCDBox *)shape_b, time_step, result);
    case CCD_SHAPE_CAPSULE:
      return ccd_sphere_vs_capsule((const CCDSphere *)shape_a,
                                   (const CCDCapsule *)shape_b, time_step,
                                   result);
    default:
      return false;
    }
  case CCD_SHAPE_BOX:
    switch (shape_b->type) {
    case CCD_SHAPE_SPHERE:
      return ccd_sphere_vs_box((const CCDSphere *)shape_b,
                               (const CCDBox *)shape_a, time_step, result);
    case CCD_SHAPE_BOX:
      return ccd_box_vs_box((const CCDBox *)shape_a, (const CCDBox *)shape_b,
                            time_step, result);
    default:
      return false;
    }
  case CCD_SHAPE_CAPSULE:
    switch (shape_b->type) {
    case CCD_SHAPE_SPHERE:
      return ccd_sphere_vs_capsule((const CCDSphere *)shape_b,
                                   (const CCDCapsule *)shape_a, time_step,
                                   result);
    case CCD_SHAPE_CAPSULE:
      return ccd_capsule_vs_capsule((const CCDCapsule *)shape_a,
                                    (const CCDCapsule *)shape_b, time_step,
                                    result);
    default:
      return false;
    }
  default:
    return false;
  }
}

bool ccd_sphere_vs_sphere(const CCDSphere *sphere_a, const CCDSphere *sphere_b,
                          float time_step, CCDCollisionResult *result) {
  if (!sphere_a || !sphere_b || !result)
    return false;

  // Calculate relative motion
  Vec3 relative_velocity =
      vec3_sub(sphere_b->base.linear_velocity, sphere_a->base.linear_velocity);
  Vec3 relative_position =
      vec3_sub(sphere_b->base.position, sphere_a->base.position);

  float combined_radius = sphere_a->radius + sphere_b->radius;

  // Check if spheres are already intersecting
  float distance_sq = vec3_length_sq(relative_position);
  if (distance_sq < combined_radius * combined_radius) {
    // Spheres are intersecting, return immediate collision
    float distance = sqrtf(distance_sq);
    result->hit = true;
    result->time_of_impact = 0.0f;
    result->contact_normal = (distance > 0.001f)
                                 ? vec3_div(relative_position, distance)
                                 : (Vec3){0, 1, 0};
    result->contact_point =
        vec3_add(sphere_a->base.position,
                 vec3_mul(result->contact_normal, sphere_a->radius));
    result->penetration_depth =
        vec3_length(relative_position) - combined_radius;
    result->impact_velocity = vec3_length(relative_velocity);
    return true;
  }

  // Solve quadratic equation for time of impact
  float a = vec3_length_sq(relative_velocity);
  float b = 2.0f * vec3_dot(relative_velocity, relative_position);
  float c =
      vec3_length_sq(relative_position) - combined_radius * combined_radius;

  float discriminant = b * b - 4.0f * a * c;
  if (discriminant < 0.0f || a < 0.001f) {
    return false; // No collision
  }

  float sqrt_discriminant = sqrtf(discriminant);
  float t1 = (-b - sqrt_discriminant) / (2.0f * a);
  float t2 = (-b + sqrt_discriminant) / (2.0f * a);

  // We want the earliest positive time of impact
  float t = (t1 > 0.0f) ? t1 : t2;
  if (t < 0.0f || t > time_step) {
    return false; // Collision occurs outside time step
  }

  // Calculate collision data
  result->hit = true;
  result->time_of_impact = t;

  Vec3 position_at_impact_a = vec3_add(
      sphere_a->base.position, vec3_mul(sphere_a->base.linear_velocity, t));
  Vec3 position_at_impact_b = vec3_add(
      sphere_b->base.position, vec3_mul(sphere_b->base.linear_velocity, t));
  Vec3 normal_at_impact = vec3_sub(position_at_impact_b, position_at_impact_a);

  float normal_length = vec3_length(normal_at_impact);
  if (normal_length > 0.001f) {
    result->contact_normal = vec3_div(normal_at_impact, normal_length);
  } else {
    result->contact_normal = (Vec3){0, 1, 0};
  }

  result->contact_point = vec3_add(
      position_at_impact_a, vec3_mul(result->contact_normal, sphere_a->radius));
  result->penetration_depth = 0.0f; // Just touching at impact
  result->impact_velocity = vec3_length(relative_velocity);

  return true;
}

bool ccd_sphere_vs_box(const CCDSphere *sphere, const CCDBox *box,
                       float time_step, CCDCollisionResult *result) {
  if (!sphere || !box || !result)
    return false;

  // Transform sphere to box's local space (simplified - ignoring rotation)
  Vec3 local_sphere_pos = vec3_sub(sphere->base.position, box->base.position);
  Vec3 local_sphere_vel =
      vec3_sub(sphere->base.linear_velocity, box->base.linear_velocity);

  // Find closest point on box to sphere center
  Vec3 closest_point = {
      clamp(local_sphere_pos.x, -box->half_extents.x, box->half_extents.x),
      clamp(local_sphere_pos.y, -box->half_extents.y, box->half_extents.y),
      clamp(local_sphere_pos.z, -box->half_extents.z, box->half_extents.z)};

  // Check if sphere is already intersecting
  Vec3 to_sphere = vec3_sub(local_sphere_pos, closest_point);
  float distance_sq = vec3_length_sq(to_sphere);
  float radius_sq = sphere->radius * sphere->radius;

  if (distance_sq < radius_sq) {
    // Already intersecting
    float distance = sqrtf(fmaxf(distance_sq, 0.001f));
    result->hit = true;
    result->time_of_impact = 0.0f;
    result->contact_normal = vec3_div(to_sphere, distance);
    result->contact_point = vec3_add(box->base.position, closest_point);
    result->penetration_depth = sphere->radius - distance;
    result->impact_velocity = vec3_length(
        vec3_sub(sphere->base.linear_velocity, box->base.linear_velocity));
    return true;
  }

  // For continuous collision, we'd need to solve the intersection over time
  // This is complex for sphere-box, so we'll use a simplified approach
  // Check if the sphere will intersect during the time step

  // Approximate by checking swept sphere against expanded box
  Vec3 swept_bounds_min = vec3_sub(
      box->base.position,
      vec3_add(box->half_extents,
               (Vec3){sphere->radius, sphere->radius, sphere->radius}));
  Vec3 swept_bounds_max = vec3_add(
      box->base.position,
      vec3_add(box->half_extents,
               (Vec3){sphere->radius, sphere->radius, sphere->radius}));

  Vec3 sphere_end_pos = vec3_add(
      sphere->base.position, vec3_mul(sphere->base.linear_velocity, time_step));

  if (point_in_aabb(sphere_end_pos, swept_bounds_min, swept_bounds_max)) {
    // Approximate collision
    result->hit = true;
    result->time_of_impact = time_step * 0.5f; // Approximate
    result->contact_normal =
        vec3_normalize(vec3_sub(sphere->base.position, box->base.position));
    result->contact_point =
        vec3_add(sphere->base.position,
                 vec3_mul(result->contact_normal, sphere->radius));
    result->penetration_depth = 0.0f;
    result->impact_velocity = vec3_length(sphere->base.linear_velocity);
    return true;
  }

  return false;
}

bool ccd_box_vs_box(const CCDBox *box_a, const CCDBox *box_b, float time_step,
                    CCDCollisionResult *result) {
  if (!box_a || !box_b || !result)
    return false;

  // Simplified AABB vs AABB continuous collision
  // For full implementation, would need to use separating axis theorem with
  // time

  Vec3 relative_velocity =
      vec3_sub(box_b->base.linear_velocity, box_a->base.linear_velocity);
  Vec3 relative_position = vec3_sub(box_b->base.position, box_a->base.position);

  // Check current intersection
  Vec3 a_min = vec3_sub(box_a->base.position, box_a->half_extents);
  Vec3 a_max = vec3_add(box_a->base.position, box_a->half_extents);
  Vec3 b_min = vec3_sub(box_b->base.position, box_b->half_extents);
  Vec3 b_max = vec3_add(box_b->base.position, box_b->half_extents);

  if (aabb_intersect(a_min, a_max, b_min, b_max)) {
    // Already intersecting
    result->hit = true;
    result->time_of_impact = 0.0f;

    // Calculate penetration on each axis
    Vec3 penetration = {fminf(a_max.x - b_min.x, b_max.x - a_min.x),
                        fminf(a_max.y - b_min.y, b_max.y - a_min.y),
                        fminf(a_max.z - b_min.z, b_max.z - a_min.z)};

    // Find axis with minimum penetration
    if (penetration.x < penetration.y && penetration.x < penetration.z) {
      result->contact_normal =
          (relative_position.x > 0) ? (Vec3){1, 0, 0} : (Vec3){-1, 0, 0};
      result->penetration_depth = penetration.x;
    } else if (penetration.y < penetration.z) {
      result->contact_normal =
          (relative_position.y > 0) ? (Vec3){0, 1, 0} : (Vec3){0, -1, 0};
      result->penetration_depth = penetration.y;
    } else {
      result->contact_normal =
          (relative_position.z > 0) ? (Vec3){0, 0, 1} : (Vec3){0, 0, -1};
      result->penetration_depth = penetration.z;
    }

    result->contact_point = vec3_mul(vec3_add(a_min, a_max), 0.5f);
    result->impact_velocity = vec3_length(relative_velocity);
    return true;
  }

  // Check for future intersection (simplified)
  Vec3 a_end_min =
      vec3_add(a_min, vec3_mul(box_a->base.linear_velocity, time_step));
  Vec3 a_end_max =
      vec3_add(a_max, vec3_mul(box_a->base.linear_velocity, time_step));

  if (aabb_intersect(a_end_min, a_end_max, b_min, b_max)) {
    result->hit = true;
    result->time_of_impact = time_step * 0.5f;
    result->contact_normal = vec3_normalize(relative_position);
    result->contact_point = vec3_mul(vec3_add(a_min, a_max), 0.5f);
    result->penetration_depth = 0.0f;
    result->impact_velocity = vec3_length(relative_velocity);
    return true;
  }

  return false;
}

void ccd_broadphase_update(CCDWorld *world) {
  if (!world)
    return;

  // Update swept bounds for all entries
  for (uint32_t i = 0; i < world->entry_count; i++) {
    ccd_update_broadphase_bounds(world, i);
  }

  // Rebuild spatial hash
  for (uint32_t i = 0; i < world->spatial_hash.table_size; i++) {
    world->spatial_hash.hash_table[i] = UINT32_MAX;
    world->spatial_hash.entry_lists[i] = UINT32_MAX;
  }

  for (uint32_t i = 0; i < world->entry_count; i++) {
    ccd_spatial_hash_insert(world, i);
  }
}

void ccd_spatial_hash_insert(CCDWorld *world, uint32_t entry_index) {
  if (!world || entry_index >= world->entry_count)
    return;

  CCDBroadphaseEntry *entry = &world->entries[entry_index];

  // Calculate hash cells for the entry's bounds
  Vec3 min_cell = {
      floorf((entry->bounds_min.x - world->spatial_hash.world_min.x) /
             world->spatial_hash.cell_size),
      floorf((entry->bounds_min.y - world->spatial_hash.world_min.y) /
             world->spatial_hash.cell_size),
      floorf((entry->bounds_min.z - world->spatial_hash.world_min.z) /
             world->spatial_hash.cell_size)};

  Vec3 max_cell = {
      floorf((entry->bounds_max.x - world->spatial_hash.world_min.x) /
             world->spatial_hash.cell_size),
      floorf((entry->bounds_max.y - world->spatial_hash.world_min.y) /
             world->spatial_hash.cell_size),
      floorf((entry->bounds_max.z - world->spatial_hash.world_min.z) /
             world->spatial_hash.cell_size)};

  // Insert into all cells the entry spans
  for (int x = (int)min_cell.x; x <= (int)max_cell.x; x++) {
    for (int y = (int)min_cell.y; y <= (int)max_cell.y; y++) {
      for (int z = (int)min_cell.z; z <= (int)max_cell.z; z++) {
        uint32_t hash = spatial_hash_coordinate(x, y, z) &
                        (world->spatial_hash.table_size - 1);

        // Insert at head of list
        world->spatial_hash.entry_lists[entry_index] =
            world->spatial_hash.hash_table[hash];
        world->spatial_hash.hash_table[hash] = entry_index;
      }
    }
  }
}

bool ccd_should_use_ccd(const CCDShape *shape, float time_step) {
  if (!shape || shape->is_static)
    return false;

  float speed = vec3_length(shape->linear_velocity);
  float angular_speed = vec3_length(shape->angular_velocity);

  // Use CCD if object is moving fast relative to its size
  float motion_threshold = speed * time_step;

  switch (shape->type) {
  case CCD_SHAPE_SPHERE:
    return motion_threshold > ((const CCDSphere *)shape)->radius * 0.5f;
  case CCD_SHAPE_BOX: {
    Vec3 extents = ((const CCDBox *)shape)->half_extents;
    float max_extent = fmaxf(fmaxf(extents.x, extents.y), extents.z);
    return motion_threshold > max_extent * 0.5f;
  }
  case CCD_SHAPE_CAPSULE:
    return motion_threshold > ((const CCDCapsule *)shape)->radius * 0.5f;
  default:
    return motion_threshold > 1.0f; // Default threshold
  }
}

void ccd_update_broadphase_bounds(CCDWorld *world, uint32_t entry_index) {
  if (!world || entry_index >= world->entry_count)
    return;

  CCDBroadphaseEntry *entry = &world->entries[entry_index];

  // Calculate swept bounds based on velocity
  Vec3 velocity_extent =
      vec3_mul(vec3_abs(entry->velocity), world->max_time_step);

  // Expand current bounds by velocity extent
  entry->bounds_min = vec3_sub(entry->bounds_min, velocity_extent);
  entry->bounds_max = vec3_add(entry->bounds_max, velocity_extent);
}

void ccd_resolve_collision(CCDWorld *world,
                           const CCDCollisionResult *collision) {
  if (!world || !collision || !collision->hit)
    return;

  // Apply position correction to prevent penetration
  if (world->enable_motion_clamping && collision->penetration_depth > 0.001f) {
    // Move objects apart to resolve penetration
    Vec3 correction =
        vec3_mul(collision->contact_normal,
                 collision->penetration_depth * world->contact_erp);

    // Find and update the entities
    for (uint32_t i = 0; i < world->entry_count; i++) {
      CCDBroadphaseEntry *entry = &world->entries[i];
      if (entry->entity == collision->entity_a) {
        entry->position = vec3_sub(entry->position, vec3_mul(correction, 0.5f));
      } else if (entry->entity == collision->entity_b) {
        entry->position = vec3_add(entry->position, vec3_mul(correction, 0.5f));
      }
    }
  }

  // Apply impulse to change velocities
  if (collision->impact_velocity > 0.1f) {
    float restitution = 0.5f; // Coefficient of restitution
    float impulse_magnitude =
        -(1.0f + restitution) * collision->impact_velocity;

    ccd_apply_impulse(collision, impulse_magnitude);
  }
}

void ccd_apply_impulse(const CCDCollisionResult *collision,
                       float impulse_magnitude) {
  if (!collision)
    return;

  // Apply impulse to velocities (simplified - assumes unit mass)
  Vec3 impulse = vec3_mul(collision->contact_normal, impulse_magnitude);

  // Note: In a full implementation, we would need to access the actual entities
  // and apply the impulse based on their masses and velocities
  LOG_DEBUG("Applied impulse of %.3f to collision", impulse_magnitude);
}

// Utility functions
uint32_t spatial_hash_coordinate(int x, int y, int z) {
  // Simple hash function for spatial coordinates
  const uint32_t primes[3] = {73856093, 19349663, 83492791};
  return (uint32_t)((x * primes[0]) ^ (y * primes[1]) ^ (z * primes[2]));
}

bool ccd_bounds_intersect(Vec3 min_a, Vec3 max_a, Vec3 min_b, Vec3 max_b) {
  return (min_a.x <= max_b.x && max_a.x >= min_b.x) &&
         (min_a.y <= max_b.y && max_a.y >= min_b.y) &&
         (min_a.z <= max_b.z && max_a.z >= min_b.z);
}

bool point_in_aabb(Vec3 point, Vec3 min_bounds, Vec3 max_bounds) {
  return (point.x >= min_bounds.x && point.x <= max_bounds.x) &&
         (point.y >= min_bounds.y && point.y <= max_bounds.y) &&
         (point.z >= min_bounds.z && point.z <= max_bounds.z);
}

float ccd_calculate_swept_bounds(const CCDShape *shape, float time_step,
                                 Vec3 *bounds_min, Vec3 *bounds_max) {
  if (!shape || !bounds_min || !bounds_max)
    return 0.0f;

  // Base bounds calculation (would depend on shape type)
  Vec3 min_b = shape->position;
  Vec3 max_b = shape->position;

  // Simple approximation for all shapes for now - likely need more specific
  // logic later
  float radius = 1.0f; // Default
  if (shape->type == CCD_SHAPE_SPHERE) {
    radius = ((const CCDSphere *)shape)->radius;
  } else if (shape->type == CCD_SHAPE_BOX) {
    Vec3 half = ((const CCDBox *)shape)->half_extents;
    radius = fmaxf(fmaxf(half.x, half.y), half.z);
  } else if (shape->type == CCD_SHAPE_CAPSULE) {
    radius = ((const CCDCapsule *)shape)->radius; // Approx
  }

  // Add margin
  radius += shape->margin;

  min_b = vec3_sub(shape->position, (Vec3){radius, radius, radius});
  max_b = vec3_add(shape->position, (Vec3){radius, radius, radius});

  // Sweep by velocity
  Vec3 velocity_step = vec3_mul(shape->linear_velocity, time_step);

  Vec3 final_min = vec3_min(min_b, vec3_add(min_b, velocity_step));
  Vec3 final_max = vec3_max(max_b, vec3_add(max_b, velocity_step));

  *bounds_min = final_min;
  *bounds_max = final_max;

  return vec3_length(velocity_step);
}

uint32_t ccd_spatial_hash_position_to_cell(const CCDWorld *world,
                                           Vec3 position) {
  if (!world)
    return 0;

  int x = (int)floorf((position.x - world->spatial_hash.world_min.x) /
                      world->spatial_hash.cell_size);
  int y = (int)floorf((position.y - world->spatial_hash.world_min.y) /
                      world->spatial_hash.cell_size);
  int z = (int)floorf((position.z - world->spatial_hash.world_min.z) /
                      world->spatial_hash.cell_size);

  return spatial_hash_coordinate(x, y, z);
}
}

// Global accessor functions
CCDWorld *get_ccd_world(void) { return &g_ccd_world; }

bool init_ccd_system(uint32_t max_entities) {
  return ccd_world_init(&g_ccd_world, max_entities);
}

void cleanup_ccd_system(void) { ccd_world_cleanup(&g_ccd_world); }
