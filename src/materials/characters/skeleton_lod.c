/**
 * =================================================================================================
 *                           SKELETON LOD SYSTEM IMPLEMENTATION
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of skeleton Level of Detail (LOD) system for performance
 * optimization through bone reduction based on distance and importance.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Skeleton LOD (Level of Detail) system
typedef struct {
    u32 lod_level;
    u32 bone_count;
    u32 bone_indices[MAX_BONES_PER_SKELETON];
    f32 quality_factor;
    bool is_valid;
} SkeletonLOD;

typedef struct {
    SkeletonLOD lods[4]; // 4 LOD levels (0 = highest, 3 = lowest)
    u32 lod_count;
    f32 transition_distances[4]; // Distance thresholds for LOD transitions
    bool auto_lod_enabled;
} SkeletonLODSystem;

// Bone importance categories for LOD
typedef enum {
    BONE_IMPORTANCE_ESSENTIAL = 4,  // Never remove (Root, Pelvis, Chest, Head)
    BONE_IMPORTANCE_HIGH = 3,       // Keep at high LOD (Spine, major limbs)
    BONE_IMPORTANCE_MEDIUM = 2,     // Remove at medium LOD (fingers, toes)
    BONE_IMPORTANCE_LOW = 1,        // Remove at low LOD (detail bones)
    BONE_IMPORTANCE_DECORATIVE = 0  // Always remove (cosmetic bones)
} BoneImportance;

// Get bone importance based on name and hierarchy
static BoneImportance get_bone_importance(const SkeletonTemplate* template, u32 bone_index) {
    if (!template || bone_index >= template->bone_count) {
        return BONE_IMPORTANCE_LOW;
    }
    
    const BoneDefinition* bone = &template->bones[bone_index];
    const char* name = bone->name;
    
    // Essential bones (never remove)
    if (strstr(name, "Root") || strstr(name, "Pelvis") || strstr(name, "Hips") ||
        strstr(name, "Chest") || strstr(name, "Spine") || strstr(name, "Head")) {
        return BONE_IMPORTANCE_ESSENTIAL;
    }
    
    // High importance bones
    if (strstr(name, "Neck") || strstr(name, "Clavicle") || 
        strstr(name, "Arm") || strstr(name, "Thigh") || strstr(name, "Leg")) {
        return BONE_IMPORTANCE_HIGH;
    }
    
    // Medium importance bones
    if (strstr(name, "Forearm") || strstr(name, "Calf") || strstr(name, "Hand") ||
        strstr(name, "Foot") || strstr(name, "Toe")) {
        return BONE_IMPORTANCE_MEDIUM;
    }
    
    // Low importance bones (fingers, decorative)
    if (strstr(name, "Finger") || strstr(name, "Thumb") || strstr(name, "Index") ||
        strstr(name, "Middle") || strstr(name, "Ring") || strstr(name, "Pinky")) {
        return BONE_IMPORTANCE_LOW;
    }
    
    // Decorative bones (props, accessories)
    if (strstr(name, "Prop") || strstr(name, "Accessory") || strstr(name, "Weapon")) {
        return BONE_IMPORTANCE_DECORATIVE;
    }
    
    // Default to medium for unknown bones
    return BONE_IMPORTANCE_MEDIUM;
}

// Create LOD level by reducing bones based on importance threshold
static SkeletonLOD create_lod_level(const SkeletonTemplate* template, u32 lod_level, 
                                   BoneImportance min_importance) {
    SkeletonLOD lod = {0};
    lod.lod_level = lod_level;
    lod.quality_factor = 1.0f - (f32)lod_level * 0.25f; // 25% quality reduction per LOD
    lod.is_valid = true;
    
    if (!template || !template->bones) {
        lod.is_valid = false;
        return lod;
    }
    
    // Count bones that meet importance threshold
    u32 bone_count = 0;
    for (u32 i = 0; i < template->bone_count; i++) {
        BoneImportance importance = get_bone_importance(template, i);
        if (importance >= min_importance) {
            bone_count++;
        }
    }
    
    lod.bone_count = bone_count;
    
    // Collect bone indices
    u32 index = 0;
    for (u32 i = 0; i < template->bone_count && index < bone_count; i++) {
        BoneImportance importance = get_bone_importance(template, i);
        if (importance >= min_importance) {
            lod.bone_indices[index++] = i;
        }
    }
    
    LOG_DEBUG("Created LOD %u: %u bones (min importance: %d)", 
             lod_level, bone_count, min_importance);
    
    return lod;
}

// Create LOD system for skeleton
static SkeletonLODSystem create_lod_system(const SkeletonTemplate* template) {
    SkeletonLODSystem lod_system = {0};
    lod_system.lod_count = 4;
    lod_system.auto_lod_enabled = true;
    
    // Set default transition distances
    lod_system.transition_distances[0] = 0.0f;    // LOD 0: Always visible
    lod_system.transition_distances[1] = 10.0f;   // LOD 1: Switch at 10 units
    lod_system.transition_distances[2] = 25.0f;   // LOD 2: Switch at 25 units
    lod_system.transition_distances[3] = 50.0f;   // LOD 3: Switch at 50 units
    
    if (!template || !template->bones) {
        lod_system.auto_lod_enabled = false;
        return lod_system;
    }
    
    // Create LOD levels
    lod_system.lods[0] = create_lod_level(template, 0, BONE_IMPORTANCE_LOW);        // All bones
    lod_system.lods[1] = create_lod_level(template, 1, BONE_IMPORTANCE_MEDIUM);    // Remove decorative bones
    lod_system.lods[2] = create_lod_level(template, 2, BONE_IMPORTANCE_HIGH);       // Remove low/medium bones
    lod_system.lods[3] = create_lod_level(template, 3, BONE_IMPORTANCE_ESSENTIAL);   // Only essential bones
    
    LOG_INFO("Created LOD system with %u levels", lod_system.lod_count);
    return lod_system;
}

// Get appropriate LOD level based on distance
static u32 get_lod_level(const SkeletonLODSystem* lod_system, f32 distance) {
    if (!lod_system || !lod_system->auto_lod_enabled) {
        return 0; // Always use highest LOD if auto LOD is disabled
    }
    
    for (u32 i = lod_system->lod_count - 1; i > 0; i--) {
        if (distance >= lod_system->transition_distances[i]) {
            return i;
        }
    }
    
    return 0; // Use highest LOD for close distances
}

// Create reduced skeleton for specific LOD
static SkeletonTemplate* create_reduced_skeleton(const SkeletonTemplate* original, 
                                                const SkeletonLOD* lod) {
    if (!original || !lod || !lod->is_valid) {
        return NULL;
    }
    
    // Create new skeleton template with reduced bone count
    SkeletonTemplate* reduced = skeleton_template_create("Reduced", lod->bone_count);
    if (!reduced) {
        return NULL;
    }
    
    // Copy bones that are included in this LOD
    for (u32 i = 0; i < lod->bone_count; i++) {
        u32 original_index = lod->bone_indices[i];
        const BoneDefinition* original_bone = &original->bones[original_index];
        
        // Find new parent index
        s32 new_parent_index = -1;
        if (original_bone->parent_index >= 0) {
            // Find parent in LOD
            for (u32 j = 0; j < lod->bone_count; j++) {
                if (lod->bone_indices[j] == (u32)original_bone->parent_index) {
                    new_parent_index = (s32)j;
                    break;
                }
            }
        }
        
        // Add bone to reduced skeleton
        skeleton_template_add_bone(reduced, original_bone->name, new_parent_index,
                                 original_bone->local_position, original_bone->local_rotation,
                                 original_bone->local_scale, original_bone->is_essential);
    }
    
    // Copy IK chains (only if both start and end bones are in LOD)
    for (u32 i = 0; i < original->ik_chain_count; i++) {
        s32 start = original->ik_chains[i].start;
        s32 end = original->ik_chains[i].end;
        
        bool start_in_lod = false;
        bool end_in_lod = false;
        s32 new_start = -1;
        s32 new_end = -1;
        
        for (u32 j = 0; j < lod->bone_count; j++) {
            if (lod->bone_indices[j] == (u32)start) {
                start_in_lod = true;
                new_start = (s32)j;
            }
            if (lod->bone_indices[j] == (u32)end) {
                end_in_lod = true;
                new_end = (s32)j;
            }
        }
        
        if (start_in_lod && end_in_lod) {
            skeleton_template_add_ik_chain(reduced, new_start, new_end);
        }
    }
    
    // Copy twist bones (only if bone is in LOD)
    for (u32 i = 0; i < original->twist_bone_count; i++) {
        s32 bone_index = original->twist_bone_indices[i];
        
        for (u32 j = 0; j < lod->bone_count; j++) {
            if (lod->bone_indices[j] == (u32)bone_index) {
                skeleton_template_add_twist_bone(reduced, (s32)j);
                break;
            }
        }
    }
    
    LOG_DEBUG("Created reduced skeleton with %u bones (LOD %u, quality %.2f)", 
             lod->bone_count, lod->lod_level, lod->quality_factor);
    
    return reduced;
}

// Public API for skeleton LOD
SkeletonLODSystem skeleton_create_lod_system(const SkeletonTemplate* template) {
    return create_lod_system(template);
}

SkeletonTemplate* skeleton_get_lod_skeleton(const SkeletonTemplate* original, 
                                            const SkeletonLODSystem* lod_system, 
                                            f32 distance) {
    if (!original || !lod_system) {
        return NULL;
    }
    
    u32 lod_level = get_lod_level(lod_system, distance);
    const SkeletonLOD* lod = &lod_system->lods[lod_level];
    
    return create_reduced_skeleton(original, lod);
}

u32 skeleton_get_current_lod_level(const SkeletonLODSystem* lod_system, f32 distance) {
    return get_lod_level(lod_system, distance);
}

f32 skeleton_get_lod_quality_factor(const SkeletonLODSystem* lod_system, u32 lod_level) {
    if (!lod_system || lod_level >= lod_system->lod_count) {
        return 1.0f;
    }
    
    return lod_system->lods[lod_level].quality_factor;
}

void skeleton_set_lod_transition_distance(SkeletonLODSystem* lod_system, 
                                          u32 lod_level, f32 distance) {
    if (!lod_system || lod_level >= lod_system->lod_count) {
        return;
    }
    
    lod_system->transition_distances[lod_level] = distance;
    LOG_DEBUG("Set LOD %u transition distance to %.1f", lod_level, distance);
}

void skeleton_enable_auto_lod(SkeletonLODSystem* lod_system, bool enabled) {
    if (!lod_system) return;
    
    lod_system->auto_lod_enabled = enabled;
    LOG_INFO("Auto LOD %s", enabled ? "enabled" : "disabled");
}

bool skeleton_is_auto_lod_enabled(const SkeletonLODSystem* lod_system) {
    return lod_system && lod_system->auto_lod_enabled;
}

// Export LOD system data
bool skeleton_export_lod_data(const SkeletonLODSystem* lod_system, const char* file_path) {
    if (!lod_system || !file_path) {
        return false;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open LOD data file: %s", file_path);
        return false;
    }
    
    fprintf(file, "# Skeleton LOD System Data\n");
    fprintf(file, "# Generated automatically\n\n");
    fprintf(file, "lod_count: %u\n", lod_system->lod_count);
    fprintf(file, "auto_lod_enabled: %s\n\n", lod_system->auto_lod_enabled ? "true" : "false");
    
    fprintf(file, "# Transition Distances\n");
    fprintf(file, "# lod_level, distance\n");
    for (u32 i = 0; i < lod_system->lod_count; i++) {
    }
    
    fprintf(file, "\n# LOD Levels\n");
    fprintf(file, "# lod_level, bone_count, quality_factor\n");
    for (u32 i = 0; i < lod_system->lod_count; i++) {
        const SkeletonLOD* lod = &lod_system->lods[i];
        fprintf(file, "%u, %u, %.3f\n", lod->lod_level, lod->bone_count, lod->quality_factor);
        
        fprintf(file, "# LOD %u bone indices: ", i);
        for (u32 j = 0; j < lod->bone_count; j++) {
            if (j < lod->bone_count - 1) fprintf(file, ", ");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    LOG_INFO("Exported LOD system data to: %s", file_path);
    return true;
}
