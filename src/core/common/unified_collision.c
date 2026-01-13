/*
 * unified_collision.c
 * Implementation of unified collision system
 */

#include "unified_collision.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static struct {
    collision_world_t *worlds[16];
    uint32_t world_count;
    collision_stats_t global_stats;
    bool initialized;
} g_collision_state = {0};

/* ============================================================================
 * WORLD MANAGEMENT
 * ============================================================================ */

collision_world_t* collision_world_create(uint32_t max_primitives) {
    collision_world_t *world = (collision_world_t*)malloc(sizeof(collision_world_t));
    if (!world) return NULL;
    
    memset(world, 0, sizeof(collision_world_t));
    
    world->primitives = (collision_primitive_t*)malloc(max_primitives * sizeof(collision_primitive_t));
    world->pairs = (collision_pair_t*)malloc(max_primitives * sizeof(collision_pair_t));
    world->broadphase_pairs = (bool*)malloc(max_primitives * max_primitives * sizeof(bool));
    
    if (!world->primitives || !world->pairs || !world->broadphase_pairs) {
        collision_world_destroy(world);
        return NULL;
    }
    
    world->capacity = max_primitives;
    world->pair_capacity = max_primitives;
    world->broadphase_size = max_primitives * max_primitives;
    
    memset(world->primitives, 0, max_primitives * sizeof(collision_primitive_t));
    memset(world->pairs, 0, max_primitives * sizeof(collision_pair_t));
    memset(world->broadphase_pairs, 0, max_primitives * max_primitives * sizeof(bool));
    
    return world;
}

void collision_world_destroy(collision_world_t *world) {
    if (!world) return;
    
    if (world->primitives) free(world->primitives);
    if (world->pairs) free(world->pairs);
    if (world->broadphase_pairs) free(world->broadphase_pairs);
    
    free(world);
}

void collision_world_clear(collision_world_t *world) {
    if (!world) return;
    
    world->count = 0;
    world->pair_count = 0;
    memset(world->primitives, 0, world->capacity * sizeof(collision_primitive_t));
    memset(world->pairs, 0, world->pair_capacity * sizeof(collision_pair_t));
    memset(world->broadphase_pairs, 0, world->broadphase_size * sizeof(bool));
}

/* ============================================================================
 * PRIMITIVE MANAGEMENT
 * ============================================================================ */

uint32_t collision_add_sphere(collision_world_t *world, const vec3_t *center, float radius) {
    if (!world || !center || world->count >= world->capacity) {
        return UNIFIED_COLLISION_INVALID_ID;
    }
    
    uint32_t id = world->count++;
    collision_primitive_t *primitive = &world->primitives[id];
    
    primitive->type = COLLISION_PRIMITIVE_SPHERE;
    primitive->position = *center;
    primitive->enabled = true;
    primitive->friction = 0.5f;
    primitive->restitution = 0.3f;
    primitive->response_type = COLLISION_RESPONSE_SLIDE;
    
    /* Initialize identity transform */
    memset(&primitive->transform, 0, sizeof(mat4_t));
    primitive->transform.m[0][0] = primitive->transform.m[1][1] = primitive->transform.m[2][2] = primitive->transform.m[3][3] = 1.0f;
    
    return id;
}

uint32_t collision_add_box(collision_world_t *world, const vec3_t *center, const vec3_t *extents) {
    if (!world || !center || !extents || world->count >= world->capacity) {
        return UNIFIED_COLLISION_INVALID_ID;
    }
    
    uint32_t id = world->count++;
    collision_primitive_t *primitive = &world->primitives[id];
    
    primitive->type = COLLISION_PRIMITIVE_BOX;
    primitive->position = *center;
    primitive->enabled = true;
    primitive->friction = 0.5f;
    primitive->restitution = 0.2f;
    primitive->response_type = COLLISION_RESPONSE_SLIDE;
    
    /* Initialize identity transform */
    memset(&primitive->transform, 0, sizeof(mat4_t));
    primitive->transform.m[0][0] = primitive->transform.m[1][1] = primitive->transform.m[2][2] = primitive->transform.m[3][3] = 1.0f;
    
    return id;
}

