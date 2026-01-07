// include/engine/ml/inverse_kinematics.h
//
// Purpose: ML-based inverse kinematics system for character animation
// Uses neural networks to solve complex IK problems with realistic joint movements
//

#ifndef INVERSE_KINEMATICS_H
#define INVERSE_KINEMATICS_H

#include "include/common.h"
#include "include/ai/ml/ml_core.h"
#include "include/math/vec3.h"
#include <stdbool.h>

typedef enum {
    IK_MODEL_FABRIK = 0,
    IK_MODEL_NEURAL_SOLVER,
    IK_MODEL_CONSTRAINT_BASED,
    IK_MODEL_LEARNING_IK,
    IK_MODEL_CUSTOM
} IKModel;

typedef struct {
    Vec3 joint_positions[16];
    Vec3 joint_rotations[16];
    Vec3 target_position;
    f32 bone_lengths[15];
    u32 joint_count;
    f32 tolerance;
    u32 max_iterations;
} IKChain;

typedef struct {
    MLSystem *ml_system;
    void *ik_model;
    IKModel model_type;
    IKChain *chains;
    u32 chain_count;
    f32 learning_rate;
    bool is_training;
    bool initialized;
} MLIKSystem;

MLIKSystem *ml_ik_create(MLSystem *ml_system);
void ml_ik_destroy(MLIKSystem *system);
bool ml_ik_initialize(MLIKSystem *system, IKModel model);
bool ml_ik_solve_chain(MLIKSystem *system, IKChain *chain, Vec3 target);
bool ml_ik_solve_multiple(MLIKSystem *system, IKChain *chains, u32 chain_count);
void ml_ik_set_tolerance(MLIKSystem *system, f32 tolerance);

#endif // INVERSE_KINEMATICS_H
