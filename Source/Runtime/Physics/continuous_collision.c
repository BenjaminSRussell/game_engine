// continuous_collision.c - CCD Implementation
#include <include/physics/continuous_collision.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <include/math/quat.h>
#include <include/physics/physics.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declarations for internal helpers
static void free_shape(CCDShape *shape);
static CCDBroadphaseEntry* find_entry(CCDWorld *world, EntityID entity);

// -----------------------------------------------------------------------------
// World Management
// -----------------------------------------------------------------------------

bool ccd_world_init(CCDWorld *world, uint32_t max_entities) {
    if (!world) return false;

    memset(world, 0, sizeof(CCDWorld));

    world->entry_capacity = max_entities > 0 ? max_entities : 1024;
    world->entries = (CCDBroadphaseEntry*)calloc(world->entry_capacity, sizeof(CCDBroadphaseEntry));

    if (!world->entries) {
        LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate CCD world entries");
        return false;
    }

    world->max_time_step = 1.0f / 60.0f;
    world->ccd_threshold = 0.1f; // Default threshold
    world->enable_speculative_contacts = true;
    world->enable_motion_clamping = true;
    world->max_substeps = 4;

    // Spatial hash init (stub for now, using brute force)
    world->spatial_hash.table_size = 1024; // Example

    LOG_INFO(LOG_CAT_PHYSICS, "CCD World initialized with capacity %d", world->entry_capacity);
    return true;
}

void ccd_world_cleanup(CCDWorld *world) {
    if (!world) return;

    if (world->entries) {
        for (uint32_t i = 0; i < world->entry_count; i++) {
            if (world->entries[i].shape_ptr) {
                free_shape((CCDShape*)world->entries[i].shape_ptr);
            }
        }
        free(world->entries);
    }

    // Free spatial hash resources if allocated
    if (world->spatial_hash.hash_table) free(world->spatial_hash.hash_table);
    if (world->spatial_hash.entry_lists) free(world->spatial_hash.entry_lists);

    memset(world, 0, sizeof(CCDWorld));
    LOG_INFO(LOG_CAT_PHYSICS, "CCD World cleanup complete");
}

static CCDBroadphaseEntry* get_free_entry(CCDWorld *world) {
    if (world->entry_count >= world->entry_capacity) return NULL;
    return &world->entries[world->entry_count++];
}

static void free_shape(CCDShape *shape) {
    if (!shape) return;
    if (shape->type == CCD_SHAPE_TRIANGLE_MESH) {
        CCDTriangleMesh *mesh = (CCDTriangleMesh*)shape;
        if (mesh->triangles) free(mesh->triangles);
        if (mesh->vertices) free(mesh->vertices);
        if (mesh->indices) free(mesh->indices);
    }
    free(shape);
}

// -----------------------------------------------------------------------------
// Shape Management
// -----------------------------------------------------------------------------

bool ccd_world_add_sphere(CCDWorld *world, EntityID entity, const CCDSphere *sphere) {
    if (!world || !sphere) return false;
    
    CCDBroadphaseEntry *entry = get_free_entry(world);
    if (!entry) return false;

    CCDSphere *shape_copy = (CCDSphere*)malloc(sizeof(CCDSphere));
    if (!shape_copy) return false;
    memcpy(shape_copy, sphere, sizeof(CCDSphere));
    shape_copy->base.type = CCD_SHAPE_SPHERE;

    entry->entity = entity;
    entry->shape_type = CCD_SHAPE_SPHERE;
    entry->shape_ptr = (CCDShape*)shape_copy;
    entry->collision_group = sphere->base.collision_group;
    entry->collision_mask = sphere->base.collision_mask;
    entry->is_static = sphere->base.is_static;
    entry->position = sphere->base.position;
    entry->velocity = sphere->base.linear_velocity;

    // Calculate bounds
    float r = sphere->radius;
    entry->bounds_min = vec3_sub(entry->position, (Vec3){r, r, r});
    entry->bounds_max = vec3_add(entry->position, (Vec3){r, r, r});

    return true;
}

