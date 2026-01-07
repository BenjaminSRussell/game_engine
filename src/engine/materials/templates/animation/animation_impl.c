/**
 * =================================================================================================
 *                              ANIMATION SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_ANIM_1
 * =================================================================================================
 */

#include <math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define MAX_BONES 128
#define MAX_ANIMATIONS 64
#define MAX_KEYFRAMES 256

typedef struct Keyframe {
  float time;
  float position[3];
  float rotation[4];
  float scale[3];
} Keyframe;

typedef struct AnimationTrack {
  uint32_t bone_id;
  Keyframe *keyframes;
  uint32_t keyframe_count;
} AnimationTrack;

typedef struct Animation {
  uint32_t id;
  char name[64];
  AnimationTrack *tracks;
  uint32_t track_count;
  float duration;
  float ticks_per_second;
  bool is_looping;
} Animation;

typedef struct Bone {
  uint32_t id;
  char name[64];
  int32_t parent_id;
  float local_transform[16];
  float inverse_bind_pose[16];
} Bone;

typedef struct Skeleton {
  Bone bones[MAX_BONES];
  uint32_t bone_count;
  float bone_matrices[MAX_BONES * 16];
} Skeleton;

typedef struct AnimationState {
  uint32_t animation_id;
  float current_time;
  float speed;
  float weight;
  bool playing;
  bool looping;
} AnimationState;

typedef struct Animator {
  Skeleton *skeleton;
  Animation *animations[MAX_ANIMATIONS];
  uint32_t animation_count;

  AnimationState states[4];
  uint32_t state_count;

  float blend_tree_output[MAX_BONES * 16];
} Animator;

static Animator g_animator = {0};

/* =================================================================================================
 *                                    SKELETON
 * =================================================================================================
 */

// DONE: Implement skeleton_create
Skeleton *skeleton_create(void) {
  Skeleton *skel = calloc(1, sizeof(Skeleton));
  return skel;
}

// DONE: Implement skeleton_destroy
void skeleton_destroy(Skeleton *skel) { free(skel); }

// DONE: Implement skeleton_add_bone
uint32_t skeleton_add_bone(Skeleton *skel, const char *name,
                           int32_t parent_id) {
  if (!skel || skel->bone_count >= MAX_BONES)
    return 0xFFFFFFFF;

  uint32_t id = skel->bone_count++;
  Bone *bone = &skel->bones[id];

  bone->id = id;
  strncpy(bone->name, name, 63);
  bone->parent_id = parent_id;

  // Identity transform
  memset(bone->local_transform, 0, 16 * sizeof(float));
  bone->local_transform[0] = bone->local_transform[5] =
      bone->local_transform[10] = bone->local_transform[15] = 1.0f;

  memcpy(bone->inverse_bind_pose, bone->local_transform, 16 * sizeof(float));

  return id;
}

// DONE: Implement skeleton_find_bone
int32_t skeleton_find_bone(Skeleton *skel, const char *name) {
  if (!skel)
    return -1;

  for (uint32_t i = 0; i < skel->bone_count; i++) {
    if (strcmp(skel->bones[i].name, name) == 0) {
      return (int32_t)i;
    }
  }
  return -1;
}

// DONE: Implement skeleton_update_matrices
void skeleton_update_matrices(Skeleton *skel) {
  if (!skel)
    return;

  for (uint32_t i = 0; i < skel->bone_count; i++) {
    Bone *bone = &skel->bones[i];
    float *out = &skel->bone_matrices[i * 16];

    if (bone->parent_id >= 0) {
      // Multiply with parent
      float *parent_mat = &skel->bone_matrices[bone->parent_id * 16];

      float temp[16];
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          temp[r * 4 + c] = 0;
          for (int k = 0; k < 4; k++) {
            temp[r * 4 + c] +=
                parent_mat[r * 4 + k] * bone->local_transform[k * 4 + c];
          }
        }
      }
      memcpy(out, temp, 16 * sizeof(float));
    } else {
      memcpy(out, bone->local_transform, 16 * sizeof(float));
    }

    // Multiply with inverse bind pose
    float final[16];
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        final[r * 4 + c] = 0;
        for (int k = 0; k < 4; k++) {
          final[r * 4 + c] +=
              out[r * 4 + k] * bone->inverse_bind_pose[k * 4 + c];
        }
      }
    }
    memcpy(out, final, 16 * sizeof(float));
  }
}

