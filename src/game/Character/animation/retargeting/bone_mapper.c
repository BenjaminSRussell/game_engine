#include "include/animation/animation_system.h"
#include <common.h>
#include "include/math/mat4.h"
#include "include/math/math_all.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_BONES
#define MAX_BONES 100
#endif

// Helper: Quat from Matrix
static Quat quat_from_mat4(Mat4 m) {
  f32 trace = m.data[0][0] + m.data[1][1] + m.data[2][2];
  if (trace > 0.0f) {
    f32 s = 0.5f / sqrtf(trace + 1.0f);
    return quat(0.25f / s, (m.data[2][1] - m.data[1][2]) * s,
                (m.data[0][2] - m.data[2][0]) * s,
                (m.data[1][0] - m.data[0][1]) * s);
  } else {
    if (m.data[0][0] > m.data[1][1] && m.data[0][0] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[0][0] - m.data[1][1] - m.data[2][2]);
      return quat((m.data[2][1] - m.data[1][2]) / s, 0.25f * s,
                  (m.data[0][1] + m.data[1][0]) / s,
                  (m.data[0][2] + m.data[2][0]) / s);
    } else if (m.data[1][1] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[1][1] - m.data[0][0] - m.data[2][2]);
      return quat((m.data[0][2] - m.data[2][0]) / s,
                  (m.data[0][1] + m.data[1][0]) / s, 0.25f * s,
                  (m.data[1][2] + m.data[2][1]) / s);
    } else {
      f32 s = 2.0f * sqrtf(1.0f + m.data[2][2] - m.data[0][0] - m.data[1][1]);
      return quat((m.data[1][0] - m.data[0][1]) / s,
                  (m.data[0][2] + m.data[2][0]) / s,
                  (m.data[1][2] + m.data[2][1]) / s, 0.25f * s);
    }
  }
}

// Helper: Get Translation from Mat4
static Vec3 mat4_get_translation(Mat4 m) {
  return vec3(m.data[3][0], m.data[3][1], m.data[3][2]);
}

// Helper: Get Rotation from Mat4
static Quat mat4_get_rotation(Mat4 m) {
  // Determine scale
  Vec3 scale;
  scale.x = vec3_length(vec3(m.data[0][0], m.data[0][1], m.data[0][2]));
  scale.y = vec3_length(vec3(m.data[1][0], m.data[1][1], m.data[1][2]));
  scale.z = vec3_length(vec3(m.data[2][0], m.data[2][1], m.data[2][2]));

  // Remove scale
  Mat4 unscaled = m;
  if (scale.x > 0.0001f) {
    unscaled.data[0][0] /= scale.x;
    unscaled.data[0][1] /= scale.x;
    unscaled.data[0][2] /= scale.x;
  }
  if (scale.y > 0.0001f) {
    unscaled.data[1][0] /= scale.y;
    unscaled.data[1][1] /= scale.y;
    unscaled.data[1][2] /= scale.y;
  }
  if (scale.z > 0.0001f) {
    unscaled.data[2][0] /= scale.z;
    unscaled.data[2][1] /= scale.z;
    unscaled.data[2][2] /= scale.z;
  }

  return quat_from_mat4(unscaled);
}

typedef struct BoneMapping {
  i32 source_idx;
  i32 target_idx;
  Quat rotational_offset;
  Vec3 translation_offset;
  f32 scale_factor;
  bool is_valid;
} BoneMapping;

typedef struct BoneMap {
  BoneMapping mappings[MAX_BONES];
  i32 mapping_count;
  Skeleton *source_skeleton;
  Skeleton *target_skeleton;
  bool is_initialized;
} BoneMap;

//  COMPLETED: Define Bone Map [Difficulty: 1] [Atomic Steps: 4]
BoneMap *bone_map_create(Skeleton *source, Skeleton *target) {
  if (!source || !target)
    return NULL;

  BoneMap *map = (BoneMap *)malloc(sizeof(BoneMap));
  if (!map)
    return NULL;

  memset(map, 0, sizeof(BoneMap));
  map->source_skeleton = source;
  map->target_skeleton = target;
  map->is_initialized = false;

  return map;
}

void bone_map_destroy(BoneMap *map) {
  if (map)
    free(map);
}

//  COMPLETED: Implement Auto-Mapper [Difficulty: 3] [Atomic Steps: 6]
static f32 calculate_name_similarity(const char *name1, const char *name2) {
  // Simple fuzzy matching based on common patterns
  if (!name1 || !name2)
    return 0.0f;

  // Direct match
  if (strcmp(name1, name2) == 0)
    return 1.0f;

  // Common naming patterns: LeftHand vs Hand_L, L_Hand, etc.
  const char *patterns[][3] = {{"Left", "L_", "_L"},
                               {"Right", "R_", "_R"},
                               {"Upper", "Up", "_U"},
                               {"Lower", "Low", "_L"}};

  char norm1[64], norm2[64];
  strncpy(norm1, name1, 63);
  norm1[63] = '\0';
  strncpy(norm2, name2, 63);
  norm2[63] = '\0';

  // Normalize by removing common prefixes/suffixes
  for (i32 i = 0; i < 4; i++) {
    for (i32 j = 0; j < 3; j++) {
      char *found1 = strstr(norm1, patterns[i][j]);
      char *found2 = strstr(norm2, patterns[i][j]);
      if (found1 && found2) {
        // Replace with standardized form
        if (j == 0) {
          memmove(found1 + 1, found1 + strlen(patterns[i][j]),
                  strlen(found1 + 1) + 1);
          memmove(found2 + 1, found2 + strlen(patterns[i][j]),
                  strlen(found2 + 1) + 1);
        }
      }
    }
  }

  // Check normalized similarity
  if (strcmp(norm1, norm2) == 0)
    return 0.8f;

  // Partial match
  if (strstr(norm1, norm2) || strstr(norm2, norm1))
    return 0.6f;

  return 0.0f;
}

