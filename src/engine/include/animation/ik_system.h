// Inverse Kinematics system
#ifndef IK_SYSTEM_H
#define IK_SYSTEM_H

#include "math/vec3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "include/common.h"

#define MAX_IK_CHAIN_LENGTH 32
#define MAX_IK_CHAINS 128

typedef enum {
  IK_SOLVER_TWO_BONE, // Leg/arm IK
  IK_SOLVER_FABRIK,   // Full body IK
  IK_SOLVER_CCD,      // Cyclic coordinate descent
  IK_SOLVER_LIMB      // Specialized limb IK
} IKSolverType;

typedef struct {
  u32 bone_index;
  Vec3 position;
  Quat rotation;
  f32 length;

  // Constraints
  bool constraints_enabled;
  Vec3 min_angles; // Euler angles in radians (pitch, yaw, roll)
  Vec3 max_angles; // Euler angles in radians (pitch, yaw, roll)
  f32 stiffness;   // Resistance to movement (0.0 - 1.0)
} IKBone;

typedef struct {
  char name[64];
  IKSolverType solver_type;

  IKBone bones[MAX_IK_CHAIN_LENGTH];
  u32 bone_count;

  Vec3 target_position;
  Quat target_rotation;

  Vec3 pole_vector;         // For two-bone IK and CCD
  bool pole_vector_enabled; // Whether to use the pole vector
  f32 blend_weight;

  bool position_enabled;
  bool rotation_enabled;

  u32 max_iterations;
  f32 precision;

} IKChain;

typedef struct {
  IKChain chains[MAX_IK_CHAINS];
  u32 chain_count;

} IKSystem;

#ifdef __cplusplus
extern "C" {
#endif

IKSystem *ik_system_create(void);
void ik_system_destroy(IKSystem *system);

u32 ik_add_chain(IKSystem *system, const char *name, IKSolverType solver);
void ik_set_target(IKSystem *system, u32 chain_id, Vec3 position,
                   Quat rotation);
void ik_solve(IKSystem *system, u32 chain_id);

// Solvers
void ik_solve_two_bone(IKChain *chain);
void ik_solve_fabrik(IKChain *chain);
void ik_solve_ccd(IKChain *chain);

#ifdef __cplusplus
}
#endif

#endif