uint32_t collision_add_capsule(collision_world_t *world, const vec3_t *start, const vec3_t *end, float radius) {
    if (!world || !start || !end || world->count >= world->capacity) {
        return UNIFIED_COLLISION_INVALID_ID;
    }
    
    uint32_t id = world->count++;
    collision_primitive_t *primitive = &world->primitives[id];
    
    primitive->type = COLLISION_PRIMITIVE_CAPSULE;
    vec3_t center;
    vec3_add(&center, start, end);
    vec3_mul(&center, &center, 0.5f);
    primitive->position = center;
    primitive->enabled = true;
    primitive->friction = 0.5f;
    primitive->restitution = 0.3f;
    primitive->response_type = COLLISION_RESPONSE_SLIDE;
    
    /* Initialize identity transform */
    memset(&primitive->transform, 0, sizeof(mat4_t));
    primitive->transform.m[0][0] = primitive->transform.m[1][1] = primitive->transform.m[2][2] = primitive->transform.m[3][3] = 1.0f;
    
    return id;
}

void collision_remove_primitive(collision_world_t *world, uint32_t primitive_id) {
    if (!world || primitive_id >= world->count) return;
    
    world->primitives[primitive_id].enabled = false;
}

void collision_set_primitive_transform(collision_world_t *world, uint32_t primitive_id, const mat4_t *transform) {
    if (!world || primitive_id >= world->count || !transform) return;
    
    world->primitives[primitive_id].transform = *transform;
    
    /* Update position from transform */
    world->primitives[primitive_id].position.x = transform->m[3][0];
    world->primitives[primitive_id].position.y = transform->m[3][1];
    world->primitives[primitive_id].position.z = transform->m[3][2];
}

/* ============================================================================
 * COLLISION DETECTION FUNCTIONS
 * ============================================================================ */

bool collision_sphere_sphere(const collision_sphere_t *a, const collision_sphere_t *b, collision_contact_t *contact) {
    if (!a || !b || !contact) return false;
    
    vec3_t diff;
    vec3_sub(&diff, &a->center, &b->center);
    
    float distance_sq = vec3_length_sq(&diff);
    float radius_sum = a->radius + b->radius;
    float radius_sum_sq = radius_sum * radius_sum;
    
    if (distance_sq > radius_sum_sq) {
        return false;
    }
    
    float distance = sqrtf(distance_sq);
    if (distance < UNIFIED_COLLISION_TOLERANCE) {
        /* Spheres are at same position, use arbitrary normal */
        contact->normal.x = 1.0f;
        contact->normal.y = 0.0f;
        contact->normal.z = 0.0f;
        contact->penetration_depth = radius_sum;
    } else {
        /* Calculate contact normal and penetration */
        vec3_normalize(&contact->normal, &diff);
        contact->penetration_depth = radius_sum - distance;
    }
    
    /* Calculate contact point (midpoint of penetration) */
    vec3_t penetration_vec;
    vec3_mul(&penetration_vec, &contact->normal, contact->penetration_depth * 0.5f);
    vec3_add(&contact->point, &a->center, &penetration_vec);
    
    contact->valid = true;
    contact->impulse = 0.0f;
    
    return true;
}

bool collision_sphere_box(const collision_sphere_t *sphere, const collision_box_t *box, collision_contact_t *contact) {
    if (!sphere || !box || !contact) return false;
    
    /* Find closest point on box to sphere center */
    vec3_t closest;
    vec3_t diff;
    
    diff.x = sphere->center.x - box->center.x;
    diff.y = sphere->center.y - box->center.y;
    diff.z = sphere->center.z - box->center.z;
    
    closest.x = (diff.x > box->extents.x) ? box->center.x + box->extents.x :
                (diff.x < -box->extents.x) ? box->center.x - box->extents.x : sphere->center.x;
    closest.y = (diff.y > box->extents.y) ? box->center.y + box->extents.y :
                (diff.y < -box->extents.y) ? box->center.y - box->extents.y : sphere->center.y;
    closest.z = (diff.z > box->extents.z) ? box->center.z + box->extents.z :
                (diff.z < -box->extents.z) ? box->center.z - box->extents.z : sphere->center.z;
    
    /* Check if closest point is inside sphere */
    vec3_sub(&diff, &sphere->center, &closest);
    float distance_sq = vec3_length_sq(&diff);
    
    if (distance_sq > sphere->radius * sphere->radius) {
        return false;
    }
    
    float distance = sqrtf(distance_sq);
    if (distance < UNIFIED_COLLISION_TOLERANCE) {
        /* Sphere center is on box surface */
        vec3_sub(&diff, &sphere->center, &box->center);
        vec3_normalize(&contact->normal, &diff);
        contact->penetration_depth = sphere->radius;
    } else {
        /* Calculate normal from box to sphere */
        vec3_normalize(&contact->normal, &diff);
        contact->penetration_depth = sphere->radius - distance;
    }
    
    contact->point = closest;
    contact->valid = true;
    contact->impulse = 0.0f;
    
    return true;
}

