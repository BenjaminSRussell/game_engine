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
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <core/types.h>
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
                               i32 parent_index, const f32 position[3], 
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
bool skeleton_template_add_ik_chain(SkeletonTemplate* template, i32 start_bone, i32 end_bone) {
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
bool skeleton_template_add_twist_bone(SkeletonTemplate* template, i32 bone_index) {
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
i32 skeleton_template_get_bone_index(const SkeletonTemplate* template, const char* bone_name) {
    if (!template || !bone_name || !template->bones) {
        return -1;
    }
    
    for (u32 i = 0; i < template->bone_count; i++) {
        if (strcmp(template->bones[i].name, bone_name) == 0) {
            return (i32)i;
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

// Skeleton validation system
typedef enum {
    SKELETON_VALID = 0,
    SKELETON_INVALID_ROOT,
    SKELETON_INVALID_PARENT,
    SKELETON_CIRCULAR_REFERENCE,
    SKELETON_MISSING_ESSENTIAL_BONES,
    SKELETON_INVALID_IK_CHAIN,
    SKELETON_INVALID_TWIST_BONES,
    SKELETON_TOO_MANY_BONES,
    SKELETON_INVALID_BONE_DATA
} SkeletonValidationError;

typedef struct {
    SkeletonValidationError error_type;
    u32 bone_index;
    char error_message[256];
} ValidationIssue;

typedef struct {
    ValidationIssue issues[32];
    u32 issue_count;
    bool is_valid;
} SkeletonValidationResult;

// Validate bone hierarchy
static bool validate_bone_hierarchy(const SkeletonTemplate* template, 
                                    SkeletonValidationResult* result) {
    if (!template || !result || !template->bones) {
        return false;
    }
    
    bool valid = true;
    
    // Check for root bone
    bool has_root = false;
    for (u32 i = 0; i < template->bone_count; i++) {
        if (template->bones[i].parent_index == -1) {
            has_root = true;
            break;
        }
    }
    
    if (!has_root) {
        ValidationIssue* issue = &result->issues[result->issue_count++];
        issue->error_type = SKELETON_INVALID_ROOT;
        issue->bone_index = -1;
        strncpy(issue->error_message, "Skeleton missing root bone", sizeof(issue->error_message) - 1);
        valid = false;
    }
    
    // Check parent indices and detect circular references
    for (u32 i = 0; i < template->bone_count; i++) {
        i32 parent = template->bones[i].parent_index;
        
        if (parent >= 0 && (u32)parent >= template->bone_count) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_PARENT;
            issue->bone_index = i;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "Bone '%s' has invalid parent index %d", template->bones[i].name, parent);
            valid = false;
            continue;
        }
        
        // Check for circular references by following parent chain
        u32 current = i;
        u32 depth = 0;
        while (template->bones[current].parent_index != -1 && depth < template->bone_count) {
            current = (u32)template->bones[current].parent_index;
            depth++;
            
            if (current == i) {
                ValidationIssue* issue = &result->issues[result->issue_count++];
                issue->error_type = SKELETON_CIRCULAR_REFERENCE;
                issue->bone_index = i;
                snprintf(issue->error_message, sizeof(issue->error_message), 
                        "Circular reference detected in bone '%s'", template->bones[i].name);
                valid = false;
                break;
            }
        }
    }
    
    return valid;
}

// Validate IK chains
static bool validate_ik_chains(const SkeletonTemplate* template, 
                               SkeletonValidationResult* result) {
    if (!template || !result) {
        return false;
    }
    
    bool valid = true;
    
    for (u32 i = 0; i < template->ik_chain_count; i++) {
        i32 start = template->ik_chains[i].start;
        i32 end = template->ik_chains[i].end;
        
        // Validate bone indices
        if (start < 0 || (u32)start >= template->bone_count) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_IK_CHAIN;
            issue->bone_index = start;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "IK chain %u has invalid start bone index %d", i, start);
            valid = false;
        }
        
        if (end < 0 || (u32)end >= template->bone_count) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_IK_CHAIN;
            issue->bone_index = end;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "IK chain %u has invalid end bone index %d", i, end);
            valid = false;
        }
        
        // Check if end bone is descendant of start bone
        if (start >= 0 && end >= 0) {
            bool is_descendant = false;
            u32 current = (u32)end;
            while (template->bones[current].parent_index != -1) {
                current = (u32)template->bones[current].parent_index;
                if (current == (u32)start) {
                    is_descendant = true;
                    break;
                }
            }
            
            if (!is_descendant) {
                ValidationIssue* issue = &result->issues[result->issue_count++];
                issue->error_type = SKELETON_INVALID_IK_CHAIN;
                issue->bone_index = start;
                snprintf(issue->error_message, sizeof(issue->error_message), 
                        "IK chain %u: end bone is not descendant of start bone", i);
                valid = false;
            }
        }
    }
    
    return valid;
}

