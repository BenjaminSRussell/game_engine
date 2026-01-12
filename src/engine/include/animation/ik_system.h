// Inverse Kinematics system
#ifndef IK_SYSTEM_H
#define IK_SYSTEM_H

#include "include/common.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_IK_CHAIN_LENGTH 32
#define MAX_IK_CHAINS 128

typedef enum {
  IK_SOLVER_TWO_BONE, // Leg/arm IK
  IK_SOLVER_FABR IK,  // Full body IK
  IK_SOLVER_CCD,      // Cyclic coordinate descent
  IK_SOLVER_LIMB      // Specialized limb IK
} IKSolverType;

typedef struct {
  u32 bone_index;
  Vec3 position;
  Quat rotation;
  f32 length;
} IKBone;

typedef struct {
  char name[64];
  IKSolverType solver_type;

  IKBone bones[MAX_IK_CHAIN_LENGTH];
  u32 bone_count;

  Vec3 target_position;
  Quat target_rotation;

  Vec3 pole_vector; // For two-bone IK
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
