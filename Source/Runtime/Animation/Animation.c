#include "Public/Animation.h"
#include <stdlib.h>

// Struct Stubs
struct SkeletonManager {
  int id;
};
struct AnimationClipManager {
  int id;
};
struct AnimationBlender {
  int id;
};
struct IKSolver {
  int id;
};

static AnimationSystem g_animation_system = {0};

// Implementation Stubs
SkeletonManager *SkeletonManager_Create(void) {
  return malloc(sizeof(SkeletonManager));
}
void SkeletonManager_Destroy(SkeletonManager *mgr) { free(mgr); }
void SkeletonManager_Update(SkeletonManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

AnimationClipManager *AnimationClipManager_Create(void) {
  return malloc(sizeof(AnimationClipManager));
}
void AnimationClipManager_Destroy(AnimationClipManager *mgr) { free(mgr); }
void AnimationClipManager_Update(AnimationClipManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

AnimationBlender *AnimationBlender_Create(void) {
  return malloc(sizeof(AnimationBlender));
}
void AnimationBlender_Destroy(AnimationBlender *blender) { free(blender); }
void AnimationBlender_Update(AnimationBlender *blender, float dt) {
  (void)blender;
  (void)dt;
}

IKSolver *IKSolver_Create(void) { return malloc(sizeof(IKSolver)); }
void IKSolver_Destroy(IKSolver *solver) { free(solver); }
void IKSolver_Update(IKSolver *solver, float dt) {
  (void)solver;
  (void)dt;
}

bool Animation_Init(void) {
  g_animation_system.skeleton_manager = SkeletonManager_Create();
  g_animation_system.clip_manager = AnimationClipManager_Create();
  g_animation_system.blender = AnimationBlender_Create();
  g_animation_system.ik_solver = IKSolver_Create();

  return true;
}

void Animation_Update(float delta_time) {
  // Update animation clips
  AnimationClipManager_Update(g_animation_system.clip_manager, delta_time);

  // Blend animations
  AnimationBlender_Update(g_animation_system.blender, delta_time);

  // Solve IK
  IKSolver_Update(g_animation_system.ik_solver, delta_time);

  // Update skeletons
  SkeletonManager_Update(g_animation_system.skeleton_manager, delta_time);
}

void Animation_Shutdown(void) {
  SkeletonManager_Destroy(g_animation_system.skeleton_manager);
  IKSolver_Destroy(g_animation_system.ik_solver);
  AnimationBlender_Destroy(g_animation_system.blender);
  AnimationClipManager_Destroy(g_animation_system.clip_manager);
}