bool ccd_world_add_box(CCDWorld *world, EntityID entity, const CCDBox *box) {
    if (!world || !box) return false;

    CCDBroadphaseEntry *entry = get_free_entry(world);
    if (!entry) return false;

    CCDBox *shape_copy = (CCDBox*)malloc(sizeof(CCDBox));
    if (!shape_copy) return false;
    memcpy(shape_copy, box, sizeof(CCDBox));
    shape_copy->base.type = CCD_SHAPE_BOX;

    entry->entity = entity;
    entry->shape_type = CCD_SHAPE_BOX;
    entry->shape_ptr = (CCDShape*)shape_copy;
    entry->collision_group = box->base.collision_group;
    entry->collision_mask = box->base.collision_mask;
    entry->is_static = box->base.is_static;
    entry->position = box->base.position;
    entry->velocity = box->base.linear_velocity;

    entry->bounds_min = vec3_sub(entry->position, box->half_extents);
    entry->bounds_max = vec3_add(entry->position, box->half_extents);

    return true;
}

bool ccd_world_add_capsule(CCDWorld *world, EntityID entity, const CCDCapsule *capsule) {
    if (!world || !capsule) return false;

    CCDBroadphaseEntry *entry = get_free_entry(world);
    if (!entry) return false;

    CCDCapsule *shape_copy = (CCDCapsule*)malloc(sizeof(CCDCapsule));
    if (!shape_copy) return false;
    memcpy(shape_copy, capsule, sizeof(CCDCapsule));
    shape_copy->base.type = CCD_SHAPE_CAPSULE;

    entry->entity = entity;
    entry->shape_type = CCD_SHAPE_CAPSULE;
    entry->shape_ptr = (CCDShape*)shape_copy;
    entry->collision_group = capsule->base.collision_group;
    entry->collision_mask = capsule->base.collision_mask;
    entry->is_static = capsule->base.is_static;
    entry->position = capsule->base.position;
    entry->velocity = capsule->base.linear_velocity;

    // Approx bounds
    float r = capsule->radius;
    float h = capsule->height;
    entry->bounds_min = vec3_sub(entry->position, (Vec3){r, r + h/2.0f, r});
    entry->bounds_max = vec3_add(entry->position, (Vec3){r, r + h/2.0f, r});

    return true;
}

bool ccd_world_remove_entity(CCDWorld *world, EntityID entity) {
    if (!world) return false;
    
    for (uint32_t i = 0; i < world->entry_count; i++) {
        if (world->entries[i].entity == entity) {
            // Free shape
            if (world->entries[i].shape_ptr) {
                free_shape((CCDShape*)world->entries[i].shape_ptr);
            }
            // Move last to here
            if (i < world->entry_count - 1) {
                world->entries[i] = world->entries[world->entry_count - 1];
            }
            world->entry_count--;
            return true;
        }
    }
    return false;
}

static CCDBroadphaseEntry* find_entry(CCDWorld *world, EntityID entity) {
    for (uint32_t i = 0; i < world->entry_count; i++) {
        if (world->entries[i].entity == entity) {
            return &world->entries[i];
        }
    }
    return NULL;
}

bool ccd_world_update_entity_motion(CCDWorld *world, EntityID entity,
                                    Vec3 linear_velocity,
                                    Vec3 angular_velocity) {
    CCDBroadphaseEntry *entry = find_entry(world, entity);
    if (!entry) return false;

    entry->velocity = linear_velocity;

    // Update shape velocity as well
    CCDShape *shape = (CCDShape*)entry->shape_ptr;
    if (shape) {
        shape->linear_velocity = linear_velocity;
        shape->angular_velocity = angular_velocity;
    }
    return true;
}

// -----------------------------------------------------------------------------
// Update Loop
// -----------------------------------------------------------------------------