// Validate twist bones
static bool validate_twist_bones(const SkeletonTemplate* template, 
                                 SkeletonValidationResult* result) {
    if (!template || !result) {
        return false;
    }
    
    bool valid = true;
    
    for (u32 i = 0; i < template->twist_bone_count; i++) {
        i32 bone_index = template->twist_bone_indices[i];
        
        // Validate bone index
        if (bone_index < 0 || (u32)bone_index >= template->bone_count) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_TWIST_BONES;
            issue->bone_index = bone_index;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "Twist bone %u has invalid bone index %d", i, bone_index);
            valid = false;
        }
    }
    
    return valid;
}

// Validate bone data integrity
static bool validate_bone_data(const SkeletonTemplate* template, 
                               SkeletonValidationResult* result) {
    if (!template || !result || !template->bones) {
        return false;
    }
    
    bool valid = true;
    
    for (u32 i = 0; i < template->bone_count; i++) {
        const BoneDefinition* bone = &template->bones[i];
        
        // Check bone name
        if (bone->name[0] == '\0') {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_BONE_DATA;
            issue->bone_index = i;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "Bone %u has empty name", i);
            valid = false;
        }
        
        // Check for valid quaternion
        f32 quat_length = sqrtf(bone->local_rotation[0] * bone->local_rotation[0] +
                               bone->local_rotation[1] * bone->local_rotation[1] +
                               bone->local_rotation[2] * bone->local_rotation[2] +
                               bone->local_rotation[3] * bone->local_rotation[3]);
        
        if (quat_length < 0.9f || quat_length > 1.1f) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_BONE_DATA;
            issue->bone_index = i;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "Bone '%s' has invalid quaternion (length: %.3f)", bone->name, quat_length);
            valid = false;
        }
        
        // Check scale values
        if (bone->local_scale[0] <= 0.0f || bone->local_scale[1] <= 0.0f || bone->local_scale[2] <= 0.0f) {
            ValidationIssue* issue = &result->issues[result->issue_count++];
            issue->error_type = SKELETON_INVALID_BONE_DATA;
            issue->bone_index = i;
            snprintf(issue->error_message, sizeof(issue->error_message), 
                    "Bone '%s' has invalid scale values", bone->name);
            valid = false;
        }
    }
    
    return valid;
}

// Main skeleton validation function
SkeletonValidationResult skeleton_template_validate(const SkeletonTemplate* template) {
    SkeletonValidationResult result = {0};
    result.is_valid = true;
    
    if (!template) {
        ValidationIssue* issue = &result.issues[result.issue_count++];
        issue->error_type = SKELETON_INVALID_BONE_DATA;
        issue->bone_index = -1;
        strncpy(issue->error_message, "Null template provided", sizeof(issue->error_message) - 1);
        result.is_valid = false;
        return result;
    }
    
    if (!template->bones || template->bone_count == 0) {
        ValidationIssue* issue = &result.issues[result.issue_count++];
        issue->error_type = SKELETON_INVALID_BONE_DATA;
        issue->bone_index = -1;
        strncpy(issue->error_message, "Template has no bones", sizeof(issue->error_message) - 1);
        result.is_valid = false;
        return result;
    }
    
    if (template->bone_count > MAX_BONES_PER_SKELETON) {
        ValidationIssue* issue = &result.issues[result.issue_count++];
        issue->error_type = SKELETON_TOO_MANY_BONES;
        issue->bone_index = -1;
        snprintf(issue->error_message, sizeof(issue->error_message), 
                "Template has too many bones: %u (max: %u)", template->bone_count, MAX_BONES_PER_SKELETON);
        result.is_valid = false;
    }
    
    // Run all validation checks
    bool hierarchy_valid = validate_bone_hierarchy(template, &result);
    bool ik_valid = validate_ik_chains(template, &result);
    bool twist_valid = validate_twist_bones(template, &result);
    bool data_valid = validate_bone_data(template, &result);
    
    result.is_valid = result.is_valid && hierarchy_valid && ik_valid && twist_valid && data_valid;
    
    // Log validation results
    if (result.is_valid) {
        LOG_INFO("Skeleton template '%s' validation passed", template->name);
    } else {
        LOG_ERROR("Skeleton template '%s' validation failed with %u issues", 
                 template->name, result.issue_count);
        for (u32 i = 0; i < result.issue_count; i++) {
            LOG_ERROR("  Issue %u: %s", i + 1, result.issues[i].error_message);
        }
    }
    
    return result;
}

