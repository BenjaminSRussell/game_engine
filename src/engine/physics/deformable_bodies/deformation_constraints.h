/**
 * @file deformation_constraints.h
 * @brief Deformation constraints
 *
 * Implements a compact constraint solver for deformable bodies with optional
 * profiling hooks and configurable iteration counts.
 */

#ifndef RENDER_DEFORMATION_CONSTRAINTS_H
#define RENDER_DEFORMATION_CONSTRAINTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Type definitions
// ========================================

typedef struct DeformationConstraintSystem DeformationConstraintSystem;

typedef enum DeformationConstraintType {
    DEFORMATION_CONSTRAINT_DISTANCE = 0,
    DEFORMATION_CONSTRAINT_VOLUME = 1
} DeformationConstraintType;

typedef struct DeformationDistanceConstraint {
    uint32_t index_a;
    uint32_t index_b;
    float rest_length;
    float stiffness;
} DeformationDistanceConstraint;

typedef struct DeformationVolumeConstraint {
    uint32_t indices[4];
    float rest_volume;
    float stiffness;
} DeformationVolumeConstraint;

typedef struct DeformationConstraint {
    DeformationConstraintType type;
    bool enabled;
    union {
        DeformationDistanceConstraint distance;
        DeformationVolumeConstraint volume;
    } data;
} DeformationConstraint;

typedef struct DeformationConstraintStats {
    uint32_t constraint_count;
    uint64_t solve_calls;
} DeformationConstraintStats;

typedef struct DeformationGpuProfiler {
    void (*begin)(const char *label, void *user_data);
    void (*end)(const char *label, void *user_data);
    void *user_data;
} DeformationGpuProfiler;

typedef struct DeformationConstraintConfig {
    uint32_t max_constraints;
    bool enable_profiling;
} DeformationConstraintConfig;

// ========================================
// Function declarations
// ========================================

DeformationConstraintSystem *deformation_constraints_create(
    const DeformationConstraintConfig *config);
void deformation_constraints_destroy(DeformationConstraintSystem *system);

int deformation_constraints_add_distance(DeformationConstraintSystem *system,
                                         uint32_t index_a, uint32_t index_b,
                                         float rest_length, float stiffness);
int deformation_constraints_add_volume(DeformationConstraintSystem *system,
                                       const uint32_t *indices,
                                       float rest_volume, float stiffness);
void deformation_constraints_remove(DeformationConstraintSystem *system,
                                    uint32_t constraint_id);

void deformation_constraints_set_profiler(
    DeformationConstraintSystem *system,
    const DeformationGpuProfiler *profiler);
void deformation_constraints_get_stats(
    const DeformationConstraintSystem *system,
    DeformationConstraintStats *out_stats);

// ========================================
// Main logic
// ========================================

void deformation_constraints_solve(DeformationConstraintSystem *system,
                                   float *positions, const float *inv_masses,
                                   uint32_t particle_count,
                                   uint32_t iterations);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_DEFORMATION_CONSTRAINTS_H */
