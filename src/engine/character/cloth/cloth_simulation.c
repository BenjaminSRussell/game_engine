/*
 * cloth_simulation.c
 * Cloth Physics Simulation (PBD)
 *
 * Part of the Physics/Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implemented features:
 * - Verlet integration, distance/bending constraints, self-collision
 * - World/collider collisions with friction/restitution + CCD
 * - Wind, stiffness control, tearing, pinned particles, solver iterations
 * - Optional SIMD length path, optional OpenMP parallelization
 * - CPU/GPU path split (GPU currently reuses CPU solver)
 * - Basic area preservation and serialization helpers
 */

#include "character/cloth/cloth_simulation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CLOTH_DEFAULT_ITERATIONS 4
#define CLOTH_GRAVITY -9.81f
#define CLOTH_MAX_OBJECTS 64
#define CLOTH_EPSILON 1e-6f
#define CLOTH_DEFAULT_DAMPING 0.01f
#define CLOTH_DEFAULT_SELF_COLLISION_RADIUS 0.05f
#define CLOTH_DEFAULT_TEAR_FACTOR 2.25f
#define CLOTH_DEFAULT_DISTANCE_STIFFNESS 0.9f
#define CLOTH_DEFAULT_BENDING_STIFFNESS 0.6f
#define CLOTH_DEFAULT_COLLISION_STIFFNESS 0.8f
#define CLOTH_DEFAULT_VOLUME_STIFFNESS 0.2f
#define CLOTH_DEFAULT_FRICTION 0.3f
#define CLOTH_DEFAULT_RESTITUTION 0.1f
#define CLOTH_DEFAULT_WIND_STRENGTH 0.0f

#if defined(__GNUC__) || defined(__clang__)
#define CLOTH_UNUSED __attribute__((unused))
#else
#define CLOTH_UNUSED
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_vec3 {
    float x, y, z;
} cloth_vec3_t;

typedef struct cloth_particle {
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float inv_mass;
    bool pinned;
} cloth_particle_t;

typedef struct cloth_distance_constraint {
    uint32_t a;
    uint32_t b;
    float rest_length;
    float stiffness;
    bool tearable;
    bool broken;
} cloth_distance_constraint_t;

typedef struct cloth_triangle {
    uint32_t a, b, c;
    float rest_area;
} cloth_triangle_t;

typedef struct cloth_sphere_collider {
    float x, y, z;
    float radius;
    float friction;
    float restitution;
} cloth_sphere_collider_t;

typedef struct cloth_capsule_collider {
    float ax, ay, az;
    float bx, by, bz;
    float radius;
    float friction;
    float restitution;
} cloth_capsule_collider_t;

typedef struct cloth_object {
    cloth_particle_t* particles;
    uint32_t particle_count;
    cloth_distance_constraint_t* distance_constraints;
    uint32_t distance_constraint_count;
    cloth_distance_constraint_t* bending_constraints;
    uint32_t bending_constraint_count;
    cloth_triangle_t* triangles;
    uint32_t triangle_count;
    cloth_sphere_collider_t* sphere_colliders;
    uint32_t sphere_collider_count;
    cloth_capsule_collider_t* capsule_colliders;
    uint32_t capsule_collider_count;
    float damping;
    float self_collision_radius;
    float tear_factor;
    float distance_stiffness;
    float bending_stiffness;
    float collision_stiffness;
    float volume_stiffness;
    float collision_friction;
    float collision_restitution;
    float wind_strength;
    cloth_vec3_t wind_direction;
    uint32_t solver_iterations;
    bool use_gpu;
    bool active;
} cloth_object_t;

static struct {
    cloth_object_t* clothes;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} g_cloth_sim = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static cloth_vec3_t cloth_vec3_make(float x, float y, float z) {
    cloth_vec3_t v = {x, y, z};
    return v;
}

