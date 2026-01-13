/**
 * =================================================================================================
 *                           SKELETON PREVIEW VISUALIZATION
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of skeleton preview visualization system for debugging
 * and character template inspection with real-time rendering.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Skeleton preview visualization system
#define MAX_PREVIEW_VERTICES 4096
#define MAX_PREVIEW_INDICES 8192
#define BONE_SPHERE_SEGMENTS 8
#define BONE_CYLINDER_SEGMENTS 8

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec3 color;
} PreviewVertex;

typedef struct {
    PreviewVertex vertices[MAX_PREVIEW_VERTICES];
    u32 vertex_count;
    u32 indices[MAX_PREVIEW_INDICES];
    u32 index_count;
    bool is_valid;
} PreviewMesh;

typedef struct {
    PreviewMesh bone_mesh;
    PreviewMesh joint_mesh;
    PreviewMesh ik_chain_mesh;
    PreviewMesh twist_bone_mesh;
    Vec3 skeleton_color;
    Vec3 essential_color;
    Vec3 ik_color;
    Vec3 twist_color;
    f32 bone_thickness;
    f32 joint_radius;
    bool show_bones;
    bool show_joints;
    bool show_ik_chains;
    bool show_twist_bones;
    bool show_labels;
    bool is_active;
} SkeletonPreview;

typedef struct {
    SkeletonPreview previews[MAX_SKELETON_TEMPLATES];
    u32 preview_count;
    bool system_initialized;
} SkeletonPreviewSystem;

static SkeletonPreviewSystem g_preview_system = {0};

// Initialize skeleton preview system
bool skeleton_preview_system_init(void) {
    if (g_preview_system.system_initialized) {
        return true;
    }
    
    memset(&g_preview_system, 0, sizeof(SkeletonPreviewSystem));
    g_preview_system.system_initialized = true;
    
    LOG_INFO("Skeleton preview system initialized");
    return true;
}

// Create sphere geometry for joints
static void create_sphere_geometry(PreviewMesh* mesh, f32 radius, u32 segments) {
    if (!mesh || segments < 3) return;
    
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    
    // Generate sphere vertices
    for (u32 lat = 0; lat <= segments; lat++) {
        f32 theta = (f32)lat * PI / segments;
        f32 sin_theta = sinf(theta);
        f32 cos_theta = cosf(theta);
        
        for (u32 lon = 0; lon <= segments; lon++) {
            f32 phi = (f32)lon * 2.0f * PI / segments;
            f32 sin_phi = sinf(phi);
            f32 cos_phi = cosf(phi);
            
            PreviewVertex vertex = {0};
            vertex.position.x = radius * sin_theta * cos_phi;
            vertex.position.y = radius * cos_theta;
            vertex.position.z = radius * sin_theta * sin_phi;
            
            // Normal is same as position for unit sphere
            vertex.normal = vertex.position;
            vertex.normal = vec3_normalize(vertex.normal);
            
            vertex.color = (Vec3){1.0f, 1.0f, 1.0f};
            
            if (mesh->vertex_count < MAX_PREVIEW_VERTICES) {
                mesh->vertices[mesh->vertex_count++] = vertex;
            }
        }
    }
    
    // Generate sphere indices
    for (u32 lat = 0; lat < segments; lat++) {
        for (u32 lon = 0; lon < segments; lon++) {
            u32 first = lat * (segments + 1) + lon;
            u32 second = first + segments + 1;
            
            if (mesh->index_count + 6 <= MAX_PREVIEW_INDICES) {
                mesh->indices[mesh->index_count++] = first;
                mesh->indices[mesh->index_count++] = second;
                mesh->indices[mesh->index_count++] = first + 1;
                
                mesh->indices[mesh->index_count++] = second;
                mesh->indices[mesh->index_count++] = second + 1;
                mesh->indices[mesh->index_count++] = first + 1;
            }
        }
    }
    
    mesh->is_valid = true;
}

// Create cylinder geometry for bones
static void create_cylinder_geometry(PreviewMesh* mesh, f32 radius, f32 height, u32 segments) {
    if (!mesh || segments < 3) return;
    
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    
    // Generate cylinder vertices
    for (u32 i = 0; i <= segments; i++) {
        f32 angle = (f32)i * 2.0f * PI / segments;
        f32 cos_angle = cosf(angle);
        f32 sin_angle = sinf(angle);
        
        // Bottom vertex
        PreviewVertex bottom_vertex = {0};
        bottom_vertex.position.x = radius * cos_angle;
        bottom_vertex.position.y = -height * 0.5f;
        bottom_vertex.position.z = radius * sin_angle;
        bottom_vertex.normal = (Vec3){cos_angle, 0.0f, sin_angle};
        bottom_vertex.color = (Vec3){1.0f, 1.0f, 1.0f};
        
        if (mesh->vertex_count < MAX_PREVIEW_VERTICES) {
            mesh->vertices[mesh->vertex_count++] = bottom_vertex;
        }
        
        // Top vertex
        PreviewVertex top_vertex = {0};
        top_vertex.position.x = radius * cos_angle;
        top_vertex.position.y = height * 0.5f;
        top_vertex.position.z = radius * sin_angle;
        top_vertex.normal = (Vec3){cos_angle, 0.0f, sin_angle};
        top_vertex.color = (Vec3){1.0f, 1.0f, 1.0f};
        
        if (mesh->vertex_count < MAX_PREVIEW_VERTICES) {
            mesh->vertices[mesh->vertex_count++] = top_vertex;
        }
    }
    
    // Generate cylinder indices
    for (u32 i = 0; i < segments; i++) {
        u32 bottom1 = i * 2;
        u32 top1 = bottom1 + 1;
        u32 bottom2 = ((i + 1) % segments) * 2;
        u32 top2 = bottom2 + 1;
        
        if (mesh->index_count + 12 <= MAX_PREVIEW_INDICES) {
            // Side faces
            mesh->indices[mesh->index_count++] = bottom1;
            mesh->indices[mesh->index_count++] = top1;
            mesh->indices[mesh->index_count++] = bottom2;
            
            mesh->indices[mesh->index_count++] = top1;
            mesh->indices[mesh->index_count++] = top2;
            mesh->indices[mesh->index_count++] = bottom2;
        }
    }
    
    mesh->is_valid = true;
}

// Create line geometry for IK chains
static void create_line_geometry(PreviewMesh* mesh, const Vec3* points, u32 point_count) {
    if (!mesh || !points || point_count < 2) return;
    
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    
    // Create vertices for line segments
    for (u32 i = 0; i < point_count && mesh->vertex_count < MAX_PREVIEW_VERTICES; i++) {
        PreviewVertex vertex = {0};
        vertex.position = points[i];
        vertex.normal = (Vec3){0.0f, 1.0f, 0.0f};
        vertex.color = (Vec3){1.0f, 0.0f, 0.0f};
        mesh->vertices[mesh->vertex_count++] = vertex;
    }
    
    // Create line indices
    for (u32 i = 0; i < point_count - 1 && mesh->index_count + 2 <= MAX_PREVIEW_INDICES; i++) {
        mesh->indices[mesh->index_count++] = i;
        mesh->indices[mesh->index_count++] = i + 1;
    }
    
    mesh->is_valid = true;
}

// Initialize preview meshes
static void init_preview_meshes(SkeletonPreview* preview) {
    if (!preview) return;
    
    // Create sphere for joints
    create_sphere_geometry(&preview->joint_mesh, preview->joint_radius, BONE_SPHERE_SEGMENTS);
    
    // Create cylinder for bones
    create_cylinder_geometry(&preview->bone_mesh, preview->bone_thickness, 1.0f, BONE_CYLINDER_SEGMENTS);
    
    // Create line geometry for IK chains (placeholder)
    Vec3 line_points[2] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    create_line_geometry(&preview->ik_chain_mesh, line_points, 2);
    
    // Copy for twist bones
    preview->twist_bone_mesh = preview->ik_chain_mesh;
    
    LOG_DEBUG("Initialized preview meshes");
}

// Create skeleton preview
SkeletonPreview* skeleton_create_preview(const SkeletonTemplate* template) {
    if (!template || !template->bones) {
        LOG_ERROR("Cannot create preview for null skeleton template");
        return NULL;
    }
    
    if (g_preview_system.preview_count >= MAX_SKELETON_TEMPLATES) {
        LOG_ERROR("Maximum skeleton previews reached");
        return NULL;
    }
    
    SkeletonPreview* preview = &g_preview_system.previews[g_preview_system.preview_count];
    
    // Initialize preview settings
    preview->skeleton_color = (Vec3){0.7f, 0.7f, 0.7f};
    preview->essential_color = (Vec3){1.0f, 1.0f, 0.0f};
    preview->ik_color = (Vec3){0.0f, 1.0f, 0.0f};
    preview->twist_color = (Vec3){0.0f, 0.5f, 1.0f};
    preview->bone_thickness = 0.02f;
    preview->joint_radius = 0.05f;
    preview->show_bones = true;
    preview->show_joints = true;
    preview->show_ik_chains = true;
    preview->show_twist_bones = true;
    preview->show_labels = false;
    preview->is_active = true;
    
    // Initialize meshes
    init_preview_meshes(preview);
    
    g_preview_system.preview_count++;
    
    LOG_INFO("Created skeleton preview for '%s'", template->name);
    return preview;
}

// Generate preview geometry for skeleton
static void generate_preview_geometry(SkeletonPreview* preview, const SkeletonTemplate* template) {
    if (!preview || !template || !template->bones) return;
    
    // This would generate the actual preview geometry based on the skeleton
    // For now, we'll just log the process
    LOG_DEBUG("Generating preview geometry for skeleton '%s'", template->name);
    
    // In a full implementation, this would:
    // 1. Create bone cylinders between parent and child bones
    // 2. Place joint spheres at bone positions
    // 3. Create line geometry for IK chains
    // 4. Create special geometry for twist bones
    // 5. Apply colors based on bone importance
}

// Update skeleton preview
void skeleton_update_preview(SkeletonPreview* preview, const SkeletonTemplate* template) {
    if (!preview || !template || !preview->is_active) return;
    
    generate_preview_geometry(preview, template);
}

// Set preview colors
void skeleton_set_preview_colors(SkeletonPreview* preview, Vec3 skeleton, Vec3 essential, 
                                  Vec3 ik, Vec3 twist) {
    if (!preview) return;
    
    preview->skeleton_color = skeleton;
    preview->essential_color = essential;
    preview->ik_color = ik;
    preview->twist_color = twist;
    
    LOG_DEBUG("Updated preview colors");
}

// Set preview visibility
void skeleton_set_preview_visibility(SkeletonPreview* preview, bool bones, bool joints, 
                                      bool ik_chains, bool twist_bones, bool labels) {
    if (!preview) return;
    
    preview->show_bones = bones;
    preview->show_joints = joints;
    preview->show_ik_chains = ik_chains;
    preview->show_twist_bones = twist_bones;
    preview->show_labels = labels;
    
    LOG_DEBUG("Updated preview visibility");
}

// Get preview mesh data
bool skeleton_get_preview_mesh_data(const SkeletonPreview* preview, PreviewVertex** vertices, 
                                    u32* vertex_count, u32** indices, u32* index_count) {
    if (!preview || !preview->is_active) return false;
    
    // In a full implementation, this would return the combined mesh data
    // For now, we'll return the bone mesh as an example
    if (vertices && vertex_count && preview->bone_mesh.is_valid) {
        *vertices = (PreviewVertex*)preview->bone_mesh.vertices;
        *vertex_count = preview->bone_mesh.vertex_count;
    }
    
    if (indices && index_count && preview->bone_mesh.is_valid) {
        *indices = (u32*)preview->bone_mesh.indices;
        *index_count = preview->bone_mesh.index_count;
    }
    
    return true;
}

// Set preview geometry parameters
void skeleton_set_preview_geometry(SkeletonPreview* preview, f32 bone_thickness, 
                                    f32 joint_radius) {
    if (!preview) return;
    
    preview->bone_thickness = bone_thickness;
    preview->joint_radius = joint_radius;
    
    // Recreate meshes with new parameters
    init_preview_meshes(preview);
    
    LOG_DEBUG("Updated preview geometry parameters");
}

// Export preview data
bool skeleton_export_preview_data(const SkeletonPreview* preview, const char* file_path) {
    if (!preview || !preview->is_active || !file_path) {
        return false;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open preview data file: %s", file_path);
        return false;
    }
    
    fprintf(file, "# Skeleton Preview Data\n");
    fprintf(file, "# Generated automatically\n\n");
    fprintf(file, "is_active: %s\n", preview->is_active ? "true" : "false");
    fprintf(file, "show_bones: %s\n", preview->show_bones ? "true" : "false");
    fprintf(file, "show_joints: %s\n", preview->show_joints ? "true" : "false");
    fprintf(file, "show_ik_chains: %s\n", preview->show_ik_chains ? "true" : "false");
    fprintf(file, "show_twist_bones: %s\n", preview->show_twist_bones ? "true" : "false");
    fprintf(file, "show_labels: %s\n", preview->show_labels ? "true" : "false");
    
    fprintf(file, "\n# Colors\n");
    fprintf(file, "skeleton_color: %.3f,%.3f,%.3f\n", 
           preview->skeleton_color.x, preview->skeleton_color.y, preview->skeleton_color.z);
    fprintf(file, "essential_color: %.3f,%.3f,%.3f\n", 
           preview->essential_color.x, preview->essential_color.y, preview->essential_color.z);
    fprintf(file, "ik_color: %.3f,%.3f,%.3f\n", 
           preview->ik_color.x, preview->ik_color.y, preview->ik_color.z);
    fprintf(file, "twist_color: %.3f,%.3f,%.3f\n", 
           preview->twist_color.x, preview->twist_color.y, preview->twist_color.z);
    
    fprintf(file, "\n# Geometry\n");
    fprintf(file, "bone_thickness: %.3f\n", preview->bone_thickness);
    fprintf(file, "joint_radius: %.3f\n", preview->joint_radius);
    
    fprintf(file, "\n# Mesh Data\n");
    fprintf(file, "bone_mesh_vertices: %u\n", preview->bone_mesh.vertex_count);
    fprintf(file, "bone_mesh_indices: %u\n", preview->bone_mesh.index_count);
    fprintf(file, "joint_mesh_vertices: %u\n", preview->joint_mesh.vertex_count);
    fprintf(file, "joint_mesh_indices: %u\n", preview->joint_mesh.index_count);
    
    fclose(file);
    LOG_INFO("Exported preview data to: %s", file_path);
    return true;
}

// Public API functions
u32 skeleton_preview_get_count(void) {
    return g_preview_system.preview_count;
}

SkeletonPreview* skeleton_preview_get_by_index(u32 index) {
    if (index >= g_preview_system.preview_count) {
        return NULL;
    }
    
    return &g_preview_system.previews[index];
}

void skeleton_preview_cleanup(void) {
    memset(&g_preview_system, 0, sizeof(SkeletonPreviewSystem));
    g_preview_system.system_initialized = false;
    LOG_INFO("Skeleton preview system cleaned up");
}
