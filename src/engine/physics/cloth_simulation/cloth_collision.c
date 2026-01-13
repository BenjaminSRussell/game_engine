/**
 * @file cloth_collision.c
 * @brief Cloth collision primitives and particle resolution.
 */

#include "cloth_collision.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CLOTH_COLLISION_INVALID_ID 0xFFFFFFFFu

struct ClothCollisionWorld {
    ClothCollisionPrimitive *primitives;
    uint32_t count;
    uint32_t capacity;
    ClothCollisionStats stats;
    ClothCollisionGpuProfiler profiler;
    bool profiling_enabled;
};

static inline void vec3_copy(float *dst, const float *src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline void vec3_add(float *out, const float *a, const float *b) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

static inline void vec3_sub(float *out, const float *a, const float *b) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline void vec3_mul(float *out, const float *v, float s) {
    out[0] = v[0] * s;
    out[1] = v[1] * s;
    out[2] = v[2] * s;
}

static inline float vec3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline float vec3_length_sq(const float *v) {
    return vec3_dot(v, v);
}

static inline float vec3_length(const float *v) {
    return sqrtf(vec3_length_sq(v));
}

static inline void vec3_normalize(float *out, const float *v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        vec3_mul(out, v, 1.0f / len);
    } else {
        out[0] = 0.0f;
        out[1] = 1.0f;
        out[2] = 0.0f;
    }
}

static inline void begin_profile(const ClothCollisionWorld *world,
                                 const char *label) {
    if (world && world->profiling_enabled && world->profiler.begin) {
        world->profiler.begin(label, world->profiler.user_data);
    }
}

static inline void end_profile(const ClothCollisionWorld *world,
                               const char *label) {
    if (world && world->profiling_enabled && world->profiler.end) {
        world->profiler.end(label, world->profiler.user_data);
    }
}

static ClothCollisionHandle make_handle(uint32_t id) {
    ClothCollisionHandle handle = {id};
    return handle;
}

static ClothCollisionHandle reserve_primitive(ClothCollisionWorld *world) {
    if (!world) {
        return make_handle(CLOTH_COLLISION_INVALID_ID);
    }

    for (uint32_t i = 0; i < world->count; i++) {
        if (!world->primitives[i].enabled) {
            world->primitives[i].enabled = true;
            return make_handle(i);
        }
    }

    if (world->count >= world->capacity) {
        return make_handle(CLOTH_COLLISION_INVALID_ID);
    }

    world->primitives[world->count].enabled = true;
    return make_handle(world->count++);
}

ClothCollisionWorld *cloth_collision_create(const ClothCollisionConfig *config) {
    ClothCollisionConfig cfg = {0};
    if (config) {
        cfg = *config;
    }

    if (cfg.max_primitives == 0) {
        cfg.max_primitives = 256;
    }

    ClothCollisionWorld *world =
        (ClothCollisionWorld *)calloc(1, sizeof(ClothCollisionWorld));
    if (!world) {
        return NULL;
    }

    world->primitives = (ClothCollisionPrimitive *)calloc(
        cfg.max_primitives, sizeof(ClothCollisionPrimitive));
    if (!world->primitives) {
        free(world);
        return NULL;
    }

    world->capacity = cfg.max_primitives;
    world->profiling_enabled = cfg.enable_profiling;
    world->stats.primitive_count = 0;
    world->stats.collision_tests = 0;
    world->stats.collisions = 0;

    return world;
}

void cloth_collision_destroy(ClothCollisionWorld *world) {
    if (!world) {
        return;
    }
    free(world->primitives);
    free(world);
}

ClothCollisionHandle cloth_collision_add_sphere(ClothCollisionWorld *world,
                                                const float *center,
                                                float radius) {
    ClothCollisionHandle handle = reserve_primitive(world);
    if (handle.id == CLOTH_COLLISION_INVALID_ID) {
        return handle;
    }

    ClothCollisionPrimitive *primitive = &world->primitives[handle.id];
    primitive->type = CLOTH_COLLISION_SPHERE;
    primitive->enabled = true;
    vec3_copy(primitive->data.sphere.center, center);
    primitive->data.sphere.radius = radius;
    world->stats.primitive_count++;
    return handle;
}