// Quick validation check (returns only valid/invalid)
bool skeleton_template_is_valid(const SkeletonTemplate* template) {
    SkeletonValidationResult result = skeleton_template_validate(template);
    return result.is_valid;
}

// Get validation error count
u32 skeleton_template_get_validation_error_count(const SkeletonTemplate* template) {
    SkeletonValidationResult result = skeleton_template_validate(template);
    return result.issue_count;
}

// Public API for skeleton validation
bool skeleton_template_validate_all(void) {
    bool all_valid = true;
    
    for (u32 i = 0; i < g_skeleton_system.template_count; i++) {
        SkeletonValidationResult result = skeleton_template_validate(&g_skeleton_system.templates[i]);
        if (!result.is_valid) {
            all_valid = false;
        }
    }
    
    LOG_INFO("Skeleton template validation complete: %s", all_valid ? "All valid" : "Some invalid");
    return all_valid;
}

// Skeleton retargeting system
typedef struct {
    i32 source_bone_index;
    i32 target_bone_index;
    f32 weight;
    bool is_essential;
} BoneMapping;

typedef struct {
    BoneMapping mappings[MAX_BONES_PER_SKELETON];
    u32 mapping_count;
    f32 overall_quality;
    bool is_retargetable;
} RetargetingData;

typedef struct {
    char source_bone[32];
    char target_bone[32];
    f32 weight;
} BoneMappingRule;

// Common bone mapping rules for humanoid skeletons
static const BoneMappingRule k_humanoid_mapping_rules[] = {
    {"Root", "Root", 1.0f},
    {"Pelvis", "Pelvis", 1.0f},
    {"Hips", "Pelvis", 1.0f},
    {"Spine", "Spine_1", 0.8f},
    {"Spine1", "Spine_1", 1.0f},
    {"Spine2", "Spine_2", 1.0f},
    {"Spine_3", "Spine_3", 1.0f},
    {"Chest", "Chest", 1.0f},
    {"UpperChest", "Chest", 0.9f},
    {"Neck", "Neck", 1.0f},
    {"Head", "Head", 1.0f},
    {"LeftShoulder", "Left_Clavicle", 0.8f},
    {"LeftArm", "Left_Arm", 1.0f},
    {"LeftForeArm", "Left_Forearm", 1.0f},
    {"LeftHand", "Left_Hand", 1.0f},
    {"RightShoulder", "Right_Clavicle", 0.8f},
    {"RightArm", "Right_Arm", 1.0f},
    {"RightForeArm", "Right_Forearm", 1.0f},
    {"RightHand", "Right_Hand", 1.0f},
    {"LeftUpLeg", "Left_Hip", 0.9f},
    {"LeftThigh", "Left_Thigh", 1.0f},
    {"LeftLeg", "Left_Calf", 0.9f},
    {"LeftShin", "Left_Calf", 1.0f},
    {"LeftFoot", "Left_Foot", 1.0f},
    {"LeftToeBase", "Left_Foot", 0.8f},
    {"RightUpLeg", "Right_Hip", 0.9f},
    {"RightThigh", "Right_Thigh", 1.0f},
    {"RightLeg", "Right_Calf", 0.9f},
    {"RightShin", "Right_Calf", 1.0f},
    {"RightFoot", "Right_Foot", 1.0f},
    {"RightToeBase", "Right_Foot", 0.8f},
};

// Find bone index by name (case-insensitive)
static i32 find_bone_index(const SkeletonTemplate* template, const char* bone_name) {
    if (!template || !bone_name || !template->bones) {
        return -1;
    }
    
    for (u32 i = 0; i < template->bone_count; i++) {
        if (strcasecmp(template->bones[i].name, bone_name) == 0) {
            return (i32)i;
        }
    }
    
    return -1;
}

