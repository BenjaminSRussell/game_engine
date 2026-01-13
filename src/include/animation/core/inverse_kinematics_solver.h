/**
 * @file inverse_kinematics_solver.h
 * @brief Inverse Kinematics (IK) solvers - FABRIK and Two-Bone.
 */
#ifndef ANIMATION_CORE_INVERSE_KINEMATICS_SOLVER_H
#define ANIMATION_CORE_INVERSE_KINEMATICS_SOLVER_H

#include <math/vec3.h>
#include <math/quat.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Analytic Two-Bone IK Solver.
 * Perfect for legs and arms (Thigh -> Shin -> Foot).
 * 
 * @param root_pos  Start of the chain (Hip/Shoulder)
 * @param end_pos   Target position (Foot/Hand)
 * @param joint_pos Output mid-joint position (Knee/Elbow)
 * @param length_a  Length of upper bone
 * @param length_b  Length of lower bone
 * @param hint_dir  Direction to bend the joint (Pole Vector)
 */
void ik_solve_two_bone(Vec3 root_pos, Vec3 end_pos, Vec3 *joint_pos,
                       float length_a, float length_b, Vec3 hint_dir);

/**
 * FABRIK Solver for N-link chains.
 * 
 * @param joints     Array of joint positions
 * @param count      Number of joints
 * @param lengths    Array of bone lengths (count - 1 elements)
 * @param target     Target position for end effector
 * @param iterations Number of solver iterations
 */
void ik_solve_fabrik(Vec3 *joints, int count, float *lengths, Vec3 target,
                     int iterations);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_CORE_INVERSE_KINEMATICS_SOLVER_H