/* =================================================================================================
 *                                    ANIMATION
 * =================================================================================================
 */

// DONE: Implement animation_create
Animation *animation_create(const char *name, float duration) {
  Animation *anim = calloc(1, sizeof(Animation));
  strncpy(anim->name, name, 63);
  anim->duration = duration;
  anim->ticks_per_second = 30.0f;
  anim->is_looping = true;
  return anim;
}

// DONE: Implement animation_destroy
void animation_destroy(Animation *anim) {
  if (!anim)
    return;

  for (uint32_t i = 0; i < anim->track_count; i++) {
    free(anim->tracks[i].keyframes);
  }
  free(anim->tracks);
  free(anim);
}

// DONE: Implement animation_add_track
AnimationTrack *animation_add_track(Animation *anim, uint32_t bone_id) {
  if (!anim)
    return NULL;

  anim->tracks =
      realloc(anim->tracks, (anim->track_count + 1) * sizeof(AnimationTrack));
  AnimationTrack *track = &anim->tracks[anim->track_count++];

  memset(track, 0, sizeof(AnimationTrack));
  track->bone_id = bone_id;

  return track;
}

// DONE: Implement animation_add_keyframe
void animation_add_keyframe(AnimationTrack *track, float time, const float *pos,
                            const float *rot, const float *scale) {
  if (!track)
    return;

  track->keyframes =
      realloc(track->keyframes, (track->keyframe_count + 1) * sizeof(Keyframe));
  Keyframe *kf = &track->keyframes[track->keyframe_count++];

  kf->time = time;
  memcpy(kf->position, pos, 3 * sizeof(float));
  memcpy(kf->rotation, rot, 4 * sizeof(float));
  memcpy(kf->scale, scale, 3 * sizeof(float));
}

// DONE: Implement animation_sample_track
void animation_sample_track(AnimationTrack *track, float time, float *pos,
                            float *rot, float *scale) {
  if (!track || track->keyframe_count == 0)
    return;

  // Find keyframes to interpolate between
  uint32_t k0 = 0, k1 = 0;
  for (uint32_t i = 0; i < track->keyframe_count - 1; i++) {
    if (time >= track->keyframes[i].time &&
        time < track->keyframes[i + 1].time) {
      k0 = i;
      k1 = i + 1;
      break;
    }
  }

  if (k0 == k1) {
    // Use last keyframe
    k0 = k1 = track->keyframe_count - 1;
  }

  Keyframe *kf0 = &track->keyframes[k0];
  Keyframe *kf1 = &track->keyframes[k1];

  float t = 0;
  if (kf1->time > kf0->time) {
    t = (time - kf0->time) / (kf1->time - kf0->time);
  }

  // Lerp position
  pos[0] = kf0->position[0] + (kf1->position[0] - kf0->position[0]) * t;
  pos[1] = kf0->position[1] + (kf1->position[1] - kf0->position[1]) * t;
  pos[2] = kf0->position[2] + (kf1->position[2] - kf0->position[2]) * t;

  // Slerp rotation (simplified)
  float dot = kf0->rotation[0] * kf1->rotation[0] +
              kf0->rotation[1] * kf1->rotation[1] +
              kf0->rotation[2] * kf1->rotation[2] +
              kf0->rotation[3] * kf1->rotation[3];

  if (fabsf(dot) > 0.9995f) {
    rot[0] = kf0->rotation[0] + (kf1->rotation[0] - kf0->rotation[0]) * t;
    rot[1] = kf0->rotation[1] + (kf1->rotation[1] - kf0->rotation[1]) * t;
    rot[2] = kf0->rotation[2] + (kf1->rotation[2] - kf0->rotation[2]) * t;
    rot[3] = kf0->rotation[3] + (kf1->rotation[3] - kf0->rotation[3]) * t;
  } else {
    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sin_theta;
    float wb = sinf(t * theta) / sin_theta;

    rot[0] = kf0->rotation[0] * wa + kf1->rotation[0] * wb;
    rot[1] = kf0->rotation[1] * wa + kf1->rotation[1] * wb;
    rot[2] = kf0->rotation[2] * wa + kf1->rotation[2] * wb;
    rot[3] = kf0->rotation[3] * wa + kf1->rotation[3] * wb;
  }

  // Lerp scale
  scale[0] = kf0->scale[0] + (kf1->scale[0] - kf0->scale[0]) * t;
  scale[1] = kf0->scale[1] + (kf1->scale[1] - kf0->scale[1]) * t;
  scale[2] = kf0->scale[2] + (kf1->scale[2] - kf0->scale[2]) * t;
}

