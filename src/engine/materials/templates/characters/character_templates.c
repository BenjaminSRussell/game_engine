/**
 * =================================================================================================
 *                           CHARACTER TEMPLATES IMPLEMENTATION
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of character template system with skeleton creation,
 * validation, and management capabilities.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include <core/logger.h>
#include <core/memory.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Skeleton template creation system
#define MAX_SKELETON_TEMPLATES 32
#define MAX_BONES_PER_SKELETON 256

typedef struct {
    SkeletonTemplate templates[MAX_SKELETON_TEMPLATES];
    u32 template_count;
    bool system_initialized;
} SkeletonTemplateSystem;

static SkeletonTemplateSystem g_skeleton_system = {0};

// Initialize skeleton template system
bool skeleton_template_system_init(void) {
    if (g_skeleton_system.system_initialized) {
        return true;
    }
    
    memset(&g_skeleton_system, 0, sizeof(SkeletonTemplateSystem));
    g_skeleton_system.system_initialized = true;
    
    LOG_INFO("Skeleton template system initialized");
    return true;
}

// Create a new skeleton template
SkeletonTemplate* skeleton_template_create(const char* name, u32 bone_count) {
    if (!name || bone_count == 0 || bone_count > MAX_BONES_PER_SKELETON) {
        LOG_ERROR("Invalid parameters for skeleton template creation");
        return NULL;
    }
    
    if (g_skeleton_system.template_count >= MAX_SKELETON_TEMPLATES) {
        LOG_ERROR("Maximum skeleton templates reached");
        return NULL;
    }
    
    // Allocate memory for bones
    BoneDefinition* bones = (BoneDefinition*)calloc(bone_count, sizeof(BoneDefinition));
    if (!bones) {
        LOG_ERROR("Failed to allocate memory for skeleton bones");
        return NULL;
    }
    
    // Create new template
    SkeletonTemplate* template = &g_skeleton_system.templates[g_skeleton_system.template_count];
    
    strncpy(template->name, name, sizeof(template->name) - 1);
    template->name[sizeof(template->name) - 1] = '\0';
    template->bones = bones;
    template->bone_count = bone_count;
    template->ik_chain_count = 0;
    template->twist_bone_count = 0;
    
    // Initialize bones to default values
    for (u32 i = 0; i < bone_count; i++) {
        strncpy(template->bones[i].name, "Unnamed", sizeof(template->bones[i].name) - 1);
        template->bones[i].parent_index = -1;
        template->bones[i].local_position[0] = 0.0f;
        template->bones[i].local_position[1] = 0.0f;
        template->bones[i].local_position[2] = 0.0f;
        template->bones[i].local_rotation[0] = 0.0f;
        template->bones[i].local_rotation[1] = 0.0f;
        template->bones[i].local_rotation[2] = 0.0f;
        template->bones[i].local_rotation[3] = 1.0f;
        template->bones[i].local_scale[0] = 1.0f;
        template->bones[i].local_scale[1] = 1.0f;
        template->bones[i].local_scale[2] = 1.0f;
        template->bones[i].is_essential = false;
    }
    
    g_skeleton_system.template_count++;
    
    LOG_INFO("Created skeleton template '%s' with %u bones", name, bone_count);
    return template;
}

// Add bone to skeleton template
bool skeleton_template_add_bone(SkeletonTemplate* template, const char* name,
                               s32 parent_index, const f32 position[3], 
                               const f32 rotation[4], const f32 scale[3],
                               bool is_essential) {
    if (!template || !name || !template->bones) {
        LOG_ERROR("Invalid template or bone data");
        return false;
    }
    
    // Find next available bone slot
    u32 bone_index = 0;
    for (; bone_index < template->bone_count; bone_index++) {
        if (template->bones[bone_index].name[0] == '\0') {
            break; // Found empty slot
        }
    }
    
    if (bone_index >= template->bone_count) {
        LOG_ERROR("No available bone slots in template");
        return false;
    }
    
    // Validate parent index
    if (parent_index >= 0 && (u32)parent_index >= template->bone_count) {
        LOG_ERROR("Invalid parent index: %d", parent_index);
        return false;
    }
    
    // Add bone
    BoneDefinition* bone = &template->bones[bone_index];
    strncpy(bone->name, name, sizeof(bone->name) - 1);
    bone->name[sizeof(bone->name) - 1] = '\0';
    bone->parent_index = parent_index;
    
    if (position) {
        bone->local_position[0] = position[0];
        bone->local_position[1] = position[1];
        bone->local_position[2] = position[2];
    }
    
    if (rotation) {
        bone->local_rotation[0] = rotation[0];
        bone->local_rotation[1] = rotation[1];
        bone->local_rotation[2] = rotation[2];
        bone->local_rotation[3] = rotation[3];
    }
    
    if (scale) {
        bone->local_scale[0] = scale[0];
        bone->local_scale[1] = scale[1];
        bone->local_scale[2] = scale[2];
    }
    
    bone->is_essential = is_essential;
    
    LOG_DEBUG("Added bone '%s' to skeleton template '%s'", name, template->name);
    return true;
}

// Create IK chain in skeleton template
bool skeleton_template_add_ik_chain(SkeletonTemplate* template, s32 start_bone, s32 end_bone) {
    if (!template || !template->bones) {
        LOG_ERROR("Invalid template for IK chain");
        return false;
    }
    
    if (template->ik_chain_count >= 8) {
        LOG_ERROR("Maximum IK chains reached");
        return false;
    }
    
    // Validate bone indices
    if (start_bone < 0 || (u32)start_bone >= template->bone_count ||
        end_bone < 0 || (u32)end_bone >= template->bone_count) {
        LOG_ERROR("Invalid bone indices for IK chain: %d to %d", start_bone, end_bone);
        return false;
    }
    
    template->ik_chains[template->ik_chain_count].start = start_bone;
    template->ik_chains[template->ik_chain_count].end = end_bone;
    template->ik_chain_count++;
    
    LOG_DEBUG("Added IK chain from bone %d to bone %d in template '%s'", 
             start_bone, end_bone, template->name);
    return true;
}

// Add twist bone to skeleton template
bool skeleton_template_add_twist_bone(SkeletonTemplate* template, s32 bone_index) {
    if (!template || !template->bones) {
        LOG_ERROR("Invalid template for twist bone");
        return false;
    }
    
    if (template->twist_bone_count >= 16) {
        LOG_ERROR("Maximum twist bones reached");
        return false;
    }
    
    // Validate bone index
    if (bone_index < 0 || (u32)bone_index >= template->bone_count) {
        LOG_ERROR("Invalid bone index for twist bone: %d", bone_index);
        return false;
    }
    
    template->twist_bone_indices[template->twist_bone_count] = bone_index;
    template->twist_bone_count++;
    
    LOG_DEBUG("Added twist bone %d to template '%s'", bone_index, template->name);
    return true;
}

// Find skeleton template by name
SkeletonTemplate* skeleton_template_find(const char* name) {
    if (!name) {
        LOG_ERROR("Invalid name for template search");
        return NULL;
    }
    
    for (u32 i = 0; i < g_skeleton_system.template_count; i++) {
        if (strcmp(g_skeleton_system.templates[i].name, name) == 0) {
            return &g_skeleton_system.templates[i];
        }
    }
    
    LOG_DEBUG("Skeleton template '%s' not found", name);
    return NULL;
}

// Get bone index by name
s32 skeleton_template_get_bone_index(const SkeletonTemplate* template, const char* bone_name) {
    if (!template || !bone_name || !template->bones) {
        return -1;
    }
    
    for (u32 i = 0; i < template->bone_count; i++) {
        if (strcmp(template->bones[i].name, bone_name) == 0) {
            return (s32)i;
        }
    }
    
    return -1;
}

// Create predefined humanoid skeleton template
SkeletonTemplate* skeleton_template_create_humanoid(const char* name) {
    SkeletonTemplate* template = skeleton_template_create(name, 31); // Standard humanoid bone count
    if (!template) {
        return NULL;
    }
    
    // Root bone
    skeleton_template_add_bone(template, "Root", -1, 
                             (f32[]){0.0f, 0.0f, 0.0f}, 
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f}, 
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Pelvis
    skeleton_template_add_bone(template, "Pelvis", 0,
                             (f32[]){0.0f, 1.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Spine chain
    skeleton_template_add_bone(template, "Spine_1", 1,
                             (f32[]){0.0f, 0.1f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Spine_2", 2,
                             (f32[]){0.0f, 0.15f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Spine_3", 3,
                             (f32[]){0.0f, 0.15f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Chest
    skeleton_template_add_bone(template, "Chest", 4,
                             (f32[]){0.0f, 0.2f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Neck
    skeleton_template_add_bone(template, "Neck", 5,
                             (f32[]){0.0f, 0.15f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Head
    skeleton_template_add_bone(template, "Head", 6,
                             (f32[]){0.0f, 0.1f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Left arm chain
    skeleton_template_add_bone(template, "Left_Clavicle", 5,
                             (f32[]){0.15f, 0.05f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, false);
    
    skeleton_template_add_bone(template, "Left_Arm", 7,
                             (f32[]){0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Left_Forearm", 8,
                             (f32[]){0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Left_Hand", 9,
                             (f32[]){0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Right arm chain
    skeleton_template_add_bone(template, "Right_Clavicle", 5,
                             (f32[]){-0.15f, 0.05f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, false);
    
    skeleton_template_add_bone(template, "Right_Arm", 11,
                             (f32[]){-0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Right_Forearm", 12,
                             (f32[]){-0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Right_Hand", 13,
                             (f32[]){-0.25f, 0.0f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Left leg chain
    skeleton_template_add_bone(template, "Left_Hip", 1,
                             (f32[]){0.1f, -0.05f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Left_Thigh", 15,
                             (f32[]){0.0f, -0.4f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Left_Calf", 16,
                             (f32[]){0.0f, -0.4f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Left_Foot", 17,
                             (f32[]){0.0f, -0.15f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Right leg chain
    skeleton_template_add_bone(template, "Right_Hip", 1,
                             (f32[]){-0.1f, -0.05f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Right_Thigh", 19,
                             (f32[]){0.0f, -0.4f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Right_Calf", 20,
                             (f32[]){0.0f, -0.4f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    skeleton_template_add_bone(template, "Right_Foot", 21,
                             (f32[]){0.0f, -0.15f, 0.0f},
                             (f32[]){0.0f, 0.0f, 0.0f, 1.0f},
                             (f32[]){1.0f, 1.0f, 1.0f}, true);
    
    // Add IK chains
    skeleton_template_add_ik_chain(template, 8, 10); // Left arm
    skeleton_template_add_ik_chain(template, 12, 14); // Right arm
    skeleton_template_add_ik_chain(template, 16, 18); // Left leg
    skeleton_template_add_ik_chain(template, 20, 22); // Right leg
    
    LOG_INFO("Created humanoid skeleton template '%s'", name);
    return template;
}

// Destroy skeleton template
void skeleton_template_destroy(SkeletonTemplate* template) {
    if (!template) return;
    
    if (template->bones) {
        free(template->bones);
        template->bones = NULL;
    }
    
    template->bone_count = 0;
    template->ik_chain_count = 0;
    template->twist_bone_count = 0;
    
    LOG_DEBUG("Destroyed skeleton template");
}

// Public API functions
u32 skeleton_template_get_count(void) {
    return g_skeleton_system.template_count;
}

SkeletonTemplate* skeleton_template_get_by_index(u32 index) {
    if (index >= g_skeleton_system.template_count) {
        return NULL;
    }
    
    return &g_skeleton_system.templates[index];
}

void skeleton_template_cleanup(void) {
    for (u32 i = 0; i < g_skeleton_system.template_count; i++) {
        skeleton_template_destroy(&g_skeleton_system.templates[i]);
    }
    
    memset(&g_skeleton_system, 0, sizeof(SkeletonTemplateSystem));
    LOG_INFO("Skeleton template system cleaned up");
}