void ccd_world_update(CCDWorld *world, float time_step) {
    if (!world) return;

    ccd_reset_performance_stats(world);

    // 1. Update positions and bounds (Swept bounds)
    for (uint32_t i = 0; i < world->entry_count; i++) {
        CCDBroadphaseEntry *entry = &world->entries[i];
        CCDShape *shape = (CCDShape*)entry->shape_ptr;

        if (shape) {
            shape->position = entry->position; // Sync

            // Calculate swept bounds
            ccd_calculate_swept_bounds(shape, time_step, &entry->bounds_min, &entry->bounds_max);
        }
    }

    // 2. Broadphase (Simple O(N^2) for now)
    // In a real high-perf system, use the spatial hash or sweep-and-prune
    for (uint32_t i = 0; i < world->entry_count; i++) {
        CCDBroadphaseEntry *entry_a = &world->entries[i];

        for (uint32_t j = i + 1; j < world->entry_count; j++) {
            CCDBroadphaseEntry *entry_b = &world->entries[j];

            // Filter
            if (entry_a->is_static && entry_b->is_static) continue;
            // if (!(entry_a->collision_mask & entry_b->collision_group)) continue; // Basic filtering

            world->broadphase_tests++;

            // AABB check
            if (ccd_bounds_intersect(entry_a->bounds_min, entry_a->bounds_max,
                                     entry_b->bounds_min, entry_b->bounds_max)) {

                // world->broadphase_hits++; // Not in struct
                world->narrowphase_tests++;

                // Narrowphase
                CCDCollisionResult result;
                memset(&result, 0, sizeof(result));

                if (ccd_detect_collision(entry_a->shape_ptr, entry_b->shape_ptr, time_step, &result)) {
                    result.entity_a = entry_a->entity;
                    result.entity_b = entry_b->entity;

                    world->collisions_found++;
                    ccd_resolve_collision(world, &result);
                }
            }
        }
    }

    // 3. Integrate Motion
    for (uint32_t i = 0; i < world->entry_count; i++) {
        CCDBroadphaseEntry *entry = &world->entries[i];
        CCDShape *shape = (CCDShape*)entry->shape_ptr;

        if (shape && !shape->is_static) {
             ccd_integrate_motion(shape, time_step);
             entry->position = shape->position;
        }
    }
}

// -----------------------------------------------------------------------------
// Collision Detection
// -----------------------------------------------------------------------------

bool ccd_detect_collision(const CCDShape *shape_a, const CCDShape *shape_b,
                          float time_step, CCDCollisionResult *result) {
    if (shape_a->type == CCD_SHAPE_SPHERE && shape_b->type == CCD_SHAPE_SPHERE) {
        return ccd_sphere_vs_sphere((const CCDSphere*)shape_a, (const CCDSphere*)shape_b, time_step, result);
    }
    // Implement other pairs as needed
    return false;
}

// Ray vs Sphere helper
static bool ray_vs_sphere(const Vec3 *origin, const Vec3 *dir, float max_dist,
                       const Vec3 *center, float radius, CCDCollisionResult *result) {
  Vec3 to_center = vec3_sub(*center, *origin);
  float projection = vec3_dot(to_center, *dir); // Fix: dereference
  
  if (projection < 0 || projection > max_dist) return false;
  
  Vec3 closest_point = vec3_add(*origin, vec3_mul(*dir, projection)); // vec3_mul for scalar
  Vec3 to_closest = vec3_sub(*center, closest_point);
  float dist_sq = vec3_length_sq(to_closest); // Fix: dereference
  
  if (dist_sq <= radius * radius) {
    if (result) {
        result->hit = true;
        result->time_of_impact = projection / max_dist;
        result->contact_point = closest_point;
        result->contact_normal = vec3_normalize(to_closest);
        result->penetration_depth = radius - sqrtf(dist_sq);
    }
    return true;
  }
  return false;
}

bool ccd_sphere_vs_sphere(const CCDSphere *sphere_a, const CCDSphere *sphere_b,
                          float time_step, CCDCollisionResult *result) {
    // Relative motion: Treat B as static, A moving with rel velocity
    Vec3 vel_a = sphere_a->base.linear_velocity;
    Vec3 vel_b = sphere_b->base.linear_velocity;
    Vec3 rel_vel = vec3_sub(vel_a, vel_b);

    float rel_speed = vec3_length(rel_vel); // Fix: dereference
    if (rel_speed < 0.0001f) return false;

    float dist = rel_speed * time_step;
    Vec3 dir = vec3_mul(rel_vel, 1.0f / rel_speed); // vec3_mul for scalar

    float combined_radius = sphere_a->radius + sphere_b->radius;

    // Ray cast from A center to B (expanded by radius)
    Vec3 start = sphere_a->base.position;
    Vec3 end = vec3_add(start, vec3_mul(rel_vel, time_step)); // vec3_mul for scalar

    if (ray_vs_sphere(&start, &dir, dist, &sphere_b->base.position, combined_radius, result)) {
        // Adjust normal (it's center to center at impact)
        // result->contact_normal is already correct from ray_vs_sphere (relative to combined center)
        return true;
    }

    return false;
}