/* ============================================================================
 * BROADPHASE COLLISION DETECTION
 * ============================================================================ */

void collision_update_broadphase(collision_world_t *world) {
    if (!world) return;
    
    /* Simple O(n^2) broadphase - in production, use spatial partitioning */
    for (uint32_t i = 0; i < world->count; i++) {
        for (uint32_t j = i + 1; j < world->count; j++) {
            if (!world->primitives[i].enabled || !world->primitives[j].enabled) continue;
            
            /* Simple AABB check for broadphase */
            vec3_t diff;
            vec3_sub(&diff, &world->primitives[i].position, &world->primitives[j].position);
            
            float combined_radius = 1.0f; /* Simplified - use actual bounds */
            if (vec3_length_sq(&diff) < combined_radius * combined_radius) {
                uint32_t index = i * world->capacity + j;
                world->broadphase_pairs[index] = true;
            }
        }
    }
}

/* ============================================================================
 * NARROWPHASE COLLISION DETECTION
 * ============================================================================ */

void collision_update_narrowphase(collision_world_t *world) {
    if (!world) return;
    
    world->pair_count = 0;
    
    for (uint32_t i = 0; i < world->count; i++) {
        for (uint32_t j = i + 1; j < world->count; j++) {
            if (!world->primitives[i].enabled || !world->primitives[j].enabled) continue;
            
            uint32_t broadphase_index = i * world->capacity + j;
            if (!world->broadphase_pairs[broadphase_index]) continue;
            
            collision_pair_t *pair = &world->pairs[world->pair_count];
            pair->primitive_a = i;
            pair->primitive_b = j;
            pair->contact_count = 0;
            
            /* Perform narrowphase test based on primitive types */
            collision_primitive_t *a = &world->primitives[i];
            collision_primitive_t *b = &world->primitives[j];
            
            /* Simplified - handle sphere-sphere only for now */
            if (a->type == COLLISION_PRIMITIVE_SPHERE && b->type == COLLISION_PRIMITIVE_SPHERE) {
                collision_sphere_t sphere_a = {a->position, 1.0f}; /* Simplified radius */
                collision_sphere_t sphere_b = {b->position, 1.0f};
                
                if (collision_sphere_sphere(&sphere_a, &sphere_b, &pair->contacts[0])) {
                    pair->contact_count = 1;
                    world->pair_count++;
                }
            }
        }
    }
}

/* ============================================================================
 * CONTACT RESOLUTION
 * ============================================================================ */

void collision_resolve_contacts(collision_world_t *world, float dt) {
    if (!world || dt <= 0.0f) return;
    
    for (uint32_t i = 0; i < world->pair_count; i++) {
        collision_pair_t *pair = &world->pairs[i];
        
        for (uint32_t j = 0; j < pair->contact_count; j++) {
            collision_contact_t *contact = &pair->contacts[j];
            
            /* Apply position correction */
            vec3_t correction;
            vec3_mul(&correction, &contact->normal, contact->penetration_depth * UNIFIED_COLLISION_BIAS_FACTOR);
            
            /* Simple position correction - in production, use mass ratios */
            vec3_sub(&world->primitives[pair->primitive_a].position, 
                     &world->primitives[pair->primitive_a].position, &correction);
            vec3_add(&world->primitives[pair->primitive_b].position, 
                     &world->primitives[pair->primitive_b].position, &correction);
        }
    }
}

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================ */

