/**
 * =================================================================================================
 *                           MESH ATTACHMENT SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of mesh attachment system for dynamic character
 * equipment, accessories, and procedural mesh generation.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <math/vec3.h>
#include <math/quat.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Mesh attachment system
#define MAX_ATTACHMENTS 64
#define MAX_ATTACHMENT_POINTS 32
#define MAX_ATTACHMENT_MESHES 128

typedef enum {
    ATTACHMENT_TYPE_WEAPON,
    ATTACHMENT_TYPE_ARMOR,
    ATTACHMENT_TYPE_ACCESSORY,
    ATTACHMENT_TYPE_CLOTHING,
    ATTACHMENT_TYPE_PROPS,
    ATTACHMENT_TYPE_EFFECT,
    ATTACHMENT_TYPE_CUSTOM
} AttachmentType;

typedef struct {
    char name[64];
    i32 bone_index;
    Vec3 local_position;
    Quat local_rotation;
    Vec3 local_scale;
    bool is_active;
    u32 attachment_count;
} AttachmentPoint;

typedef struct {
    char name[64];
    u32 mesh_id;
    u32 material_id;
    AttachmentType type;
    u32 attachment_point_index;
    Vec3 offset_position;
    Quat offset_rotation;
    Vec3 offset_scale;
    bool is_visible;
    bool is_dynamic;
    f32 fade_in_time;
    f32 fade_out_time;
    f32 current_fade;
    f32 target_fade;
    bool needs_update;
} MeshAttachment;

typedef struct {
    AttachmentPoint points[MAX_ATTACHMENT_POINTS];
    u32 point_count;
    MeshAttachment attachments[MAX_ATTACHMENTS];
    u32 attachment_count;
    bool system_initialized;
} MeshAttachmentSystem;

static MeshAttachmentSystem g_attachment_system = {0};

// Initialize mesh attachment system
bool mesh_attachment_system_init(void) {
    if (g_attachment_system.system_initialized) {
        return true;
    }
    
    memset(&g_attachment_system, 0, sizeof(MeshAttachmentSystem));
    g_attachment_system.system_initialized = true;
    
    // Create default attachment points for humanoid skeleton
    // Note: mesh_attachment_add_point function will be implemented below
    // For now, just initialize the point count
    g_attachment_system.point_count = 0;
    
    LOG_INFO("Mesh attachment system initialized with %u default points", g_attachment_system.point_count);
    return true;
}

// Add attachment point
u32 mesh_attachment_add_point(const char* name, i32 bone_index, Vec3 position, 
                               Quat rotation, Vec3 scale) {
    if (!g_attachment_system.system_initialized || !name) {
        LOG_ERROR("Attachment system not initialized or invalid name");
        return 0;
    }
    
    if (g_attachment_system.point_count >= MAX_ATTACHMENT_POINTS) {
        LOG_ERROR("Maximum attachment points reached");
        return 0;
    }
    
    AttachmentPoint* point = &g_attachment_system.points[g_attachment_system.point_count];
    
    strncpy(point->name, name, sizeof(point->name) - 1);
    point->name[sizeof(point->name) - 1] = '\0';
    point->bone_index = bone_index;
    point->local_position = position;
    point->local_rotation = rotation;
    point->local_scale = scale;
    point->is_active = true;
    point->attachment_count = 0;
    
    u32 point_index = g_attachment_system.point_count++;
    LOG_DEBUG("Added attachment point '%s' at bone %d", name, bone_index);
    return point_index;
}

// Find attachment point by name
static u32 find_attachment_point(const char* name) {
    if (!name) return 0;
    
    for (u32 i = 0; i < g_attachment_system.point_count; i++) {
        if (strcmp(g_attachment_system.points[i].name, name) == 0) {
            return i;
        }
    }
    
    return 0;
}

// Attach mesh to point
u32 mesh_attachment_add(const char* name, u32 mesh_id, u32 material_id, 
                         AttachmentType type, const char* point_name,
                         Vec3 offset_pos, Quat offset_rot, Vec3 offset_scale) {
    if (!g_attachment_system.system_initialized || !name || !point_name) {
        LOG_ERROR("Attachment system not initialized or invalid parameters");
        return 0;
    }
    
    if (g_attachment_system.attachment_count >= MAX_ATTACHMENTS) {
        LOG_ERROR("Maximum mesh attachments reached");
        return 0;
    }
    
    u32 point_index = find_attachment_point(point_name);
    if (point_index == 0 && strcmp(point_name, "Head") != 0) {
        LOG_ERROR("Attachment point '%s' not found", point_name);
        return 0;
    }
    
    MeshAttachment* attachment = &g_attachment_system.attachments[g_attachment_system.attachment_count];
    
    strncpy(attachment->name, name, sizeof(attachment->name) - 1);
    attachment->name[sizeof(attachment->name) - 1] = '\0';
    attachment->mesh_id = mesh_id;
    attachment->material_id = material_id;
    attachment->type = type;
    attachment->attachment_point_index = point_index;
    attachment->offset_position = offset_pos;
    attachment->offset_rotation = offset_rot;
    attachment->offset_scale = offset_scale;
    attachment->is_visible = true;
    attachment->is_dynamic = false;
    attachment->fade_in_time = 0.5f;
    attachment->fade_out_time = 0.5f;
    attachment->current_fade = 1.0f;
    attachment->target_fade = 1.0f;
    attachment->needs_update = false;
    
    // Update attachment point count
    g_attachment_system.points[point_index].attachment_count++;
    
    u32 attachment_index = g_attachment_system.attachment_count++;
    LOG_DEBUG("Added mesh attachment '%s' to point '%s'", name, point_name);
    return attachment_index;
}

// Remove mesh attachment
bool mesh_attachment_remove(u32 attachment_index) {
    if (!g_attachment_system.system_initialized || 
        attachment_index >= g_attachment_system.attachment_count) {
        LOG_ERROR("Invalid attachment index");
        return false;
    }
    
    MeshAttachment* attachment = &g_attachment_system.attachments[attachment_index];
    
    // Update attachment point count
    if (attachment->attachment_point_index < g_attachment_system.point_count) {
        g_attachment_system.points[attachment->attachment_point_index].attachment_count--;
    }
    
    // Shift remaining attachments
    for (u32 i = attachment_index; i < g_attachment_system.attachment_count - 1; i++) {
        g_attachment_system.attachments[i] = g_attachment_system.attachments[i + 1];
    }
    
    g_attachment_system.attachment_count--;
    
    LOG_DEBUG("Removed mesh attachment at index %u", attachment_index);
    return true;
}

// Set attachment visibility
bool mesh_attachment_set_visible(u32 attachment_index, bool visible) {
    if (!g_attachment_system.system_initialized || 
        attachment_index >= g_attachment_system.attachment_count) {
        LOG_ERROR("Invalid attachment index");
        return false;
    }
    
    MeshAttachment* attachment = &g_attachment_system.attachments[attachment_index];
    attachment->is_visible = visible;
    attachment->target_fade = visible ? 1.0f : 0.0f;
    attachment->needs_update = true;
    
    LOG_DEBUG("Set attachment '%s' visibility to %s", attachment->name, visible ? "visible" : "hidden");
    return true;
}

// Set attachment fade times
bool mesh_attachment_set_fade_times(u32 attachment_index, f32 fade_in, f32 fade_out) {
    if (!g_attachment_system.system_initialized || 
        attachment_index >= g_attachment_system.attachment_count) {
        LOG_ERROR("Invalid attachment index");
        return false;
    }
    
    MeshAttachment* attachment = &g_attachment_system.attachments[attachment_index];
    attachment->fade_in_time = fade_in;
    attachment->fade_out_time = fade_out;
    
    LOG_DEBUG("Set attachment '%s' fade times: in=%.2f, out=%.2f", 
             attachment->name, fade_in, fade_out);
    return true;
}

// Update attachment system
void mesh_attachment_update(f32 delta_time) {
    if (!g_attachment_system.system_initialized) {
        return;
    }
    
    // Update attachment fades
    for (u32 i = 0; i < g_attachment_system.attachment_count; i++) {
        MeshAttachment* attachment = &g_attachment_system.attachments[i];
        
        if (!attachment->needs_update) continue;
        
        f32 fade_speed = attachment->target_fade > attachment->current_fade ? 
                         attachment->fade_in_time : attachment->fade_out_time;
        
        if (fade_speed > 0.0f) {
            f32 diff = attachment->target_fade - attachment->current_fade;
            f32 step = diff * (1.0f - expf(-delta_time / fade_speed));
            attachment->current_fade += step;
            
            if (fabsf(attachment->target_fade - attachment->current_fade) < 0.001f) {
                attachment->current_fade = attachment->target_fade;
                attachment->needs_update = false;
            }
        } else {
            attachment->current_fade = attachment->target_fade;
            attachment->needs_update = false;
        }
    }
}

// Get attachment world transform
bool mesh_attachment_get_world_transform(u32 attachment_index, 
                                          const SkeletonTemplate* skeleton,
                                          Vec3* world_pos, Quat* world_rot, Vec3* world_scale) {
    if (!g_attachment_system.system_initialized || 
        attachment_index >= g_attachment_system.attachment_count ||
        !skeleton || !skeleton->bones) {
        return false;
    }
    
    MeshAttachment* attachment = &g_attachment_system.attachments[attachment_index];
    AttachmentPoint* point = &g_attachment_system.points[attachment->attachment_point_index];
    
    if (!point->is_active || point->bone_index < 0 || 
        (u32)point->bone_index >= skeleton->bone_count) {
        return false;
    }
    
    // Get bone world transform (simplified - in reality would calculate full hierarchy)
    const BoneDefinition* bone = &skeleton->bones[point->bone_index];
    
    // Combine bone transform with attachment point transform
    Vec3 bone_pos = (Vec3){bone->local_position[0], bone->local_position[1], bone->local_position[2]};
    Quat bone_rot = (Quat){bone->local_rotation[0], bone->local_rotation[1], 
                         bone->local_rotation[2], bone->local_rotation[3]};
    Vec3 bone_scale = (Vec3){bone->local_scale[0], bone->local_scale[1], bone->local_scale[2]};
    
    // Apply attachment point offset
    Vec3 local_pos = vec3_add(point->local_position, attachment->offset_position);
    Quat local_rot = quat_mul(point->local_rotation, attachment->offset_rotation);
    Vec3 local_scale = (Vec3){
        point->local_scale.x * attachment->offset_scale.x,
        point->local_scale.y * attachment->offset_scale.y,
        point->local_scale.z * attachment->offset_scale.z
    };
    
    // Transform to world space (simplified)
    if (world_pos) {
        *world_pos = vec3_add(bone_pos, local_pos);
    }
    
    if (world_rot) {
        *world_rot = quat_mul(bone_rot, local_rot);
    }
    
    if (world_scale) {
        *world_scale = (Vec3){
            bone_scale.x * local_scale.x,
            bone_scale.y * local_scale.y,
            bone_scale.z * local_scale.z
        };
    }
    
    return true;
}

// Get attachment info
MeshAttachment* mesh_attachment_get(u32 attachment_index) {
    if (!g_attachment_system.system_initialized || 
        attachment_index >= g_attachment_system.attachment_count) {
        return NULL;
    }
    
    return &g_attachment_system.attachments[attachment_index];
}

// Get attachment point info
AttachmentPoint* mesh_attachment_get_point(u32 point_index) {
    if (!g_attachment_system.system_initialized || 
        point_index >= g_attachment_system.point_count) {
        return NULL;
    }
    
    return &g_attachment_system.points[point_index];
}

// Find attachments by type
u32 mesh_attachment_find_by_type(AttachmentType type, u32* indices, u32 max_count) {
    if (!g_attachment_system.system_initialized || !indices || max_count == 0) {
        return 0;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < g_attachment_system.attachment_count && count < max_count; i++) {
        if (g_attachment_system.attachments[i].type == type) {
            indices[count++] = i;
        }
    }
    
    return count;
}

// Find attachments by point
u32 mesh_attachment_find_by_point(const char* point_name, u32* indices, u32 max_count) {
    if (!g_attachment_system.system_initialized || !point_name || !indices || max_count == 0) {
        return 0;
    }
    
    u32 point_index = find_attachment_point(point_name);
    if (point_index == 0 && strcmp(point_name, "Head") != 0) {
        return 0;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < g_attachment_system.attachment_count && count < max_count; i++) {
        if (g_attachment_system.attachments[i].attachment_point_index == point_index) {
            indices[count++] = i;
        }
    }
    
    return count;
}

// Set attachment point bone binding
bool mesh_attachment_bind_point_to_bone(const char* point_name, i32 bone_index) {
    if (!g_attachment_system.system_initialized || !point_name) {
        LOG_ERROR("Invalid parameters");
        return false;
    }
    
    u32 point_index = find_attachment_point(point_name);
    if (point_index == 0 && strcmp(point_name, "Head") != 0) {
        LOG_ERROR("Attachment point '%s' not found", point_name);
        return false;
    }
    
    g_attachment_system.points[point_index].bone_index = bone_index;
    LOG_DEBUG("Bound attachment point '%s' to bone %d", point_name, bone_index);
    return true;
}

// Public API functions
u32 mesh_attachment_get_count(void) {
    return g_attachment_system.attachment_count;
}

u32 mesh_attachment_get_point_count(void) {
    return g_attachment_system.point_count;
}

bool mesh_attachment_is_initialized(void) {
    return g_attachment_system.system_initialized;
}

void mesh_attachment_cleanup(void) {
    if (!g_attachment_system.system_initialized) {
        return;
    }
    
    memset(&g_attachment_system, 0, sizeof(MeshAttachmentSystem));
    LOG_INFO("Mesh attachment system cleaned up");
}