ClothCollisionHandle cloth_collision_add_plane(ClothCollisionWorld *world,
                                               const float *normal,
                                               float offset) {
    ClothCollisionHandle handle = reserve_primitive(world);
    if (handle.id == CLOTH_COLLISION_INVALID_ID) {
        return handle;
    }

    ClothCollisionPrimitive *primitive = &world->primitives[handle.id];
    primitive->type = CLOTH_COLLISION_PLANE;
    primitive->enabled = true;
    vec3_normalize(primitive->data.plane.normal, normal);
    primitive->data.plane.offset = offset;
    world->stats.primitive_count++;
    return handle;
}

ClothCollisionHandle cloth_collision_add_capsule(ClothCollisionWorld *world,
                                                 const float *point_a,
                                                 const float *point_b,
                                                 float radius) {
    ClothCollisionHandle handle = reserve_primitive(world);
    if (handle.id == CLOTH_COLLISION_INVALID_ID) {
        return handle;
    }

    ClothCollisionPrimitive *primitive = &world->primitives[handle.id];
    primitive->type = CLOTH_COLLISION_CAPSULE;
    primitive->enabled = true;
    vec3_copy(primitive->data.capsule.point_a, point_a);
    vec3_copy(primitive->data.capsule.point_b, point_b);
    primitive->data.capsule.radius = radius;
    world->stats.primitive_count++;
    return handle;
}

ClothCollisionHandle cloth_collision_add_box(ClothCollisionWorld *world,
                                             const float *min_bounds,
                                             const float *max_bounds) {
    ClothCollisionHandle handle = reserve_primitive(world);
    if (handle.id == CLOTH_COLLISION_INVALID_ID) {
        return handle;
    }

    ClothCollisionPrimitive *primitive = &world->primitives[handle.id];
    primitive->type = CLOTH_COLLISION_BOX;
    primitive->enabled = true;
    vec3_copy(primitive->data.box.min, min_bounds);
    vec3_copy(primitive->data.box.max, max_bounds);
    world->stats.primitive_count++;
    return handle;
}

void cloth_collision_remove(ClothCollisionWorld *world,
                            ClothCollisionHandle handle) {
    if (!world || handle.id >= world->count) {
        return;
    }
    if (world->primitives[handle.id].enabled) {
        world->primitives[handle.id].enabled = false;
        if (world->stats.primitive_count > 0) {
            world->stats.primitive_count--;
        }
    }
}

void cloth_collision_clear(ClothCollisionWorld *world) {
    if (!world) {
        return;
    }
    for (uint32_t i = 0; i < world->count; i++) {
        world->primitives[i].enabled = false;
    }
    world->count = 0;
    world->stats.primitive_count = 0;
}

void cloth_collision_set_profiler(ClothCollisionWorld *world,
                                  const ClothCollisionGpuProfiler *profiler) {
    if (!world) {
        return;
    }
    if (profiler) {
        world->profiler = *profiler;
        world->profiling_enabled = true;
    } else {
        memset(&world->profiler, 0, sizeof(world->profiler));
        world->profiling_enabled = false;
    }
}

void cloth_collision_get_stats(const ClothCollisionWorld *world,
                               ClothCollisionStats *out_stats) {
    if (!world || !out_stats) {
        return;
    }
    *out_stats = world->stats;
}

static void apply_collision_response(float *position, float *velocity,
                                     const float *normal, float penetration,
                                     float restitution, float friction) {
    float correction[3];
    vec3_mul(correction, normal, penetration);
    vec3_add(position, position, correction);

    if (!velocity) {
        return;
    }

    float normal_velocity = vec3_dot(velocity, normal);
    if (normal_velocity < 0.0f) {
        float normal_component[3];
        vec3_mul(normal_component, normal, normal_velocity);

        float tangent[3];
        vec3_sub(tangent, velocity, normal_component);
        vec3_mul(normal_component, normal, (1.0f + restitution) * normal_velocity);
        vec3_sub(velocity, velocity, normal_component);

        float tangent_len = vec3_length(tangent);
        if (tangent_len > 0.0f) {
            float friction_scale = fminf(1.0f, friction);
            vec3_mul(tangent, tangent, friction_scale);
            vec3_sub(velocity, velocity, tangent);
        }
    }
}

