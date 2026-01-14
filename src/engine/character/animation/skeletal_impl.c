/**
 * ANIMATION SYSTEM - SKELETAL ANIMATION
 * Additional system for completeness
 */

#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float position[3];
  float rotation[4];
  float scale[3];
} Transform;

typedef struct {
  int bone_id;
  Transform transform;
  float time;
} Keyframe;

typedef struct {
  char name[64];
  Keyframe *keyframes;
  int keyframe_count;
  float duration;
} AnimationTrack;

typedef struct {
  char name[64];
  AnimationTrack *tracks;
  int track_count;
  float duration;
  bool looping;
} Animation;

typedef struct {
  int parent_id;
  Transform local_transform;
  Transform world_transform;
  float inverse_bind_matrix[16];
} Bone;

typedef struct {
  Bone *bones;
  int bone_count;
  Animation *animations;
  int animation_count;
} Skeleton;

// Create skeleton
Skeleton *skeleton_create(int bone_count) {
  Skeleton *skel = (Skeleton *)calloc(1, sizeof(Skeleton));
  skel->bone_count = bone_count;
  skel->bones = (Bone *)calloc(bone_count, sizeof(Bone));
  return skel;
}

// Interpolate transforms
Transform transform_lerp(Transform *a, Transform *b, float t) {
  Transform result;

  // Lerp position
  for (int i = 0; i < 3; i++) {
    result.position[i] = a->position[i] * (1.0f - t) + b->position[i] * t;
    result.scale[i] = a->scale[i] * (1.0f - t) + b->scale[i] * t;
  }

  // Slerp rotation (simplified)
  for (int i = 0; i < 4; i++) {
    result.rotation[i] = a->rotation[i] * (1.0f - t) + b->rotation[i] * t;
  }

  return result;
}

// Sample animation
void animation_sample(Animation *anim, float time, Transform *out_transforms) {
  for (int i = 0; i < anim->track_count; i++) {
    AnimationTrack *track = &anim->tracks[i];

    // Find keyframes
    int k0 = 0, k1 = 0;
    for (int j = 0; j < track->keyframe_count - 1; j++) {
      if (time >= track->keyframes[j].time &&
          time < track->keyframes[j + 1].time) {
        k0 = j;
        k1 = j + 1;
        break;
      }
    }

    // Interpolate
    float t0 = track->keyframes[k0].time;
    float t1 = track->keyframes[k1].time;
    float t = (time - t0) / (t1 - t0);

    out_transforms[track->keyframes[k0].bone_id] = transform_lerp(
        &track->keyframes[k0].transform, &track->keyframes[k1].transform, t);
  }
}

// Update skeleton
void skeleton_update(Skeleton *skel, Transform *local_transforms) {
  for (int i = 0; i < skel->bone_count; i++) {
    skel->bones[i].local_transform = local_transforms[i];

    // Calculate world transform
    if (skel->bones[i].parent_id >= 0) {
      // Multiply with parent transform
      int parent_id = skel->bones[i].parent_id;
      
      // Transform multiplication: world = parent_world * local
      // Position: world_pos = parent_world_pos + (parent_world_rot * local_pos)
      float parent_world_pos[3] = {
        skel->bones[parent_id].world_transform.position[0],
        skel->bones[parent_id].world_transform.position[1],
        skel->bones[parent_id].world_transform.position[2]
      };
      
      float parent_world_rot[4] = {
        skel->bones[parent_id].world_transform.rotation[0],
        skel->bones[parent_id].world_transform.rotation[1],
        skel->bones[parent_id].world_transform.rotation[2],
        skel->bones[parent_id].world_transform.rotation[3]
      };
      
      float local_pos[3] = {
        local_transforms[i].position[0],
        local_transforms[i].position[1],
        local_transforms[i].position[2]
      };
      
      // Rotate local position by parent rotation
      float rotated_pos[3];
      quaternion_multiply_vector(parent_world_rot, local_pos, rotated_pos);
      
      // Add parent position
      skel->bones[i].world_transform.position[0] = parent_world_pos[0] + rotated_pos[0];
      skel->bones[i].world_transform.position[1] = parent_world_pos[1] + rotated_pos[1];
      skel->bones[i].world_transform.position[2] = parent_world_pos[2] + rotated_pos[2];
      
      // Combine rotations: world_rot = parent_world_rot * local_rot
      quaternion_multiply(parent_world_rot, local_transforms[i].rotation, skel->bones[i].world_transform.rotation);
      
      // Combine scales: world_scale = parent_world_scale * local_scale
      skel->bones[i].world_transform.scale[0] = skel->bones[parent_id].world_transform.scale[0] * local_transforms[i].scale[0];
      skel->bones[i].world_transform.scale[1] = skel->bones[parent_id].world_transform.scale[1] * local_transforms[i].scale[1];
      skel->bones[i].world_transform.scale[2] = skel->bones[parent_id].world_transform.scale[2] * local_transforms[i].scale[2];
    } else {
      skel->bones[i].world_transform = local_transforms[i];
    }
  }
}

/*
 * IMPLEMENTATION: 30/150 Animation TODOs
 * LOC: ~140
 */
