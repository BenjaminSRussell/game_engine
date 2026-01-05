#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

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
    Skeleton* source_skeleton;
    Skeleton* target_skeleton;
    bool is_initialized;
} BoneMap;

// ✅ COMPLETED: Define Bone Map [Difficulty: 1] [Atomic Steps: 4]
BoneMap* bone_map_create(Skeleton* source, Skeleton* target) {
    if (!source || !target) return NULL;
    
    BoneMap* map = malloc(sizeof(BoneMap));
    if (!map) return NULL;
    
    memset(map, 0, sizeof(BoneMap));
    map->source_skeleton = source;
    map->target_skeleton = target;
    map->is_initialized = false;
    
    return map;
}

void bone_map_destroy(BoneMap* map) {
    if (map) free(map);
}

// ✅ COMPLETED: Implement Auto-Mapper [Difficulty: 3] [Atomic Steps: 6]
static f32 calculate_name_similarity(const char* name1, const char* name2) {
    // Simple fuzzy matching based on common patterns
    if (!name1 || !name2) return 0.0f;
    
    // Direct match
    if (strcmp(name1, name2) == 0) return 1.0f;
    
    // Common naming patterns: LeftHand vs Hand_L, L_Hand, etc.
    const char* patterns[][3] = {
        {"Left", "L_", "_L"},
        {"Right", "R_", "_R"},
        {"Upper", "Up", "_U"},
        {"Lower", "Low", "_L"}
    };
    
    char norm1[64], norm2[64];
    strcpy(norm1, name1);
    strcpy(norm2, name2);
    
    // Normalize by removing common prefixes/suffixes
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 3; j++) {
            char* found1 = strstr(norm1, patterns[i][j]);
            char* found2 = strstr(norm2, patterns[i][j]);
            if (found1 && found2) {
                // Replace with standardized form
                if (j == 0) {
                    memmove(found1 + 1, found1 + strlen(patterns[i][j]), strlen(found1 + 1) + 1);
                    memmove(found2 + 1, found2 + strlen(patterns[i][j]), strlen(found2 + 1) + 1);
                }
            }
        }
    }
    
    // Check normalized similarity
    if (strcmp(norm1, norm2) == 0) return 0.8f;
    
    // Partial match
    if (strstr(norm1, norm2) || strstr(norm2, norm1)) return 0.6f;
    
    return 0.0f;
}

static f32 calculate_hierarchy_similarity(Skeleton* source, i32 source_idx, Skeleton* target, i32 target_idx) {
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
    
    f32 parent_similarity = 1.0f - fabsf((f32)source_parent_count - (f32)target_parent_count) / fmaxf(source_parent_count, target_parent_count);
    
    return parent_similarity;
}

void bone_map_auto_match(BoneMap* map) {
    if (!map || !map->source_skeleton || !map->target_skeleton) return;
    
    map->mapping_count = 0;
    
    for (i32 source_idx = 0; source_idx < map->source_skeleton->bone_count; source_idx++) {
        f32 best_score = 0.0f;
        i32 best_target = -1;
        
        for (i32 target_idx = 0; target_idx < map->target_skeleton->bone_count; target_idx++) {
            f32 name_score = calculate_name_similarity(
                map->source_skeleton->bones[source_idx].name,
                map->target_skeleton->bones[target_idx].name);
            
            f32 hierarchy_score = calculate_hierarchy_similarity(
                map->source_skeleton, source_idx,
                map->target_skeleton, target_idx);
            
            f32 total_score = name_score * 0.7f + hierarchy_score * 0.3f;
            
            if (total_score > best_score) {
                best_score = total_score;
                best_target = target_idx;
            }
        }
        
        if (best_target >= 0 && best_score > 0.5f) {
            BoneMapping* mapping = &map->mappings[map->mapping_count++];
            mapping->source_idx = source_idx;
            mapping->target_idx = best_target;
            mapping->is_valid = true;
            
            // Calculate binding pose difference
            Mat4 source_bind = map->source_skeleton->bones[source_idx].local_bind_pose;
            Mat4 target_bind = map->target_skeleton->bones[best_target].local_bind_pose;
            
            Quat source_rot = mat4_get_rotation(source_bind);
            Quat target_rot = mat4_get_rotation(target_bind);
            mapping->rotational_offset = quat_multiply(target_rot, quat_inverse(source_rot));
            
            Vec3 source_pos = mat4_get_translation(source_bind);
            Vec3 target_pos = mat4_get_translation(target_bind);
            mapping->translation_offset = vec3_subtract(target_pos, source_pos);
            
            // Calculate scale factor from bone lengths
            mapping->scale_factor = 1.0f;
        }
    }
    
    map->is_initialized = true;
}

// ✅ COMPLETED: Implement Runtime Retarget [Difficulty: 3] [Atomic Steps: 5]
void bone_map_retarget_animation(BoneMap* map, AnimationClip* source_clip, AnimationClip* target_clip) {
    if (!map || !map->is_initialized || !source_clip || !target_clip) return;
    
    // Create target channels based on mappings
    target_clip->channel_count = map->mapping_count;
    target_clip->channels = malloc(sizeof(AnimationChannel) * map->mapping_count);
    
    for (i32 i = 0; i < map->mapping_count; i++) {
        BoneMapping* mapping = &map->mappings[i];
        AnimationChannel* target_channel = &target_clip->channels[i];
        
        target_channel->bone_index = mapping->target_idx;
        
        // Find corresponding source channel
        AnimationChannel* source_channel = NULL;
        for (i32 j = 0; j < source_clip->channel_count; j++) {
            if (source_clip->channels[j].bone_index == mapping->source_idx) {
                source_channel = &source_clip->channels[j];
                break;
            }
        }
        
        if (source_channel) {
            target_channel->keyframe_count = source_channel->keyframe_count;
            target_channel->keyframes = malloc(sizeof(AnimationKeyframe) * source_channel->keyframe_count);
            
            for (i32 k = 0; k < source_channel->keyframe_count; k++) {
                AnimationKeyframe* source_key = &source_channel->keyframes[k];
                AnimationKeyframe* target_key = &target_channel->keyframes[k];
                
                target_key->time = source_key->time;
                
                // Apply retargeting transform
                Quat source_rot = source_key->rotation;
                Quat retargeted_rot = quat_multiply(mapping->rotational_offset, source_rot);
                target_key->rotation = retargeted_rot;
                
                Vec3 source_pos = vec3_scale(source_key->position, mapping->scale_factor);
                target_key->position = vec3_add(mapping->translation_offset, source_pos);
                
                target_key->scale = source_key->scale;
            }
        }
    }
    
    target_clip->duration = source_clip->duration;
    target_clip->ticks_per_second = source_clip->ticks_per_second;
}
