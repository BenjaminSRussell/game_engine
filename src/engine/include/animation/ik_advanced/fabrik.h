#ifndef ANIMATION_IK_ADVANCED_FABRIK_H
#define ANIMATION_IK_ADVANCED_FABRIK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FABRIKChain FABRIKChain;

FABRIKChain *fabrik_chain_create(const float *joint_positions, uint32_t bone_count);
void fabrik_chain_destroy(FABRIKChain *chain);
bool fabrik_solve(FABRIKChain *chain, float target_x, float target_y, float target_z);
void fabrik_set_iterations(FABRIKChain *chain, uint32_t iterations);
void fabrik_set_tolerance(FABRIKChain *chain, float tolerance);
void fabrik_set_pole_vector(FABRIKChain *chain, float x, float y, float z);
void fabrik_disable_pole_vector(FABRIKChain *chain);
void fabrik_get_joint_position(FABRIKChain *chain, uint32_t joint_index, float *out);
bool fabrik_reached_target(FABRIKChain *chain);
float fabrik_get_error(FABRIKChain *chain);
bool fabrik_solve_two_bone(float *root, float *mid, float *end, float *target,
                           float *pole, float length_upper, float length_lower);

#ifdef __cplusplus
}
#endif

#endif
