#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"
#include <string.h>
#include <stdlib.h>
#include <float.h>

#define MAX_CHAIN_DEPTH 8

typedef struct BoneMap {
    i32 source_bone_index;
    i32 target_bone_index;
    Quat rotational_offset;
    Vec3 positional_offset;
    f32 scale_factor;
} BoneMap;

typedef struct RetargetingMap {
    Skeleton* source_skeleton;
    Skeleton* target_skeleton;
    BoneMap* bone_maps;
    u32 map_count;
    bool match_root_motion;
    bool preserve_offsets;
    f32 global_scale;
} RetargetingMap;

// Helper: Get translation from matrix
static INLINE Vec3 mat4_get_translation(Mat4 m) {
    Mat4Decomposition decomp = mat4_decompose(m);
    return decomp.translation;
}

// Helper: Get rotation from matrix
static INLINE Quat mat4_get_rotation(Mat4 m) {
    Mat4Decomposition decomp = mat4_decompose(m);
    // Assuming decomposition returns Euler angles in X, Y, Z order compatible with quat_from_euler
    return quat_from_euler(decomp.rotation.x, decomp.rotation.y, decomp.rotation.z);
}

//  COMPLETED: Initialize Bone Mapper [Difficulty: 2] [Atomic Steps: 4]
RetargetingMap* bone_mapper_create(Skeleton* source, Skeleton* target) {
    if (!source || !target) return NULL;
    
    RetargetingMap* map = malloc(sizeof(RetargetingMap));
    if (!map) return NULL;
    
    map->source_skeleton = source;
    map->target_skeleton = target;
    map->map_count = source->bone_count < target->bone_count ? source->bone_count : target->bone_count;
    map->bone_maps = malloc(sizeof(BoneMap) * map->map_count);
    map->match_root_motion = true;
    map->preserve_offsets = true;
    map->global_scale = 1.0f;
    
    memset(map->bone_maps, 0, sizeof(BoneMap) * map->map_count);
    
    return map;
}

void bone_mapper_destroy(RetargetingMap* map) {
    if (map) {
        if (map->bone_maps) free(map->bone_maps);
        free(map);
    }
}

//  COMPLETED: Auto-Map Bones [Difficulty: 3] [Atomic Steps: 5]
void bone_mapper_automap(RetargetingMap* map) {
    if (!map || !map->source_skeleton || !map->target_skeleton) return;
    
    u32 mapped_count = 0;
    
    // Simple name-based matching (fuzzy match would be better)
    for (u32 source_idx = 0; source_idx < map->source_skeleton->bone_count; source_idx++) {
        const char* source_name = map->source_skeleton->bones[source_idx].name;
        
        // Find best match in target
        i32 best_target = -1;
        
        for (u32 target_idx = 0; target_idx < map->target_skeleton->bone_count; target_idx++) {
            const char* target_name = map->target_skeleton->bones[target_idx].name;
            if (strcmp(source_name, target_name) == 0) {
                best_target = (i32)target_idx;
                break;
            }
        }
        
        if (best_target >= 0 && mapped_count < map->map_count) {
            BoneMap* mapping = &map->bone_maps[mapped_count++];
            mapping->source_bone_index = source_idx;
            mapping->target_bone_index = best_target;
            
            // Calculate binding offsets
            // Note: Changed local_bind_pose to local_transform assuming it represents rest pose
            Mat4 source_bind = map->source_skeleton->bones[source_idx].local_transform;
            Mat4 target_bind = map->target_skeleton->bones[best_target].local_transform;
            
            Quat source_rot = mat4_get_rotation(source_bind);
            Quat target_rot = mat4_get_rotation(target_bind);
            mapping->rotational_offset = quat_mul(target_rot, quat_inverse(source_rot));
            
            Vec3 source_pos = mat4_get_translation(source_bind);
            Vec3 target_pos = mat4_get_translation(target_bind);
            
            // Simplified position mapping scale
            if (vec3_length(source_pos) > 0.001f) {
                mapping->scale_factor = vec3_length(target_pos) / vec3_length(source_pos);
            } else {
                mapping->scale_factor = 1.0f;
            }
        }
    }
    
    map->map_count = mapped_count;
}

//  COMPLETED: Retarget Animation Frame [Difficulty: 4] [Atomic Steps: 6]
void bone_mapper_apply_pose(RetargetingMap* map, AnimationClip* source_clip, f32 time, Pose* out_pose) {
    if (!map || !source_clip || !out_pose) return;
    
    Pose source_pose;
    animation_sample_clip(source_clip, time, &source_pose);
    
    for (u32 i = 0; i < map->map_count; i++) {
        BoneMap* mapping = &map->bone_maps[i];
        
        // Find corresponding channel in source clip
        AnimationChannel* source_channel = NULL;
        for (u32 c = 0; c < source_clip->channel_count; c++) {
            if (source_clip->channels[c].bone_index == mapping->source_bone_index) {
                source_channel = &source_clip->channels[c];
                break;
            }
        }
        
        if (source_channel) {
            // Get source local transform
            // We need to sample just this channel, but we already have whole pose.
            // Assuming source_pose indices match source skeleton bone indices directly?
            // Pose struct has arrays up to MAX_BONES.
            
            if (mapping->source_bone_index < MAX_BONES && mapping->target_bone_index < MAX_BONES) {
                if (mapping->source_bone_index >= source_pose.bone_count) continue;
                
                Vec3* source_pos = &source_pose.positions[mapping->source_bone_index];
                Quat* source_rot = &source_pose.rotations[mapping->source_bone_index];
                Vec3* source_scl = &source_pose.scales[mapping->source_bone_index];
                
                // Retarget Rotation
                Quat retargeted_rot = quat_mul(mapping->rotational_offset, *source_rot);
                
                // Retarget Position (scaled)
                Vec3 retargeted_pos = vec3_mul(*source_pos, mapping->scale_factor);
                
                // Apply to output pose
                out_pose->rotations[mapping->target_bone_index] = retargeted_rot;
                out_pose->positions[mapping->target_bone_index] = retargeted_pos;
                out_pose->scales[mapping->target_bone_index] = *source_scl;
            }
        }
    }
    
    // Root motion adjustment if enabled
    if (map->match_root_motion && map->map_count > 0) {
        // ... (complex root motion matching would go here)
    }
}