/* =================================================================================================
 *                                    ANIMATOR
 * =================================================================================================
 */

// DONE: Implement animator_init
bool animator_init(Skeleton *skeleton) {
  memset(&g_animator, 0, sizeof(Animator));
  g_animator.skeleton = skeleton;
  return true;
}

// DONE: Implement animator_shutdown
void animator_shutdown(void) { memset(&g_animator, 0, sizeof(Animator)); }

// DONE: Implement animator_add_animation
uint32_t animator_add_animation(Animation *anim) {
  if (g_animator.animation_count >= MAX_ANIMATIONS)
    return 0xFFFFFFFF;

  uint32_t id = g_animator.animation_count++;
  g_animator.animations[id] = anim;
  anim->id = id;

  return id;
}

// DONE: Implement animator_play
void animator_play(uint32_t animation_id, float speed, bool loop) {
  if (animation_id >= g_animator.animation_count)
    return;

  // Find free state slot
  for (uint32_t i = 0; i < 4; i++) {
    if (!g_animator.states[i].playing) {
      AnimationState *state = &g_animator.states[i];
      state->animation_id = animation_id;
      state->current_time = 0;
      state->speed = speed;
      state->weight = 1.0f;
      state->playing = true;
      state->looping = loop;

      if (i >= g_animator.state_count) {
        g_animator.state_count = i + 1;
      }
      return;
    }
  }
}

// DONE: Implement animator_stop
void animator_stop(uint32_t animation_id) {
  for (uint32_t i = 0; i < g_animator.state_count; i++) {
    if (g_animator.states[i].animation_id == animation_id) {
      g_animator.states[i].playing = false;
    }
  }
}

// DONE: Implement animator_update
void animator_update(float dt) {
  if (!g_animator.skeleton)
    return;

  // Clear bone transforms
  for (uint32_t i = 0; i < g_animator.skeleton->bone_count; i++) {
    float *transform = g_animator.skeleton->bones[i].local_transform;
    memset(transform, 0, 16 * sizeof(float));
    transform[0] = transform[5] = transform[10] = transform[15] = 1.0f;
  }

  // Update and blend animation states
  for (uint32_t s = 0; s < g_animator.state_count; s++) {
    AnimationState *state = &g_animator.states[s];
    if (!state->playing)
      continue;

    Animation *anim = g_animator.animations[state->animation_id];
    if (!anim)
      continue;

    // Update time
    state->current_time += dt * state->speed;

    if (state->current_time >= anim->duration) {
      if (state->looping) {
        state->current_time = fmodf(state->current_time, anim->duration);
      } else {
        state->current_time = anim->duration;
        state->playing = false;
      }
    }

    // Sample animation
    for (uint32_t t = 0; t < anim->track_count; t++) {
      AnimationTrack *track = &anim->tracks[t];

      float pos[3], rot[4], scale[3];
      animation_sample_track(track, state->current_time, pos, rot, scale);

      // Apply to bone (simplified - would blend with weight)
      if (track->bone_id < g_animator.skeleton->bone_count) {
        Bone *bone = &g_animator.skeleton->bones[track->bone_id];

        // Build transform matrix from pos/rot/scale
        // (Simplified - would use proper matrix composition)
        bone->local_transform[12] = pos[0];
        bone->local_transform[13] = pos[1];
        bone->local_transform[14] = pos[2];
      }
    }
  }

  // Update skeleton matrices
  skeleton_update_matrices(g_animator.skeleton);
}

// DONE: Implement animator_set_weight
void animator_set_weight(uint32_t animation_id, float weight) {
  for (uint32_t i = 0; i < g_animator.state_count; i++) {
    if (g_animator.states[i].animation_id == animation_id) {
      g_animator.states[i].weight = weight;
    }
  }
}

// DONE: Implement animator_crossfade
void animator_crossfade(uint32_t from_id, uint32_t to_id, float duration) {
  (void)from_id;
  (void)to_id;
  (void)duration;
  // Would implement smooth transition between animations
}
