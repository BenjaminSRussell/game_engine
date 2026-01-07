#include "physics/core/physics_types.h"
#include <math.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Helper to access core raycast from physics_core.c
bool core_physics_world_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit);

// Forward declaration of AABB helper exposed in physics_core.c (now non-static)
void core_rigid_body_get_aabb(RigidBody *body, float *min_out, float *max_out);

bool physics_query_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit) {
    return core_physics_world_raycast(world, origin, dir, max_dist, out_hit);
}

// -----------------------------------------------------------------------------
// Overlap Queries
// -----------------------------------------------------------------------------

// Basic AABB overlap test
static bool test_aabb_overlap(const float *min_a, const float *max_a, const float *min_b, const float *max_b) {
    if (max_a[0] < min_b[0] || min_a[0] > max_b[0]) return false;
    if (max_a[1] < min_b[1] || min_a[1] > max_b[1]) return false;
    if (max_a[2] < min_b[2] || min_a[2] > max_b[2]) return false;
    return true;
}

// Sphere-AABB overlap test
static bool test_sphere_aabb(const float *sphere_center, float radius, const float *aabb_min, const float *aabb_max) {
    float closest_point[3];
    for (int i = 0; i < 3; i++) {
        closest_point[i] = fmaxf(aabb_min[i], fminf(sphere_center[i], aabb_max[i]));
    }
    float dx = closest_point[0] - sphere_center[0];
    float dy = closest_point[1] - sphere_center[1];
    float dz = closest_point[2] - sphere_center[2];
    return (dx*dx + dy*dy + dz*dz) <= (radius * radius);
}

int physics_query_overlap_sphere(PhysicsWorld *world, const float *origin, float radius, RigidBody **out_bodies, int max_bodies) {
    if (!world || !out_bodies || max_bodies <= 0) return 0;

    int count = 0;
    float body_min[3], body_max[3];

    // Naive iteration over all bodies.
    // Optimization: Use Broadphase Query if available.
    for (uint32_t i = 0; i < world->body_count; i++) {
        RigidBody *body = world->bodies[i];
        if (!body || !body->is_active || !body->shape) continue;

        core_rigid_body_get_aabb(body, body_min, body_max);

        if (test_sphere_aabb(origin, radius, body_min, body_max)) {
            // Precise check could go here (Sphere vs Shape).
            // For now, AABB check + Sphere is a decent approximation for "Overlap" in this basic engine.
            // If the body is a sphere, we can do sphere-sphere.
            if (body->shape->type == COLLISION_SHAPE_SPHERE) {
                 float r_sum = radius + body->shape->data.sphere.radius;
                 float dx = origin[0] - body->position[0];
                 float dy = origin[1] - body->position[1];
                 float dz = origin[2] - body->position[2];
                 if ((dx*dx + dy*dy + dz*dz) <= (r_sum * r_sum)) {
                     out_bodies[count++] = body;
                 }
            } else {
                // Assume AABB overlap is enough for now or treat other shapes as their bounding box
                out_bodies[count++] = body;
            }

            if (count >= max_bodies) break;
        }
    }
    return count;
}

int physics_query_overlap_box(PhysicsWorld *world, const float *center, const float *half_extents, const float* rotation, RigidBody **out_bodies, int max_bodies) {
    if (!world || !out_bodies || max_bodies <= 0) return 0;

    int count = 0;
    float query_min[3] = { center[0] - half_extents[0], center[1] - half_extents[1], center[2] - half_extents[2] };
    float query_max[3] = { center[0] + half_extents[0], center[1] + half_extents[1], center[2] + half_extents[2] };

    // Naive AABB-AABB check (ignoring rotation for the query box for simplicity, assuming axis aligned query)
    float body_min[3], body_max[3];

    for (uint32_t i = 0; i < world->body_count; i++) {
        RigidBody *body = world->bodies[i];
        if (!body || !body->is_active || !body->shape) continue;

        core_rigid_body_get_aabb(body, body_min, body_max);

        if (test_aabb_overlap(query_min, query_max, body_min, body_max)) {
            out_bodies[count++] = body;
            if (count >= max_bodies) break;
        }
    }
    return count;
}

// -----------------------------------------------------------------------------
// Sweep (Shapecast) Queries
// -----------------------------------------------------------------------------