bool ccd_ray_cast(const CCDWorld *world, Vec3 ray_start, Vec3 ray_direction,
                  float max_distance, CCDCollisionResult *result) {
    if (!world || !result) return false;

    result->hit = false;
    float closest_dist = max_distance; // Store closest locally

    bool hit_any = false;

    for (uint32_t i = 0; i < world->entry_count; i++) {
        CCDBroadphaseEntry *entry = &world->entries[i];
        CCDShape *shape = (CCDShape*)entry->shape_ptr;

        CCDCollisionResult temp_res;
        bool hit = false;

        if (shape->type == CCD_SHAPE_SPHERE) {
            hit = ccd_ray_cast_sphere((const CCDSphere*)shape, ray_start, ray_direction, max_distance, &temp_res);
        } else if (shape->type == CCD_SHAPE_BOX) {
            hit = ccd_ray_cast_box((const CCDBox*)shape, ray_start, ray_direction, max_distance, &temp_res);
        }

        if (hit) {
            float dist = vec3_distance(ray_start, temp_res.contact_point); // Fix: dereference
            if (dist < closest_dist) {
                *result = temp_res;
                closest_dist = dist;
                result->entity_b = entry->entity;
                result->shape_b = shape->type;
                hit_any = true;
            }
        }
    }
    return hit_any;
}

bool ccd_ray_cast_sphere(const CCDSphere *sphere, Vec3 ray_start,
                         Vec3 ray_direction, float max_distance,
                         CCDCollisionResult *result) {
    return ray_vs_sphere(&ray_start, &ray_direction, max_distance, &sphere->base.position, sphere->radius, result);
}

bool ccd_ray_cast_box(const CCDBox *box, Vec3 ray_start, Vec3 ray_direction,
                      float max_distance, CCDCollisionResult *result) {
    // Basic AABB raycast or OBB if rotated (ignoring rotation for simplicity or implementing slab method)
    // Placeholder for now
    return false;
}

// -----------------------------------------------------------------------------
// Utilities & Resolution
// -----------------------------------------------------------------------------

bool ccd_should_use_ccd(const CCDShape *shape, float time_step) {
    if (!shape) return false;
    float speed = vec3_length(shape->linear_velocity); // Fix: dereference
    float min_dim = 1.0f; // Default
    if (shape->type == CCD_SHAPE_SPHERE) min_dim = ((CCDSphere*)shape)->radius;
    // ... other shapes

    // If movement > min_dimension / 2, use CCD
    return (speed * time_step) > (min_dim * 0.5f);
}

float ccd_calculate_swept_bounds(const CCDShape *shape, float time_step,
                                 Vec3 *bounds_min, Vec3 *bounds_max) {
    if (!shape) return 0.0f;

    Vec3 start_pos = shape->position;
    Vec3 end_pos = vec3_add(start_pos, vec3_mul(shape->linear_velocity, time_step)); // vec3_mul for scalar

    Vec3 min_p, max_p;
    // Current bounds at start
    // (Assume sphere for simplicity of bounds calc for now, or use switch)
    float radius = 1.0f;
    if (shape->type == CCD_SHAPE_SPHERE) radius = ((CCDSphere*)shape)->radius;

    Vec3 size = {radius, radius, radius};

    Vec3 b_min_start = vec3_sub(start_pos, size);
    Vec3 b_max_start = vec3_add(start_pos, size);
    Vec3 b_min_end = vec3_sub(end_pos, size);
    Vec3 b_max_end = vec3_add(end_pos, size);

    *bounds_min = vec3_min(b_min_start, b_min_end);
    *bounds_max = vec3_max(b_max_start, b_max_end);

    return vec3_distance(start_pos, end_pos); // Fix: dereference
}

