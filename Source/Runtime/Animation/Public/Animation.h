#ifndef ULTIMATE_ENGINE_ANIMATION_H
#define ULTIMATE_ENGINE_ANIMATION_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct SkeletonManager SkeletonManager;
typedef struct AnimationClipManager AnimationClipManager;
typedef struct AnimationBlender AnimationBlender;
typedef struct IKSolver IKSolver;

typedef struct AnimationSystem {
  SkeletonManager *skeleton_manager;
  AnimationClipManager *clip_manager;
  AnimationBlender *blender;
  IKSolver *ik_solver;
} AnimationSystem;

// Lifecycle
bool Animation_Init(void);
void Animation_Shutdown(void);
void Animation_Update(float delta_time);

// Subsystems
SkeletonManager *SkeletonManager_Create(void);
void SkeletonManager_Destroy(SkeletonManager *mgr);
void SkeletonManager_Update(SkeletonManager *mgr, float dt);

AnimationClipManager *AnimationClipManager_Create(void);
void AnimationClipManager_Destroy(AnimationClipManager *mgr);
void AnimationClipManager_Update(AnimationClipManager *mgr, float dt);

AnimationBlender *AnimationBlender_Create(void);
void AnimationBlender_Destroy(AnimationBlender *blender);
void AnimationBlender_Update(AnimationBlender *blender, float dt);

IKSolver *IKSolver_Create(void);
void IKSolver_Destroy(IKSolver *solver);
void IKSolver_Update(IKSolver *solver, float dt);

#endif // ULTIMATE_ENGINE_ANIMATION_H