// Calculate bone similarity based on position and hierarchy
static f32 calculate_bone_similarity(const SkeletonTemplate* source, u32 source_index,
                                   const SkeletonTemplate* target, u32 target_index) {
    if (!source || !target || source_index >= source->bone_count || target_index >= target->bone_count) {
        return 0.0f;
    }
    
    const BoneDefinition* source_bone = &source->bones[source_index];
    const BoneDefinition* target_bone = &target->bones[target_index];
    
    f32 similarity = 0.0f;
    
    // Name similarity (case-insensitive)
    if (strcasecmp(source_bone->name, target_bone->name) == 0) {
        similarity += 0.5f;
    } else {
        // Partial name match
        if (strstr(source_bone->name, target_bone->name) || strstr(target_bone->name, source_bone->name)) {
            similarity += 0.2f;
        }
    }
    
    // Position similarity
    f32 pos_diff = fabsf(source_bone->local_position[0] - target_bone->local_position[0]) +
                    fabsf(source_bone->local_position[1] - target_bone->local_position[1]) +
                    fabsf(source_bone->local_position[2] - target_bone->local_position[2]);
    f32 pos_similarity = fmaxf(0.0f, 1.0f - pos_diff * 0.1f);
    similarity += pos_similarity * 0.3f;
    
    // Essential bone bonus
    if (source_bone->is_essential && target_bone->is_essential) {
        similarity += 0.2f;
    }
    
    return fminf(1.0f, similarity);
}

// Create bone mapping between skeletons
static RetargetingData create_bone_mapping(const SkeletonTemplate* source, 
                                           const SkeletonTemplate* target) {
    RetargetingData mapping = {0};
    mapping.overall_quality = 0.0f;
    mapping.is_retargetable = false;
    
    if (!source || !target || !source->bones || !target->bones) {
        return mapping;
    }
    
    // First, try exact name matches and rule-based matches
    u32 rule_matches = 0;
    for (u32 i = 0; i < sizeof(k_humanoid_mapping_rules) / sizeof(k_humanoid_mapping_rules[0]); i++) {
        const BoneMappingRule* rule = &k_humanoid_mapping_rules[i];
        
        i32 source_index = find_bone_index(source, rule->source_bone);
        i32 target_index = find_bone_index(target, rule->target_bone);
        
        if (source_index >= 0 && target_index >= 0) {
            if (mapping.mapping_count < MAX_BONES_PER_SKELETON) {
                BoneMapping* bone_map = &mapping.mappings[mapping.mapping_count++];
                bone_map->source_bone_index = source_index;
                bone_map->target_bone_index = target_index;
                bone_map->weight = rule->weight;
                bone_map->is_essential = source->bones[source_index].is_essential;
                rule_matches++;
            }
        }
    }
    
    // Then, find additional matches based on similarity
    for (u32 source_idx = 0; source_idx < source->bone_count && mapping.mapping_count < MAX_BONES_PER_SKELETON; source_idx++) {
        // Skip if already mapped
        bool already_mapped = false;
        for (u32 j = 0; j < mapping.mapping_count; j++) {
            if (mapping.mappings[j].source_bone_index == (i32)source_idx) {
                already_mapped = true;
                break;
            }
        }
        
        if (already_mapped) continue;
        
        // Find best match in target
        f32 best_similarity = 0.0f;
        i32 best_target_index = -1;
        
        for (u32 target_idx = 0; target_idx < target->bone_count; target_idx++) {
            f32 similarity = calculate_bone_similarity(source, source_idx, target, target_idx);
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_target_index = (i32)target_idx;
            }
        }
        
        // Add mapping if similarity is good enough
        if (best_similarity > 0.3f && best_target_index >= 0) {
            BoneMapping* bone_map = &mapping.mappings[mapping.mapping_count++];
            bone_map->source_bone_index = (i32)source_idx;
            bone_map->target_bone_index = best_target_index;
            bone_map->weight = best_similarity;
            bone_map->is_essential = source->bones[source_idx].is_essential;
        }
    }
    
    // Calculate overall quality
    f32 total_weight = 0.0f;
    f32 essential_mapped = 0.0f;
    u32 essential_count = 0;
    
    for (u32 i = 0; i < mapping.mapping_count; i++) {
        total_weight += mapping.mappings[i].weight;
        if (mapping.mappings[i].is_essential) {
            essential_mapped += mapping.mappings[i].weight;
            essential_count++;
        }
    }
    
    u32 source_essential_count = 0;
    for (u32 i = 0; i < source->bone_count; i++) {
        if (source->bones[i].is_essential) {
            source_essential_count++;
        }
    }
    
    mapping.overall_quality = total_weight / (f32)source->bone_count;
    
    // Check if retargeting is viable
    if (essential_count > 0) {
        f32 essential_ratio = essential_mapped / (f32)essential_count;
        mapping.is_retargetable = (essential_ratio >= 0.7f && mapping.overall_quality >= 0.5f);
    } else {
        mapping.is_retargetable = (mapping.overall_quality >= 0.6f);
    }
    
    LOG_DEBUG("Retargeting analysis: %u mappings, quality=%.2f, essential=%.2f/%u, retargetable=%s",
             mapping.mapping_count, mapping.overall_quality, essential_mapped, essential_count,
             mapping.is_retargetable ? "YES" : "NO");
    
    return mapping;
}

