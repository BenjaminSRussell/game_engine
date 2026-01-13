/**
 * @file deformation_constraints.c
 * @brief Deformation constraint solver for deformable bodies.
 */

#include "deformation_constraints.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DEFORMATION_INVALID_ID -1

struct DeformationConstraintSystem {
    DeformationConstraint *constraints;
    uint32_t count;
    uint32_t capacity;
    DeformationConstraintStats stats;
    DeformationGpuProfiler profiler;
    bool profiling_enabled;
};

static inline void vec3_sub(float *out, const float *a, const float *b) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline void vec3_add(float *out, const float *a, const float *b) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

static inline void vec3_mul(float *out, const float *v, float s) {
    out[0] = v[0] * s;
    out[1] = v[1] * s;
    out[2] = v[2] * s;
}

static inline float vec3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void vec3_cross(float *out, const float *a, const float *b) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline float vec3_length_sq(const float *v) {
    return vec3_dot(v, v);
}

static inline void begin_profile(const DeformationConstraintSystem *system,
                                 const char *label) {
    if (system && system->profiling_enabled && system->profiler.begin) {
        system->profiler.begin(label, system->profiler.user_data);
    }
}

static inline void end_profile(const DeformationConstraintSystem *system,
                               const char *label) {
    if (system && system->profiling_enabled && system->profiler.end) {
        system->profiler.end(label, system->profiler.user_data);
    }
}

DeformationConstraintSystem *deformation_constraints_create(
    const DeformationConstraintConfig *config) {
    DeformationConstraintConfig cfg = {0};
    if (config) {
        cfg = *config;
    }
    if (cfg.max_constraints == 0) {
        cfg.max_constraints = 4096;
    }

    DeformationConstraintSystem *system =
        (DeformationConstraintSystem *)calloc(
            1, sizeof(DeformationConstraintSystem));
    if (!system) {
        return NULL;
    }

    system->constraints = (DeformationConstraint *)calloc(
        cfg.max_constraints, sizeof(DeformationConstraint));
    if (!system->constraints) {
        free(system);
        return NULL;
    }

    system->capacity = cfg.max_constraints;
    system->profiling_enabled = cfg.enable_profiling;
    system->stats.constraint_count = 0;
    system->stats.solve_calls = 0;

    return system;
}

void deformation_constraints_destroy(DeformationConstraintSystem *system) {
    if (!system) {
        return;
    }
    free(system->constraints);
    free(system);
}

int deformation_constraints_add_distance(DeformationConstraintSystem *system,
                                         uint32_t index_a, uint32_t index_b,
                                         float rest_length, float stiffness) {
    if (!system || system->count >= system->capacity) {
        return DEFORMATION_INVALID_ID;
    }

    uint32_t id = system->count++;
    DeformationConstraint *constraint = &system->constraints[id];
    constraint->type = DEFORMATION_CONSTRAINT_DISTANCE;
    constraint->enabled = true;
    constraint->data.distance.index_a = index_a;
    constraint->data.distance.index_b = index_b;
    constraint->data.distance.rest_length = rest_length;
    constraint->data.distance.stiffness = stiffness;

    system->stats.constraint_count++;
    return (int)id;
}

int deformation_constraints_add_volume(DeformationConstraintSystem *system,
                                       const uint32_t *indices,
                                       float rest_volume, float stiffness) {
    if (!system || !indices || system->count >= system->capacity) {
        return DEFORMATION_INVALID_ID;
    }

    uint32_t id = system->count++;
    DeformationConstraint *constraint = &system->constraints[id];
    constraint->type = DEFORMATION_CONSTRAINT_VOLUME;
    constraint->enabled = true;
    memcpy(constraint->data.volume.indices, indices,
           sizeof(constraint->data.volume.indices));
    constraint->data.volume.rest_volume = rest_volume;
    constraint->data.volume.stiffness = stiffness;

    system->stats.constraint_count++;
    return (int)id;
}

void deformation_constraints_remove(DeformationConstraintSystem *system,
                                    uint32_t constraint_id) {
    if (!system || constraint_id >= system->count) {
        return;
    }

    if (system->constraints[constraint_id].enabled) {
        system->constraints[constraint_id].enabled = false;
        if (system->stats.constraint_count > 0) {
            system->stats.constraint_count--;
        }
    }
}

void deformation_constraints_set_profiler(
    DeformationConstraintSystem *system,
    const DeformationGpuProfiler *profiler) {
    if (!system) {
        return;
    }

    if (profiler) {
        system->profiler = *profiler;
        system->profiling_enabled = true;
    } else {
        memset(&system->profiler, 0, sizeof(system->profiler));
        system->profiling_enabled = false;
    }
}

void deformation_constraints_get_stats(
    const DeformationConstraintSystem *system,
    DeformationConstraintStats *out_stats) {
    if (!system || !out_stats) {
        return;
    }
    *out_stats = system->stats;
}