// Simple Sphere Cast (Sweep)
bool physics_query_sweep_sphere(PhysicsWorld *world, const float *origin, float radius, const float *direction, float max_dist, RayHit *out_hit) {
    if (!world || !out_hit) return false;

    out_hit->hit = false;
    out_hit->distance = max_dist;

    float body_min[3], body_max[3];
    float sweep_min[3], sweep_max[3];

    // Compute bounds for the entire sweep operation for AABB culling
    // Sweep AABB is AABB(start_sphere) U AABB(end_sphere)
    float start_min[3] = { origin[0] - radius, origin[1] - radius, origin[2] - radius };
    float start_max[3] = { origin[0] + radius, origin[1] + radius, origin[2] + radius };
    float end_pos[3] = { origin[0] + direction[0] * max_dist, origin[1] + direction[1] * max_dist, origin[2] + direction[2] * max_dist };
    float end_min[3] = { end_pos[0] - radius, end_pos[1] - radius, end_pos[2] - radius };
    float end_max[3] = { end_pos[0] + radius, end_pos[1] + radius, end_pos[2] + radius };

    for (int k=0; k<3; k++) {
        sweep_min[k] = fminf(start_min[k], end_min[k]);
        sweep_max[k] = fmaxf(start_max[k], end_max[k]);
    }

    for (uint32_t i = 0; i < world->body_count; i++) {
        RigidBody *body = world->bodies[i];
        if (!body || !body->is_active || !body->shape) continue;

        // 1. Broadphase AABB Check
        core_rigid_body_get_aabb(body, body_min, body_max);
        if (!test_aabb_overlap(sweep_min, sweep_max, body_min, body_max)) {
            continue;
        }

        // 2. Precise Shape Check
        if (body->shape->type == COLLISION_SHAPE_SPHERE) {
            float r_sum = radius + body->shape->data.sphere.radius;

            // Raycast against expanded sphere
            float oc[3] = { origin[0] - body->position[0], origin[1] - body->position[1], origin[2] - body->position[2] };
            float b = 2.0f * (oc[0]*direction[0] + oc[1]*direction[1] + oc[2]*direction[2]);
            float c = (oc[0]*oc[0] + oc[1]*oc[1] + oc[2]*oc[2]) - r_sum*r_sum;
            float discrim = b*b - 4*c;

            if (discrim >= 0) {
                float t = (-b - sqrtf(discrim)) * 0.5f;
                if (t < 0) t = (-b + sqrtf(discrim)) * 0.5f;

                if (t >= 0 && t < out_hit->distance) {
                    out_hit->hit = true;
                    out_hit->distance = t;
                    out_hit->body = body;

                    // Hit point on the surface of the body (approx)
                    out_hit->point[0] = origin[0] + direction[0] * t;
                    out_hit->point[1] = origin[1] + direction[1] * t;
                    out_hit->point[2] = origin[2] + direction[2] * t;

                    // Normal
                    float n[3] = { out_hit->point[0] - body->position[0], out_hit->point[1] - body->position[1], out_hit->point[2] - body->position[2] };
                    float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
                    if (len > 0) { n[0]/=len; n[1]/=len; n[2]/=len; }
                    out_hit->normal[0] = n[0]; out_hit->normal[1] = n[1]; out_hit->normal[2] = n[2];
                }
            }
        } else {
            // Fallback: AABB overlap check along the ray?
            // For now, since we passed the sweep AABB vs body AABB check, we know they are close.
            // A simple approximation for Box/Mesh vs Sphere Sweep is to treat the body as a sphere that encloses its AABB,
            // OR just return the hit if the AABB overlaps (which is very inaccurate but safe - i.e. won't miss walls).
            // A better approximation: Raycast against the AABB (expanded by radius).

            // Inflate body AABB by radius
            float expanded_min[3] = { body_min[0] - radius, body_min[1] - radius, body_min[2] - radius };
            float expanded_max[3] = { body_max[0] + radius, body_max[1] + radius, body_max[2] + radius };

            // Ray AABB intersection
            float tmin = -1e9f, tmax = 1e9f;
            for (int k=0; k<3; k++) {
                 if (fabsf(direction[k]) < 1e-6f) {
                     if (origin[k] < expanded_min[k] || origin[k] > expanded_max[k]) {
                         tmin = 1e9f; tmax = -1e9f; // No hit
                         break;
                     }
                 } else {
                     float invD = 1.0f / direction[k];
                     float t1 = (expanded_min[k] - origin[k]) * invD;
                     float t2 = (expanded_max[k] - origin[k]) * invD;
                     if (t1 > t2) { float temp=t1; t1=t2; t2=temp; }
                     if (t1 > tmin) tmin = t1;
                     if (t2 < tmax) tmax = t2;
                 }
            }

            if (tmin <= tmax && tmax >= 0 && tmin < out_hit->distance) {
                // If tmin < 0, we started inside the expanded AABB.
                float t = (tmin < 0) ? 0.0f : tmin;

                out_hit->hit = true;
                out_hit->distance = t;
                out_hit->body = body;
                out_hit->point[0] = origin[0] + direction[0] * t;
                out_hit->point[1] = origin[1] + direction[1] * t;
                out_hit->point[2] = origin[2] + direction[2] * t;
                // Normal approximation (generic up)
                out_hit->normal[0] = 0; out_hit->normal[1] = 1; out_hit->normal[2] = 0;
            }
        }
    }

    return out_hit->hit;
}