// Apply retargeting to animation data
static bool apply_retargeting(const SkeletonTemplate* source, const SkeletonTemplate* target,
                             const RetargetingData* mapping, const f32* source_anim_data,
                             f32* target_anim_data, u32 bone_count) {
    if (!source || !target || !mapping || !source_anim_data || !target_anim_data) {
        return false;
    }
    
    // Initialize target animation data to identity
    for (u32 i = 0; i < bone_count * 16; i++) { // 16 floats per bone (4x4 matrix)
        target_anim_data[i] = (i % 5 == 0) ? 1.0f : 0.0f; // Identity matrix
    }
    
    // Apply mappings
    for (u32 i = 0; i < mapping->mapping_count; i++) {
        const BoneMapping* bone_map = &mapping->mappings[i];
        
        if (bone_map->source_bone_index >= 0 && bone_map->target_bone_index >= 0 &&
            (u32)bone_map->source_bone_index < source->bone_count &&
            (u32)bone_map->target_bone_index < target->bone_count) {
            
            // Copy animation data with weight
            u32 source_offset = (u32)bone_map->source_bone_index * 16;
            u32 target_offset = (u32)bone_map->target_bone_index * 16;
            
            for (u32 j = 0; j < 16; j++) {
                target_anim_data[target_offset + j] = source_anim_data[source_offset + j] * bone_map->weight;
            }
        }
    }
    
    return true;
}

// Public API for skeleton retargeting
RetargetingData skeleton_create_retargeting(const SkeletonTemplate* source, 
                                            const SkeletonTemplate* target) {
    return create_bone_mapping(source, target);
}

bool skeleton_retarget_animation(const SkeletonTemplate* source, const SkeletonTemplate* target,
                                const RetargetingData* mapping, const f32* source_anim_data,
                                f32* target_anim_data, u32 bone_count) {
    return apply_retargeting(source, target, mapping, source_anim_data, target_anim_data, bone_count);
}

bool skeleton_can_retarget(const SkeletonTemplate* source, const SkeletonTemplate* target) {
    RetargetingData mapping = create_bone_mapping(source, target);
    return mapping.is_retargetable;
}

f32 skeleton_get_retargeting_quality(const SkeletonTemplate* source, const SkeletonTemplate* target) {
    RetargetingData mapping = create_bone_mapping(source, target);
    return mapping.overall_quality;
}

// Export retargeting data to file
bool skeleton_export_retargeting_data(const RetargetingData* mapping, const char* file_path) {
    if (!mapping || !file_path) {
        return false;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open retargeting data file: %s", file_path);
        return false;
    }
    
    fprintf(file, "# Skeleton Retargeting Data\n");
    fprintf(file, "# Generated automatically\n\n");
    fprintf(file, "quality: %.3f\n", mapping->overall_quality);
    fprintf(file, "is_retargetable: %s\n", mapping->is_retargetable ? "true" : "false");
    fprintf(file, "mapping_count: %u\n\n", mapping->mapping_count);
    
    fprintf(file, "# Bone Mappings\n");
    fprintf(file, "# source_index, target_index, weight, is_essential\n");
    
    for (u32 i = 0; i < mapping->mapping_count; i++) {
        const BoneMapping* bone_map = &mapping->mappings[i];
        fprintf(file, "%d, %d, %.3f, %s\n",
               bone_map->source_bone_index, bone_map->target_bone_index,
               bone_map->weight, bone_map->is_essential ? "true" : "false");
    }
    
    fclose(file);
    LOG_INFO("Exported retargeting data to: %s", file_path);
    return true;
}