bool ccd_bounds_intersect(Vec3 min_a, Vec3 max_a, Vec3 min_b, Vec3 max_b) {
    return (min_a.x <= max_b.x && max_a.x >= min_b.x) &&
           (min_a.y <= max_b.y && max_a.y >= min_b.y) &&
           (min_a.z <= max_b.z && max_a.z >= min_b.z);
}

void ccd_integrate_motion(CCDShape *shape, float time_step) {
    if (shape) {
        shape->position = vec3_add(shape->position, vec3_mul(shape->linear_velocity, time_step)); // vec3_mul for scalar
    }
}

void ccd_resolve_collision(CCDWorld *world,
                           const CCDCollisionResult *collision) {
    // This function needs to communicate back to the Physics World or RigidBody
    // Since CCDWorld doesn't have direct access to RigidBody pointers,
    // we use the EntityID to look it up if we include physics_integration.

    // For now, we can log it.
    // In a real integration, we might apply a position correction or impulse.
    // LOG_DEBUG(LOG_CAT_PHYSICS, "CCD Collision resolved between %d and %d", collision->entity_a, collision->entity_b);

    // Apply position correction (snap to impact point)
    // This requires updating the CCDShape and potentially the external RigidBody.

    // Simple response: Stop at impact.
    // We need to fetch the shape to update it.
    CCDBroadphaseEntry *entry_a = find_entry(world, collision->entity_a);
    if (entry_a && entry_a->shape_ptr) {
        CCDShape *shape = (CCDShape*)entry_a->shape_ptr;
        // Move to contact point (minus radius along normal)
        // shape->position = ...
    }
}

void ccd_reset_performance_stats(CCDWorld *world) {
    if (world) {
        world->broadphase_tests = 0;
        world->narrowphase_tests = 0;
        world->collisions_found = 0;
    }
}

// Stubs for other required functions
bool ccd_sphere_vs_box(const CCDSphere *sphere, const CCDBox *box,
                       float time_step, CCDCollisionResult *result) { return false; }
bool ccd_sphere_vs_capsule(const CCDSphere *sphere, const CCDCapsule *capsule,
                           float time_step, CCDCollisionResult *result) { return false; }
bool ccd_box_vs_box(const CCDBox *box_a, const CCDBox *box_b, float time_step,
                    CCDCollisionResult *result) { return false; }
bool ccd_capsule_vs_capsule(const CCDCapsule *capsule_a,
                            const CCDCapsule *capsule_b, float time_step,
                            CCDCollisionResult *result) { return false; }
void ccd_broadphase_update(CCDWorld *world) {}
void ccd_spatial_hash_init(CCDWorld *world) {}
void ccd_spatial_hash_insert(CCDWorld *world, uint32_t entry_index) {}
void ccd_spatial_hash_remove(CCDWorld *world, uint32_t entry_index) {}
void ccd_spatial_hash_query(const CCDWorld *world, Vec3 bounds_min,
                            Vec3 bounds_max, uint32_t *results,
                            uint32_t *result_count) {}
uint32_t ccd_spatial_hash_position_to_cell(const CCDWorld *world,
                                           Vec3 position) { return 0; }
void ccd_update_broadphase_bounds(CCDWorld *world, uint32_t entry_index) {}
void ccd_apply_impulse(const CCDCollisionResult *collision,
                       float impulse_magnitude) {}
void ccd_apply_position_correction(const CCDCollisionResult *collision,
                                   float correction_magnitude) {}
Vec3 ccd_extrapolate_position(const CCDShape *shape, float time) { return shape ? shape->position : (Vec3){0}; }
Quat ccd_extrapolate_rotation(const CCDShape *shape, float time) { return shape ? shape->rotation : (Quat){0,0,0,1}; }
void ccd_debug_draw_bounds(const CCDWorld *world) {}
void ccd_debug_draw_contacts(const CCDWorld *world) {}
void ccd_debug_draw_swept_shapes(const CCDWorld *world, float time_step) {}
void ccd_get_performance_stats(const CCDWorld *world,
                               CCDPerformanceStats *stats) {}
bool ccd_world_add_triangle_mesh(CCDWorld *world, EntityID entity,
                                 const CCDTriangleMesh *mesh) { return false; }