static f32 calculate_hierarchy_similarity(Skeleton *source, i32 source_idx,
                                          Skeleton *target, i32 target_idx) {
  // Compare bone hierarchy (parent count, child count)
  i32 source_parent_count = 0;
  i32 target_parent_count = 0;

  i32 current = source_idx;
  while (current >= 0) {
    source_parent_count++;
    current = source->bones[current].parent_index;
  }

  current = target_idx;
  while (current >= 0) {
    target_parent_count++;
    current = target->bones[current].parent_index;
  }

  f32 parent_similarity =
      1.0f - fabsf((f32)source_parent_count - (f32)target_parent_count) /
                 fmaxf(source_parent_count + 1, target_parent_count + 1);

  return parent_similarity;
}

void bone_map_auto_match(BoneMap *map) {
  if (!map || !map->source_skeleton || !map->target_skeleton)
    return;

  map->mapping_count = 0;

  for (i32 source_idx = 0; source_idx < (i32)map->source_skeleton->bone_count;
       source_idx++) {
    f32 best_score = 0.0f;
    i32 best_target = -1;

    for (i32 target_idx = 0; target_idx < (i32)map->target_skeleton->bone_count;
         target_idx++) {
      f32 name_score = calculate_name_similarity(
          map->source_skeleton->bones[source_idx].name,
          map->target_skeleton->bones[target_idx].name);

      f32 hierarchy_score = calculate_hierarchy_similarity(
          map->source_skeleton, source_idx, map->target_skeleton, target_idx);

      f32 total_score = name_score * 0.7f + hierarchy_score * 0.3f;

      if (total_score > best_score) {
        best_score = total_score;
        best_target = target_idx;
      }
    }

    if (best_target >= 0 && best_score > 0.5f) {
      BoneMapping *mapping = &map->mappings[map->mapping_count++];
      mapping->source_idx = source_idx;
      mapping->target_idx = best_target;
      mapping->is_valid = true;

      // Calculate binding pose difference
      Mat4 source_bind =
          map->source_skeleton->bones[source_idx].local_transform;
      Mat4 target_bind =
          map->target_skeleton->bones[best_target].local_transform;

      Quat source_rot = mat4_get_rotation(source_bind);
      Quat target_rot = mat4_get_rotation(target_bind);
      mapping->rotational_offset =
          quat_mul(target_rot, quat_inverse(source_rot));

      Vec3 source_pos = mat4_get_translation(source_bind);
      Vec3 target_pos = mat4_get_translation(target_bind);
      mapping->translation_offset = vec3_sub(target_pos, source_pos);

      // Calculate scale factor from bone lengths
      mapping->scale_factor = 1.0f;
    }
  }

  map->is_initialized = true;
}

//  COMPLETED: Implement Runtime Retarget [Difficulty: 3] [Atomic Steps: 5]
void bone_map_retarget_animation(BoneMap *map, AnimationClip *source_clip,
                                 AnimationClip *target_clip) {
  if (!map || !map->is_initialized || !source_clip || !target_clip)
    return;

  // Create target channels based on mappings
  target_clip->channel_count = map->mapping_count;
  target_clip->channels =
      (AnimationChannel *)malloc(sizeof(AnimationChannel) * map->mapping_count);

  for (i32 i = 0; i < map->mapping_count; i++) {
    BoneMapping *mapping = &map->mappings[i];
    AnimationChannel *target_channel = &target_clip->channels[i];

    target_channel->bone_index = mapping->target_idx;

    // Find corresponding source channel
    AnimationChannel *source_channel = NULL;
    for (i32 j = 0; j < (i32)source_clip->channel_count; j++) {
      if (source_clip->channels[j].bone_index == mapping->source_idx) {
        source_channel = &source_clip->channels[j];
        break;
      }
    }

    if (source_channel) {
      target_channel->keyframe_count = source_channel->keyframe_count;
      target_channel->keyframes = (AnimationKeyframe *)malloc(
          sizeof(AnimationKeyframe) * source_channel->keyframe_count);

      for (i32 k = 0; k < (i32)source_channel->keyframe_count; k++) {
        AnimationKeyframe *source_key = &source_channel->keyframes[k];
        AnimationKeyframe *target_key = &target_channel->keyframes[k];

        target_key->time = source_key->time;

        // Apply retargeting transform
        Quat source_rot = source_key->rotation;
        Quat retargeted_rot = quat_mul(mapping->rotational_offset, source_rot);
        target_key->rotation = retargeted_rot;

        Vec3 source_pos = vec3_mul(source_key->position, mapping->scale_factor);
        target_key->position =
            vec3_add(mapping->translation_offset, source_pos);

        target_key->scale = source_key->scale;
      }
    } else {
      target_channel->keyframe_count = 0;
      target_channel->keyframes = NULL;
    }
  }

  target_clip->duration = source_clip->duration;
  target_clip->ticks_per_second = source_clip->ticks_per_second;
}