static cloth_vec3_t cloth_vec3_add(cloth_vec3_t a, cloth_vec3_t b) {
    return cloth_vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

static cloth_vec3_t cloth_vec3_sub(cloth_vec3_t a, cloth_vec3_t b) {
    return cloth_vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

static cloth_vec3_t cloth_vec3_scale(cloth_vec3_t v, float s) {
    return cloth_vec3_make(v.x * s, v.y * s, v.z * s);
}

static float cloth_vec3_dot(cloth_vec3_t a, cloth_vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static cloth_vec3_t cloth_vec3_cross(cloth_vec3_t a, cloth_vec3_t b) {
    return cloth_vec3_make(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static float cloth_vec3_length(cloth_vec3_t v) {
#if defined(__SSE__)
    __m128 vec = _mm_set_ps(0.0f, v.z, v.y, v.x);
    __m128 mul = _mm_mul_ps(vec, vec);
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3));
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(1, 0, 3, 2));
    sums = _mm_add_ps(sums, shuf);
    return sqrtf(_mm_cvtss_f32(sums));
#else
    return sqrtf(cloth_vec3_dot(v, v));
#endif
}

static cloth_vec3_t cloth_vec3_normalize(cloth_vec3_t v) {
    float len = cloth_vec3_length(v);
    if (len < CLOTH_EPSILON) return cloth_vec3_make(0.0f, 0.0f, 0.0f);
    return cloth_vec3_scale(v, 1.0f / len);
}

static cloth_vec3_t cloth_particle_pos(const cloth_particle_t* p) {
    return cloth_vec3_make(p->x, p->y, p->z);
}

static cloth_vec3_t cloth_particle_prev(const cloth_particle_t* p) {
    return cloth_vec3_make(p->prev_x, p->prev_y, p->prev_z);
}

static void cloth_particle_set_pos(cloth_particle_t* p, cloth_vec3_t v) {
    p->x = v.x;
    p->y = v.y;
    p->z = v.z;
}

static void cloth_particle_set_prev(cloth_particle_t* p, cloth_vec3_t v) {
    p->prev_x = v.x;
    p->prev_y = v.y;
    p->prev_z = v.z;
}

/* ============================================================================
 * CONSTRAINT HELPERS
 * ============================================================================ */

static void cloth_apply_collision_response(cloth_particle_t* p, cloth_vec3_t normal, float friction, float restitution) {
    cloth_vec3_t pos = cloth_particle_pos(p);
    cloth_vec3_t prev = cloth_particle_prev(p);
    cloth_vec3_t vel = cloth_vec3_sub(pos, prev);
    float vn = cloth_vec3_dot(vel, normal);
    if (vn > 0.0f) {
        return;
    }
    cloth_vec3_t vt = cloth_vec3_sub(vel, cloth_vec3_scale(normal, vn));
    float vn_out = -vn * restitution;
    cloth_vec3_t vel_out = cloth_vec3_add(cloth_vec3_scale(vt, 1.0f - friction), cloth_vec3_scale(normal, vn_out));
    cloth_particle_set_prev(p, cloth_vec3_sub(pos, vel_out));
}

static bool cloth_segment_sphere_intersect(cloth_vec3_t p0, cloth_vec3_t p1, cloth_vec3_t center, float radius, float* out_t) {
    cloth_vec3_t d = cloth_vec3_sub(p1, p0);
    cloth_vec3_t m = cloth_vec3_sub(p0, center);
    float b = cloth_vec3_dot(m, d);
    float c = cloth_vec3_dot(m, m) - radius * radius;
    if (c <= 0.0f) {
        if (out_t) *out_t = 0.0f;
        return true;
    }
    if (b > 0.0f) {
        return false;
    }
    float denom = cloth_vec3_dot(d, d);
    if (denom < CLOTH_EPSILON) {
        return false;
    }
    float discr = b * b - denom * c;
    if (discr < 0.0f) {
        return false;
    }
    float t = (-b - sqrtf(discr)) / denom;
    if (t < 0.0f || t > 1.0f) {
        return false;
    }
    if (out_t) *out_t = t;
    return true;
}

static void cloth_closest_points_on_segments(cloth_vec3_t p1, cloth_vec3_t q1, cloth_vec3_t p2, cloth_vec3_t q2, float* out_s, float* out_t) {
    cloth_vec3_t d1 = cloth_vec3_sub(q1, p1);
    cloth_vec3_t d2 = cloth_vec3_sub(q2, p2);
    cloth_vec3_t r = cloth_vec3_sub(p1, p2);
    float a = cloth_vec3_dot(d1, d1);
    float e = cloth_vec3_dot(d2, d2);
    float f = cloth_vec3_dot(d2, r);
    float s = 0.0f;
    float t = 0.0f;

    if (a <= CLOTH_EPSILON && e <= CLOTH_EPSILON) {
        s = 0.0f;
        t = 0.0f;
    } else if (a <= CLOTH_EPSILON) {
        s = 0.0f;
        t = f / e;
    } else {
        float c = cloth_vec3_dot(d1, r);
        if (e <= CLOTH_EPSILON) {
            t = 0.0f;
            s = -c / a;
        } else {
            float b = cloth_vec3_dot(d1, d2);
            float denom = a * e - b * b;
            if (denom != 0.0f) {
                s = (b * f - c * e) / denom;
            }
            t = (b * s + f) / e;
        }
    }

    if (s < 0.0f) s = 0.0f;
    else if (s > 1.0f) s = 1.0f;

    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    if (out_s) *out_s = s;
    if (out_t) *out_t = t;
}

/* ============================================================================
 * CLOTH SOLVER
 * ============================================================================ */

static void cloth_object_reset_defaults(cloth_object_t* cloth) {
    if (!cloth) return;
    cloth->particles = NULL;
    cloth->particle_count = 0;
    cloth->distance_constraints = NULL;
    cloth->distance_constraint_count = 0;
    cloth->bending_constraints = NULL;
    cloth->bending_constraint_count = 0;
    cloth->triangles = NULL;
    cloth->triangle_count = 0;
    cloth->sphere_colliders = NULL;
    cloth->sphere_collider_count = 0;
    cloth->capsule_colliders = NULL;
    cloth->capsule_collider_count = 0;
    cloth->damping = CLOTH_DEFAULT_DAMPING;
    cloth->self_collision_radius = CLOTH_DEFAULT_SELF_COLLISION_RADIUS;
    cloth->tear_factor = CLOTH_DEFAULT_TEAR_FACTOR;
    cloth->distance_stiffness = CLOTH_DEFAULT_DISTANCE_STIFFNESS;
    cloth->bending_stiffness = CLOTH_DEFAULT_BENDING_STIFFNESS;
    cloth->collision_stiffness = CLOTH_DEFAULT_COLLISION_STIFFNESS;
    cloth->volume_stiffness = CLOTH_DEFAULT_VOLUME_STIFFNESS;
    cloth->collision_friction = CLOTH_DEFAULT_FRICTION;
    cloth->collision_restitution = CLOTH_DEFAULT_RESTITUTION;
    cloth->wind_strength = CLOTH_DEFAULT_WIND_STRENGTH;
    cloth->wind_direction = cloth_vec3_make(0.0f, 0.0f, 0.0f);
    cloth->solver_iterations = CLOTH_DEFAULT_ITERATIONS;
    cloth->use_gpu = false;
    cloth->active = false;
}

static void cloth_object_release(cloth_object_t* cloth) {
    if (!cloth) return;
    free(cloth->particles);
    free(cloth->distance_constraints);
    free(cloth->bending_constraints);
    free(cloth->triangles);
    free(cloth->sphere_colliders);
    free(cloth->capsule_colliders);
    cloth_object_reset_defaults(cloth);
}

static void cloth_apply_forces(cloth_object_t* cloth, float dt) {
    if (!cloth || cloth->particle_count == 0) return;
    cloth_vec3_t gravity = cloth_vec3_make(0.0f, CLOTH_GRAVITY, 0.0f);
    cloth_vec3_t wind = cloth_vec3_scale(cloth->wind_direction, cloth->wind_strength);
    cloth_vec3_t accel = cloth_vec3_add(gravity, wind);
    float dt2 = dt * dt;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (uint32_t i = 0; i < cloth->particle_count; i++) {
        cloth_particle_t* p = &cloth->particles[i];
        if (p->pinned || p->inv_mass <= 0.0f) {
            cloth_particle_set_prev(p, cloth_particle_pos(p));
            continue;
        }
        cloth_vec3_t pos = cloth_particle_pos(p);
        cloth_vec3_t prev = cloth_particle_prev(p);
        cloth_vec3_t vel = cloth_vec3_sub(pos, prev);
        vel = cloth_vec3_scale(vel, 1.0f - cloth->damping);
        cloth_vec3_t next = cloth_vec3_add(pos, vel);
        next = cloth_vec3_add(next, cloth_vec3_scale(accel, dt2));
        cloth_particle_set_prev(p, pos);
        cloth_particle_set_pos(p, next);
    }
}

static void cloth_solve_distance_constraints(cloth_object_t* cloth, cloth_distance_constraint_t* constraints, uint32_t count, float stiffness, float tear_factor) {
    if (!cloth || !constraints) return;
    float combined_stiffness = stiffness;
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (uint32_t i = 0; i < count; i++) {
        cloth_distance_constraint_t* c = &constraints[i];
        if (c->broken) continue;
        cloth_particle_t* pa = &cloth->particles[c->a];
        cloth_particle_t* pb = &cloth->particles[c->b];
        if ((pa->pinned && pb->pinned) || (pa->inv_mass <= 0.0f && pb->inv_mass <= 0.0f)) {
            continue;
        }
        cloth_vec3_t pos_a = cloth_particle_pos(pa);
        cloth_vec3_t pos_b = cloth_particle_pos(pb);
        cloth_vec3_t delta = cloth_vec3_sub(pos_b, pos_a);
        float dist = cloth_vec3_length(delta);
        if (dist < CLOTH_EPSILON) {
            continue;
        }
        if (c->tearable && dist > c->rest_length * tear_factor) {
            c->broken = true;
            continue;
        }
        float diff = (dist - c->rest_length) / dist;
        float inv_mass_a = pa->pinned ? 0.0f : pa->inv_mass;
        float inv_mass_b = pb->pinned ? 0.0f : pb->inv_mass;
        float inv_mass_sum = inv_mass_a + inv_mass_b;
        if (inv_mass_sum < CLOTH_EPSILON) {
            continue;
        }
        float w_a = inv_mass_a / inv_mass_sum;
        float w_b = inv_mass_b / inv_mass_sum;
        cloth_vec3_t correction = cloth_vec3_scale(delta, diff * combined_stiffness * c->stiffness);
        if (!pa->pinned) {
            cloth_particle_set_pos(pa, cloth_vec3_add(pos_a, cloth_vec3_scale(correction, w_a)));
        }
        if (!pb->pinned) {
            cloth_particle_set_pos(pb, cloth_vec3_sub(pos_b, cloth_vec3_scale(correction, w_b)));
        }
    }
}

static void cloth_solve_self_collision(cloth_object_t* cloth) {
    if (!cloth || cloth->particle_count == 0) return;
    float radius = cloth->self_collision_radius;
    if (radius <= CLOTH_EPSILON) return;
    float radius_sq = radius * radius;

    for (uint32_t i = 0; i < cloth->particle_count; i++) {
        cloth_particle_t* pa = &cloth->particles[i];
        if (pa->pinned) continue;
        cloth_vec3_t pos_a = cloth_particle_pos(pa);
        for (uint32_t j = i + 1; j < cloth->particle_count; j++) {
            cloth_particle_t* pb = &cloth->particles[j];
            if (pb->pinned) continue;
            cloth_vec3_t pos_b = cloth_particle_pos(pb);
            cloth_vec3_t delta = cloth_vec3_sub(pos_b, pos_a);
            float dist_sq = cloth_vec3_dot(delta, delta);
            if (dist_sq >= radius_sq || dist_sq < CLOTH_EPSILON) continue;
            float dist = sqrtf(dist_sq);
            cloth_vec3_t normal = cloth_vec3_scale(delta, 1.0f / dist);
            float penetration = radius - dist;
            float inv_mass_sum = pa->inv_mass + pb->inv_mass;
            if (inv_mass_sum <= CLOTH_EPSILON) continue;
            float w_a = pa->inv_mass / inv_mass_sum;
            float w_b = pb->inv_mass / inv_mass_sum;
            cloth_vec3_t correction = cloth_vec3_scale(normal, penetration * cloth->collision_stiffness);
            cloth_particle_set_pos(pa, cloth_vec3_sub(pos_a, cloth_vec3_scale(correction, w_a)));
            cloth_particle_set_pos(pb, cloth_vec3_add(pos_b, cloth_vec3_scale(correction, w_b)));
            pos_a = cloth_particle_pos(pa);
        }
    }
}

static void cloth_solve_area_constraints(cloth_object_t* cloth) {
    if (!cloth || !cloth->triangles || cloth->triangle_count == 0) return;
    float stiffness = cloth->volume_stiffness;
    for (uint32_t i = 0; i < cloth->triangle_count; i++) {
        cloth_triangle_t* tri = &cloth->triangles[i];
        cloth_particle_t* pa = &cloth->particles[tri->a];
        cloth_particle_t* pb = &cloth->particles[tri->b];
        cloth_particle_t* pc = &cloth->particles[tri->c];
        if (pa->pinned && pb->pinned && pc->pinned) continue;
        cloth_vec3_t a = cloth_particle_pos(pa);
        cloth_vec3_t b = cloth_particle_pos(pb);
        cloth_vec3_t c = cloth_particle_pos(pc);
        cloth_vec3_t ab = cloth_vec3_sub(b, a);
        cloth_vec3_t ac = cloth_vec3_sub(c, a);
        cloth_vec3_t n = cloth_vec3_cross(ab, ac);
        float area = 0.5f * cloth_vec3_length(n);
        if (area < CLOTH_EPSILON) continue;
        float diff = (area - tri->rest_area) / area;
        cloth_vec3_t correction = cloth_vec3_scale(cloth_vec3_normalize(n), diff * stiffness);
        if (!pa->pinned) cloth_particle_set_pos(pa, cloth_vec3_sub(a, correction));
        if (!pb->pinned) cloth_particle_set_pos(pb, cloth_vec3_add(b, correction));
        if (!pc->pinned) cloth_particle_set_pos(pc, cloth_vec3_add(c, correction));
    }
}

static void cloth_resolve_plane_collision(cloth_particle_t* p, float friction, float restitution) {
    cloth_vec3_t pos = cloth_particle_pos(p);
    cloth_vec3_t prev = cloth_particle_prev(p);
    if (pos.y >= 0.0f) return;
    if (prev.y > 0.0f) {
        float t = prev.y / (prev.y - pos.y);
        cloth_vec3_t hit = cloth_vec3_add(prev, cloth_vec3_scale(cloth_vec3_sub(pos, prev), t));
        hit.y = 0.0f;
        cloth_particle_set_pos(p, hit);
    } else {
        pos.y = 0.0f;
        cloth_particle_set_pos(p, pos);
    }
    cloth_apply_collision_response(p, cloth_vec3_make(0.0f, 1.0f, 0.0f), friction, restitution);
}

static void cloth_resolve_sphere_collision(cloth_particle_t* p, const cloth_sphere_collider_t* sphere) {
    cloth_vec3_t pos = cloth_particle_pos(p);
    cloth_vec3_t prev = cloth_particle_prev(p);
    cloth_vec3_t center = cloth_vec3_make(sphere->x, sphere->y, sphere->z);
    float t = 0.0f;
    bool intersect = cloth_segment_sphere_intersect(prev, pos, center, sphere->radius, &t);
    if (intersect) {
        cloth_vec3_t hit = cloth_vec3_add(prev, cloth_vec3_scale(cloth_vec3_sub(pos, prev), t));
        cloth_vec3_t normal = cloth_vec3_normalize(cloth_vec3_sub(hit, center));
        hit = cloth_vec3_add(center, cloth_vec3_scale(normal, sphere->radius));
        cloth_particle_set_pos(p, hit);
        cloth_apply_collision_response(p, normal, sphere->friction, sphere->restitution);
        return;
    }
    cloth_vec3_t delta = cloth_vec3_sub(pos, center);
    float dist = cloth_vec3_length(delta);
    if (dist >= sphere->radius || dist < CLOTH_EPSILON) return;
    cloth_vec3_t normal = cloth_vec3_scale(delta, 1.0f / dist);
    cloth_vec3_t corrected = cloth_vec3_add(center, cloth_vec3_scale(normal, sphere->radius));
    cloth_particle_set_pos(p, corrected);
    cloth_apply_collision_response(p, normal, sphere->friction, sphere->restitution);
}

static void cloth_resolve_capsule_collision(cloth_particle_t* p, const cloth_capsule_collider_t* capsule) {
    cloth_vec3_t prev = cloth_particle_prev(p);
    cloth_vec3_t pos = cloth_particle_pos(p);
    cloth_vec3_t a = cloth_vec3_make(capsule->ax, capsule->ay, capsule->az);
    cloth_vec3_t b = cloth_vec3_make(capsule->bx, capsule->by, capsule->bz);
    float s = 0.0f;
    float t = 0.0f;
    cloth_closest_points_on_segments(prev, pos, a, b, &s, &t);
    cloth_vec3_t move_point = cloth_vec3_add(prev, cloth_vec3_scale(cloth_vec3_sub(pos, prev), s));
    cloth_vec3_t cap_point = cloth_vec3_add(a, cloth_vec3_scale(cloth_vec3_sub(b, a), t));
    cloth_vec3_t delta = cloth_vec3_sub(move_point, cap_point);
    float dist = cloth_vec3_length(delta);
    if (dist >= capsule->radius || dist < CLOTH_EPSILON) return;
    cloth_vec3_t normal = cloth_vec3_scale(delta, 1.0f / dist);
    cloth_vec3_t corrected = cloth_vec3_add(cap_point, cloth_vec3_scale(normal, capsule->radius));
    cloth_particle_set_pos(p, corrected);
    cloth_apply_collision_response(p, normal, capsule->friction, capsule->restitution);
}

static void cloth_resolve_collisions(cloth_object_t* cloth) {
    if (!cloth || cloth->particle_count == 0) return;
    for (uint32_t i = 0; i < cloth->particle_count; i++) {
        cloth_particle_t* p = &cloth->particles[i];
        if (p->pinned) continue;
        cloth_resolve_plane_collision(p, cloth->collision_friction, cloth->collision_restitution);
        for (uint32_t s = 0; s < cloth->sphere_collider_count; s++) {
            cloth_resolve_sphere_collision(p, &cloth->sphere_colliders[s]);
        }
        for (uint32_t c = 0; c < cloth->capsule_collider_count; c++) {
            cloth_resolve_capsule_collision(p, &cloth->capsule_colliders[c]);
        }
    }
}

static void cloth_simulate_cpu(cloth_object_t* cloth, float dt) {
    cloth_apply_forces(cloth, dt);
    uint32_t iterations = cloth->solver_iterations > 0 ? cloth->solver_iterations : CLOTH_DEFAULT_ITERATIONS;
    for (uint32_t iter = 0; iter < iterations; iter++) {
        cloth_solve_distance_constraints(cloth, cloth->distance_constraints, cloth->distance_constraint_count, cloth->distance_stiffness, cloth->tear_factor);
        cloth_solve_distance_constraints(cloth, cloth->bending_constraints, cloth->bending_constraint_count, cloth->bending_stiffness, cloth->tear_factor);
        cloth_solve_self_collision(cloth);
        cloth_solve_area_constraints(cloth);
        cloth_resolve_collisions(cloth);
    }
}

static void cloth_simulate_gpu(cloth_object_t* cloth, float dt) {
    /* GPU compute path is intentionally routed through the CPU solver
       until the backend binding is wired in. */
    cloth_simulate_cpu(cloth, dt);
}

/* ============================================================================
 * SERIALIZATION HELPERS
 * ============================================================================ */

typedef struct cloth_state_header {
    uint32_t particle_count;
} cloth_state_header_t;

static CLOTH_UNUSED size_t cloth_simulation_serialize_state(const cloth_object_t* cloth, void* out, size_t max_size) {
    if (!cloth || !out) return 0;
    size_t required = sizeof(cloth_state_header_t) + cloth->particle_count * sizeof(cloth_particle_t);
    if (max_size < required) return 0;
    cloth_state_header_t header = {cloth->particle_count};
    memcpy(out, &header, sizeof(header));
    memcpy((uint8_t*)out + sizeof(header), cloth->particles, cloth->particle_count * sizeof(cloth_particle_t));
    return required;
}

static CLOTH_UNUSED bool cloth_simulation_deserialize_state(cloth_object_t* cloth, const void* data, size_t size) {
    if (!cloth || !data || size < sizeof(cloth_state_header_t)) return false;
    cloth_state_header_t header;
    memcpy(&header, data, sizeof(header));
    size_t required = sizeof(cloth_state_header_t) + header.particle_count * sizeof(cloth_particle_t);
    if (size < required) return false;
    if (header.particle_count != cloth->particle_count) return false;
    memcpy(cloth->particles, (const uint8_t*)data + sizeof(header), header.particle_count * sizeof(cloth_particle_t));
    return true;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int cloth_simulation_init(void) {
    if (g_cloth_sim.initialized) return 0;
    
    g_cloth_sim.capacity = CLOTH_MAX_OBJECTS;
    g_cloth_sim.clothes = calloc(g_cloth_sim.capacity, sizeof(cloth_object_t));
    if (!g_cloth_sim.clothes) {
        return -1;
    }
    g_cloth_sim.count = 0;
    for (uint32_t i = 0; i < g_cloth_sim.capacity; i++) {
        cloth_object_reset_defaults(&g_cloth_sim.clothes[i]);
    }
    g_cloth_sim.initialized = true;
    return 0;
}

void cloth_simulation_shutdown(void) {
    if (!g_cloth_sim.initialized) return;
    for (uint32_t i = 0; i < g_cloth_sim.capacity; i++) {
        cloth_object_release(&g_cloth_sim.clothes[i]);
    }
    free(g_cloth_sim.clothes);
    g_cloth_sim.clothes = NULL;
    g_cloth_sim.count = 0;
    g_cloth_sim.capacity = 0;
    g_cloth_sim.initialized = false;
}

void cloth_simulation_update(float dt) {
    if (!g_cloth_sim.initialized) return;

    // PBD Steps:
    // 1. Predict positions (vel + gravity)
    // 2. Solve constraints
    // 3. Update velocities
    
    for (uint32_t i = 0; i < g_cloth_sim.count; i++) {
        cloth_object_t* cloth = &g_cloth_sim.clothes[i];
        if (!cloth->active) continue;
        if (cloth->use_gpu) {
            cloth_simulate_gpu(cloth, dt);
        } else {
            cloth_simulate_cpu(cloth, dt);
        }
    }
}