bool collision_ray_cast(const collision_world_t *world, const vec3_t *start, const vec3_t *direction, 
                       float max_distance, collision_contact_t *result) {
    if (!world || !start || !direction || !result) return false;
    
    vec3_t normalized_dir;
    vec3_normalize(&normalized_dir, direction);
    
    float closest_distance = max_distance;
    bool hit = false;
    
    for (uint32_t i = 0; i < world->count; i++) {
        if (!world->primitives[i].enabled) continue;
        
        collision_primitive_t *primitive = &world->primitives[i];
        
        /* Simplified sphere raycast */
        if (primitive->type == COLLISION_PRIMITIVE_SPHERE) {
            float t;
            if (ray_sphere_intersect(start, &normalized_dir, &primitive->position, 1.0f, &t)) {
                if (t < closest_distance) {
                    closest_distance = t;
                    hit = true;
                    
                    /* Calculate hit point and normal */
                    vec3_mul(&result->point, &normalized_dir, t);
                    vec3_add(&result->point, start, &result->point);
                    
                    vec3_sub(&result->normal, &result->point, &primitive->position);
                    vec3_normalize(&result->normal, &result->normal);
                    
                    result->penetration_depth = 0.0f;
                    result->valid = true;
                }
            }
        }
    }
    
    return hit;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

float distance_point_to_plane(const vec3_t *point, const vec3_t *plane_normal, float plane_distance) {
    if (!point || !plane_normal) return 0.0f;
    return vec3_dot(point, plane_normal) - plane_distance;
}

void closest_point_on_line(const vec3_t *point, const vec3_t *line_start, const vec3_t *line_end, vec3_t *closest) {
    if (!point || !line_start || !line_end || !closest) return;
    
    vec3_t line_vec;
    vec3_sub(&line_vec, line_end, line_start);
    
    vec3_t point_vec;
    vec3_sub(&point_vec, point, line_start);
    
    float line_length_sq = vec3_length_sq(&line_vec);
    if (line_length_sq < UNIFIED_COLLISION_TOLERANCE) {
        *closest = *line_start;
        return;
    }
    
    float t = vec3_dot(&point_vec, &line_vec) / line_length_sq;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
    
    vec3_mul(closest, &line_vec, t);
    vec3_add(closest, closest, line_start);
}

bool ray_sphere_intersect(const vec3_t *ray_start, const vec3_t *ray_dir, const vec3_t *sphere_center, 
                        float sphere_radius, float *t) {
    if (!ray_start || !ray_dir || !sphere_center || !t) return false;
    
    vec3_t oc;
    vec3_sub(&oc, ray_start, sphere_center);
    
    float a = vec3_dot(ray_dir, ray_dir);
    float b = 2.0f * vec3_dot(&oc, ray_dir);
    float c = vec3_dot(&oc, &oc) - sphere_radius * sphere_radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0.0f) return false;
    
    float sqrt_discriminant = sqrtf(discriminant);
    float t1 = (-b - sqrt_discriminant) / (2.0f * a);
    float t2 = (-b + sqrt_discriminant) / (2.0f * a);
    
    *t = (t1 > 0.0f) ? t1 : t2;
    return *t > 0.0f;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

void collision_get_stats(const collision_world_t *world, collision_stats_t *stats) {
    if (!world || !stats) return;
    
    memset(stats, 0, sizeof(collision_stats_t));
    
    stats->total_primitives = world->capacity;
    stats->active_primitives = 0;
    
    for (uint32_t i = 0; i < world->count; i++) {
        if (world->primitives[i].enabled) {
            stats->active_primitives++;
        }
    }
    
    stats->collision_pairs = world->pair_count;
    stats->contact_points = 0;
    
    for (uint32_t i = 0; i < world->pair_count; i++) {
        stats->contact_points += world->pairs[i].contact_count;
    }
}

void collision_reset_stats(collision_world_t *world) {
    if (!world) return;
    
    /* Clear broadphase pairs */
    memset(world->broadphase_pairs, 0, world->broadphase_size * sizeof(bool));
    world->pair_count = 0;
}