static void solve_distance_constraint(DeformationDistanceConstraint *constraint,
                                      float *positions, const float *inv_masses,
                                      uint32_t particle_count) {
    if (constraint->index_a >= particle_count ||
        constraint->index_b >= particle_count) {
        return;
    }

    float *p0 = &positions[constraint->index_a * 3];
    float *p1 = &positions[constraint->index_b * 3];

    float delta[3];
    vec3_sub(delta, p1, p0);
    float dist_sq = vec3_length_sq(delta);
    if (dist_sq < 1e-8f) {
        return;
    }

    float dist = sqrtf(dist_sq);
    float rest_length = constraint->rest_length;
    float diff = (dist - rest_length) / dist;
    float stiffness = constraint->stiffness;

    float w0 = inv_masses ? inv_masses[constraint->index_a] : 1.0f;
    float w1 = inv_masses ? inv_masses[constraint->index_b] : 1.0f;
    float w_sum = w0 + w1;
    if (w_sum == 0.0f) {
        return;
    }

    float correction[3];
    vec3_mul(correction, delta, diff * stiffness);
    float c0[3];
    float c1[3];
    vec3_mul(c0, correction, w0 / w_sum);
    vec3_mul(c1, correction, -w1 / w_sum);
    vec3_add(p0, p0, c0);
    vec3_add(p1, p1, c1);
}

static void solve_volume_constraint(DeformationVolumeConstraint *constraint,
                                    float *positions, const float *inv_masses,
                                    uint32_t particle_count) {
    const uint32_t *id = constraint->indices;
    if (id[0] >= particle_count || id[1] >= particle_count ||
        id[2] >= particle_count || id[3] >= particle_count) {
        return;
    }

    float *p0 = &positions[id[0] * 3];
    float *p1 = &positions[id[1] * 3];
    float *p2 = &positions[id[2] * 3];
    float *p3 = &positions[id[3] * 3];

    float p1p0[3], p2p0[3], p3p0[3];
    vec3_sub(p1p0, p1, p0);
    vec3_sub(p2p0, p2, p0);
    vec3_sub(p3p0, p3, p0);

    float cross_p2p0_p3p0[3];
    vec3_cross(cross_p2p0_p3p0, p2p0, p3p0);
    float current_volume = vec3_dot(p1p0, cross_p2p0_p3p0) / 6.0f;

    float c = current_volume - constraint->rest_volume;
    if (fabsf(c) < 1e-6f) {
        return;
    }

    float grad0[3], grad1[3], grad2[3], grad3[3];
    float p1p2[3], p2p3[3], p3p1[3];
    vec3_sub(p1p2, p1, p2);
    vec3_sub(p2p3, p2, p3);
    vec3_sub(p3p1, p3, p1);

    vec3_cross(grad0, p1p2, p2p3);
    vec3_cross(grad1, p2p3, p3p1);
    vec3_cross(grad2, p3p1, p1p2);
    vec3_cross(grad3, p1p0, p2p0);

    float w0 = inv_masses ? inv_masses[id[0]] : 1.0f;
    float w1 = inv_masses ? inv_masses[id[1]] : 1.0f;
    float w2 = inv_masses ? inv_masses[id[2]] : 1.0f;
    float w3 = inv_masses ? inv_masses[id[3]] : 1.0f;

    float denom = w0 * vec3_length_sq(grad0) + w1 * vec3_length_sq(grad1) +
                  w2 * vec3_length_sq(grad2) + w3 * vec3_length_sq(grad3);
    if (denom == 0.0f) {
        return;
    }

    float stiffness = constraint->stiffness;
    float s = (c / denom) * stiffness;

    float corr[3];
    vec3_mul(corr, grad0, -w0 * s);
    vec3_add(p0, p0, corr);
    vec3_mul(corr, grad1, -w1 * s);
    vec3_add(p1, p1, corr);
    vec3_mul(corr, grad2, -w2 * s);
    vec3_add(p2, p2, corr);
    vec3_mul(corr, grad3, -w3 * s);
    vec3_add(p3, p3, corr);
}

void deformation_constraints_solve(DeformationConstraintSystem *system,
                                   float *positions, const float *inv_masses,
                                   uint32_t particle_count,
                                   uint32_t iterations) {
    if (!system || !positions || particle_count == 0) {
        return;
    }

    begin_profile(system, "deformation_constraints_solve");
    system->stats.solve_calls++;

    uint32_t iters = iterations > 0 ? iterations : 1;
    for (uint32_t iter = 0; iter < iters; iter++) {
        for (uint32_t i = 0; i < system->count; i++) {
            DeformationConstraint *constraint = &system->constraints[i];
            if (!constraint->enabled) {
                continue;
            }

            switch (constraint->type) {
                case DEFORMATION_CONSTRAINT_DISTANCE:
                    solve_distance_constraint(&constraint->data.distance,
                                              positions, inv_masses,
                                              particle_count);
                    break;
                case DEFORMATION_CONSTRAINT_VOLUME:
                    solve_volume_constraint(&constraint->data.volume, positions,
                                            inv_masses, particle_count);
                    break;
                default:
                    break;
            }
        }
    }

    end_profile(system, "deformation_constraints_solve");
}