static bool collide_sphere(const ClothCollisionSphere *sphere,
                           const float *position, float radius,
                           float *out_normal, float *out_penetration) {
    float delta[3];
    vec3_sub(delta, position, sphere->center);
    float dist_sq = vec3_length_sq(delta);
    float combined = sphere->radius + radius;
    float combined_sq = combined * combined;

    if (dist_sq >= combined_sq) {
        return false;
    }

    float dist = sqrtf(fmaxf(dist_sq, 1e-6f));
    float penetration = combined - dist;
    vec3_mul(out_normal, delta, 1.0f / dist);
    *out_penetration = penetration;
    return true;
}

static bool collide_plane(const ClothCollisionPlane *plane,
                          const float *position, float radius,
                          float *out_normal, float *out_penetration) {
    float distance = vec3_dot(position, plane->normal) - plane->offset;
    if (distance >= radius) {
        return false;
    }

    vec3_copy(out_normal, plane->normal);
    *out_penetration = radius - distance;
    return true;
}

static bool collide_capsule(const ClothCollisionCapsule *capsule,
                            const float *position, float radius,
                            float *out_normal, float *out_penetration) {
    float seg[3];
    vec3_sub(seg, capsule->point_b, capsule->point_a);
    float seg_len_sq = vec3_length_sq(seg);

    float t = 0.0f;
    if (seg_len_sq > 1e-6f) {
        float to_pos[3];
        vec3_sub(to_pos, position, capsule->point_a);
        t = vec3_dot(to_pos, seg) / seg_len_sq;
        if (t < 0.0f)
            t = 0.0f;
        else if (t > 1.0f)
            t = 1.0f;
    }

    float closest[3];
    vec3_mul(closest, seg, t);
    vec3_add(closest, closest, capsule->point_a);

    ClothCollisionSphere sphere = {0};
    vec3_copy(sphere.center, closest);
    sphere.radius = capsule->radius;
    return collide_sphere(&sphere, position, radius, out_normal, out_penetration);
}

static bool collide_box(const ClothCollisionBox *box, const float *position,
                        float radius, float *out_normal,
                        float *out_penetration) {
    float closest[3] = {
        fminf(fmaxf(position[0], box->min[0]), box->max[0]),
        fminf(fmaxf(position[1], box->min[1]), box->max[1]),
        fminf(fmaxf(position[2], box->min[2]), box->max[2])};

    float delta[3];
    vec3_sub(delta, position, closest);
    float dist_sq = vec3_length_sq(delta);
    if (dist_sq >= radius * radius) {
        return false;
    }

    float dist = sqrtf(fmaxf(dist_sq, 1e-6f));
    float penetration = radius - dist;
    vec3_mul(out_normal, delta, 1.0f / dist);
    *out_penetration = penetration;
    return true;
}

void cloth_collision_resolve(ClothCollisionWorld *world, float *positions,
                             float *velocities, uint32_t particle_count,
                             float particle_radius, float restitution,
                             float friction) {
    if (!world || !positions || particle_count == 0) {
        return;
    }

    begin_profile(world, "cloth_collision_resolve");

    world->stats.collision_tests = 0;
    world->stats.collisions = 0;

    for (uint32_t i = 0; i < particle_count; i++) {
        float *pos = &positions[i * 3];
        float *vel = velocities ? &velocities[i * 3] : NULL;

        for (uint32_t p = 0; p < world->count; p++) {
            ClothCollisionPrimitive *primitive = &world->primitives[p];
            if (!primitive->enabled) {
                continue;
            }

            world->stats.collision_tests++;

            float normal[3];
            float penetration = 0.0f;
            bool hit = false;

            switch (primitive->type) {
                case CLOTH_COLLISION_SPHERE:
                    hit = collide_sphere(&primitive->data.sphere, pos,
                                         particle_radius, normal, &penetration);
                    break;
                case CLOTH_COLLISION_PLANE:
                    hit = collide_plane(&primitive->data.plane, pos,
                                        particle_radius, normal, &penetration);
                    break;
                case CLOTH_COLLISION_CAPSULE:
                    hit = collide_capsule(&primitive->data.capsule, pos,
                                          particle_radius, normal, &penetration);
                    break;
                case CLOTH_COLLISION_BOX:
                    hit = collide_box(&primitive->data.box, pos,
                                      particle_radius, normal, &penetration);
                    break;
                default:
                    break;
            }

            if (hit) {
                world->stats.collisions++;
                apply_collision_response(pos, vel, normal, penetration,
                                         restitution, friction);
            }
        }
    }

    end_profile(world, "cloth_collision_resolve");
}

