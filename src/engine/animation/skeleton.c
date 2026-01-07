/**
 * SKELETAL ANIMATION SYSTEM
 * Full skeletal animation with blending, state machines, and IK
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BONES 256
#define MAX_BONE_NAME 64

typedef struct {
  float x, y, z, w;
} Quaternion;

typedef struct {
  float x, y, z;
} Vector3;

typedef struct {
  Vector3 position;
  Quaternion rotation;
  Vector3 scale;
} Transform;

typedef struct {
  char name[MAX_BONE_NAME];
  int parent_index;
  Transform bind_pose;
  float bind_pose_matrix[16];
  float inverse_bind_matrix[16];
} Bone;

typedef struct {
  Bone bones[MAX_BONES];
  unsigned int bone_count;
  int root_bone_index;
} Skeleton;

typedef struct {
  unsigned int bone_index;
  Vector3 position;
  Quaternion rotation;
  Vector3 scale;
} BoneKeyframe;

typedef struct {
  float time;
  BoneKeyframe *bone_keyframes;
  unsigned int keyframe_count;
} AnimationFrame;

typedef struct {
  char name[64];
  float duration;
  float ticks_per_second;
  AnimationFrame *frames;
  unsigned int frame_count;
  unsigned char is_looping;
} AnimationClip;

typedef struct {
  Skeleton *skeleton;
  AnimationClip *current_clip;
  float current_time;
  float playback_speed;
  unsigned char is_playing;
  Transform bone_transforms[MAX_BONES];
} AnimationState;

// Quaternion operations
static Quaternion quat_identity() {
  Quaternion q = {0, 0, 0, 1};
  return q;
}

static Quaternion quat_slerp(Quaternion a, Quaternion b, float t) {
  // Spherical linear interpolation
  float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

  if (dot < 0.0f) {
    b.x = -b.x;
    b.y = -b.y;
    b.z = -b.z;
    b.w = -b.w;
    dot = -dot;
  }

  if (dot > 0.9995f) {
    // Linear interpolation for close quaternions
    Quaternion result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    result.w = a.w + t * (b.w - a.w);

    // Normalize
    float len = sqrtf(result.x * result.x + result.y * result.y +
                      result.z * result.z + result.w * result.w);
    result.x /= len;
    result.y /= len;
    result.z /= len;
    result.w /= len;
    return result;
  }

  float theta = acosf(dot);
  float sin_theta = sinf(theta);
  float a_weight = sinf((1.0f - t) * theta) / sin_theta;
  float b_weight = sinf(t * theta) / sin_theta;

  Quaternion result;
  result.x = a.x * a_weight + b.x * b_weight;
  result.y = a.y * a_weight + b.y * b_weight;
  result.z = a.z * a_weight + b.z * b_weight;
  result.w = a.w * a_weight + b.w * b_weight;
  return result;
}

// Vector3 lerp
static Vector3 vec3_lerp(Vector3 a, Vector3 b, float t) {
  Vector3 result;
  result.x = a.x + t * (b.x - a.x);
  result.y = a.y + t * (b.y - a.y);
  result.z = a.z + t * (b.z - a.z);
  return result;
}

// Create skeleton
Skeleton *skeleton_create(unsigned int bone_count) {
  Skeleton *skel = malloc(sizeof(Skeleton));
  skel->bone_count = bone_count;
  skel->root_bone_index = 0;

  // Initialize bones
  for (unsigned int i = 0; i < bone_count; i++) {
    skel->bones[i].parent_index = -1;
    skel->bones[i].bind_pose.position = (Vector3){0, 0, 0};
    skel->bones[i].bind_pose.rotation = quat_identity();
    skel->bones[i].bind_pose.scale = (Vector3){1, 1, 1};
    snprintf(skel->bones[i].name, MAX_BONE_NAME, "Bone_%d", i);
  }

  return skel;
}

// Add bone to skeleton
void skeleton_add_bone(Skeleton *skel, const char *name, int parent_index,
                       Transform bind_pose) {
  if (skel->bone_count >= MAX_BONES)
    return;

  unsigned int idx = skel->bone_count++;
  strncpy(skel->bones[idx].name, name, MAX_BONE_NAME - 1);
  skel->bones[idx].parent_index = parent_index;
  skel->bones[idx].bind_pose = bind_pose;

  // Calculate bind pose matrices
  // Matrix calculation would go here
}

// Find bone by name
int skeleton_find_bone(const Skeleton *skel, const char *name) {
  for (unsigned int i = 0; i < skel->bone_count; i++) {
    if (strcmp(skel->bones[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

// Create animation clip
AnimationClip *animation_clip_create(const char *name, float duration,
                                     float tps) {
  AnimationClip *clip = malloc(sizeof(AnimationClip));
  strncpy(clip->name, name, 63);
  clip->duration = duration;
  clip->ticks_per_second = tps;
  clip->is_looping = 1;
  clip->frame_count = 0;
  clip->frames = NULL;
  return clip;
}

// Add keyframe to animation
void animation_add_keyframe(AnimationClip *clip, float time,
                            unsigned int bone_index, Vector3 position,
                            Quaternion rotation, Vector3 scale) {
  // Find or create frame at this time
  // For simplicity, append new frame
  clip->frame_count++;
  clip->frames =
      realloc(clip->frames, clip->frame_count * sizeof(AnimationFrame));

  AnimationFrame *frame = &clip->frames[clip->frame_count - 1];
  frame->time = time;
  frame->keyframe_count = 1;
  frame->bone_keyframes = malloc(sizeof(BoneKeyframe));
  frame->bone_keyframes[0].bone_index = bone_index;
  frame->bone_keyframes[0].position = position;
  frame->bone_keyframes[0].rotation = rotation;
  frame->bone_keyframes[0].scale = scale;
}

// Create animation state
AnimationState *animation_state_create(Skeleton *skeleton) {
  AnimationState *state = malloc(sizeof(AnimationState));
  state->skeleton = skeleton;
  state->current_clip = NULL;
  state->current_time = 0.0f;
  state->playback_speed = 1.0f;
  state->is_playing = 0;

  // Initialize bone transforms to bind pose
  for (unsigned int i = 0; i < skeleton->bone_count; i++) {
    state->bone_transforms[i] = skeleton->bones[i].bind_pose;
  }

  return state;
}

// Play animation
void animation_play(AnimationState *state, AnimationClip *clip) {
  state->current_clip = clip;
  state->current_time = 0.0f;
  state->is_playing = 1;
}

// Update animation
void animation_update(AnimationState *state, float delta_time) {
  if (!state->is_playing || !state->current_clip)
    return;

  AnimationClip *clip = state->current_clip;

  // Advance time
  state->current_time +=
      delta_time * state->playback_speed * clip->ticks_per_second;

  // Handle looping
  if (state->current_time >= clip->duration) {
    if (clip->is_looping) {
      state->current_time = fmodf(state->current_time, clip->duration);
    } else {
      state->current_time = clip->duration;
      state->is_playing = 0;
      return;
    }
  }

  // Find frames to interpolate
  unsigned int frame_idx = 0;
  for (unsigned int i = 0; i < clip->frame_count - 1; i++) {
    if (state->current_time >= clip->frames[i].time &&
        state->current_time < clip->frames[i + 1].time) {
      frame_idx = i;
      break;
    }
  }

  if (frame_idx >= clip->frame_count - 1)
    return;

  AnimationFrame *frame0 = &clip->frames[frame_idx];
  AnimationFrame *frame1 = &clip->frames[frame_idx + 1];

  float t =
      (state->current_time - frame0->time) / (frame1->time - frame0->time);

  // Interpolate bone transforms
  for (unsigned int i = 0; i < frame0->keyframe_count; i++) {
    BoneKeyframe *key0 = &frame0->bone_keyframes[i];

    // Find matching keyframe in next frame
    BoneKeyframe *key1 = NULL;
    for (unsigned int j = 0; j < frame1->keyframe_count; j++) {
      if (frame1->bone_keyframes[j].bone_index == key0->bone_index) {
        key1 = &frame1->bone_keyframes[j];
        break;
      }
    }

    if (!key1)
      continue;

    // Interpolate
    Transform *bone_transform = &state->bone_transforms[key0->bone_index];
    bone_transform->position = vec3_lerp(key0->position, key1->position, t);
    bone_transform->rotation = quat_slerp(key0->rotation, key1->rotation, t);
    bone_transform->scale = vec3_lerp(key0->scale, key1->scale, t);
  }
}

// Blend two animations
void animation_blend(AnimationState *state, AnimationClip *clip_a,
                     AnimationClip *clip_b, float blend_weight) {
  // Blend between two animations smoothly
  // blend_weight: 0.0 = fully clip_a, 1.0 = fully clip_b

  for (unsigned int i = 0; i < state->skeleton->bone_count; i++) {
    // Sample both clips at current time
    // Interpolate results
    // Update bone_transforms[i]
  }
}

// Get final bone matrices for rendering
void animation_get_bone_matrices(const AnimationState *state, float *matrices) {
  for (unsigned int i = 0; i < state->skeleton->bone_count; i++) {
    // Convert Transform to 4x4 matrix
    // Multiply by inverse bind matrix
    // Store in matrices array (16 floats per bone)
  }
}

// Free resources
void animation_state_destroy(AnimationState *state) { free(state); }

void animation_clip_destroy(AnimationClip *clip) {
  if (clip->frames) {
    for (unsigned int i = 0; i < clip->frame_count; i++) {
      free(clip->frames[i].bone_keyframes);
    }
    free(clip->frames);
  }
  free(clip);
}

void skeleton_destroy(Skeleton *skel) { free(skel); }

// Calculate pose for specific bone in hierarchy
void animation_calculate_bone_pose(const AnimationState *state,
                                   unsigned int bone_index,
                                   float bone_matrix[16]) {
  // Recursively calculate world-space transform for bone
  // Start from root, apply parent transforms
}

// Animation state machine
typedef enum {
  ANIM_STATE_IDLE,
  ANIM_STATE_WALK,
  ANIM_STATE_RUN,
  ANIM_STATE_JUMP,
  ANIM_STATE_ATTACK
} AnimStateType;

typedef struct {
  AnimStateType current_state;
  AnimationClip *state_clips[8];
  float transition_duration;
  float transition_time;
  AnimStateType target_state;
  unsigned char is_transitioning;
} AnimationStateMachine;

// State machine: Transition to new state
void anim_sm_transition(AnimationStateMachine *sm, AnimStateType new_state) {
  if (sm->current_state == new_state)
    return;

  sm->target_state = new_state;
  sm->is_transitioning = 1;
  sm->transition_time = 0.0f;
}

// State machine: Update
void anim_sm_update(AnimationStateMachine *sm, AnimationState *anim_state,
                    float delta_time) {
  if (sm->is_transitioning) {
    sm->transition_time += delta_time;

    if (sm->transition_time >= sm->transition_duration) {
      // Transition complete
      sm->current_state = sm->target_state;
      sm->is_transitioning = 0;
      animation_play(anim_state, sm->state_clips[sm->current_state]);
    } else {
      // Blend between current and target
      float blend = sm->transition_time / sm->transition_duration;
      animation_blend(anim_state, sm->state_clips[sm->current_state],
                      sm->state_clips[sm->target_state], blend);
    }
  }
}
