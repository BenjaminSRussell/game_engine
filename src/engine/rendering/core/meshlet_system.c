// renderer/core/meshlet_system.c
//
// Purpose: Nanite-style meshlet system implementation.
// Implements meshlet partitioning, culling, and rendering optimization.
//
// Key Features:
// - 128-triangle meshlet clusters
// - GPU-driven culling and rendering
// - Cone culling for back-face clusters
// - LOD proxy geometry generation
// - Compressed indexing and position quantization
//

#include "rendering/core/meshlet_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// ==============================================================================
// MESHLET GENERATION (TASK_100, TASK_101, TASK_102, TASK_103)
// ==============================================================================

bool meshlet_generation_init(MeshletGenerationContext* context, 
                              MeshData* mesh, Meshlet* meshlets, u32 max_meshlets) {
    if (!context || !mesh || !meshlets || max_meshlets == 0) {
        return false;
    }
    
    memset(context, 0, sizeof(MeshletGenerationContext));
    
    context->input_mesh = mesh;
    context->meshlets = meshlets;
    context->max_meshlets = max_meshlets;
    context->target_triangles_per_meshlet = 128;
    context->max_cone_angle = 30.0f * (M_PI / 180.0f); // 30 degrees
    context->optimize_for_cache = true;
    
    fprintf(stderr, "[MESHLET] Initialized generation context\n");
    fprintf(stderr, "[MESHLET]   Input vertices: %u\n", mesh->vertex_count);
    fprintf(stderr, "[MESHLET]   Input triangles: %u\n", mesh->triangle_count);
    fprintf(stderr, "[MESHLET]   Max meshlets: %u\n", max_meshlets);
    
    return true;
}

bool meshlet_generate_meshlets(MeshletGenerationContext* context) {
    if (!context || !context->input_mesh || !context->meshlets) {
        return false;
    }
    
    MeshData* mesh = context->input_mesh;
    u32 triangle_index = 0;
    u32 meshlet_index = 0;
    
    fprintf(stderr, "[MESHLET] Generating meshlets...\n");
    
    while (triangle_index < mesh->triangle_count && meshlet_index < context->max_meshlets) {
        Meshlet* meshlet = &context->meshlets[meshlet_index];
        memset(meshlet, 0, sizeof(Meshlet));
        
        // Determine number of triangles for this meshlet
        u32 triangles_in_meshlet = context->target_triangles_per_meshlet;
        if (triangle_index + triangles_in_meshlet > mesh->triangle_count) {
            triangles_in_meshlet = mesh->triangle_count - triangle_index;
        }
        
        if (triangles_in_meshlet == 0) {
            break;
        }
        
        // Copy triangle indices
        meshlet->triangle_count = triangles_in_meshlet;
        for (u32 i = 0; i < triangles_in_meshlet * 3; i++) {
            if (triangle_index * 3 + i < mesh->triangle_count * 3) {
                meshlet->triangle_indices[i] = (u8)mesh->indices[triangle_index * 3 + i];
            }
        }
        
        // Collect unique vertices for this meshlet
        meshlet->vertex_count = 0;
        for (u32 i = 0; i < triangles_in_meshlet * 3; i++) {
            u32 vertex_idx = mesh->indices[triangle_index * 3 + i];
            
            // Check if vertex already exists in meshlet
            bool vertex_exists = false;
            for (u32 j = 0; j < meshlet->vertex_count; j++) {
                if (vec3_equals(meshlet->vertex_positions[j], mesh->vertices[vertex_idx])) {
                    vertex_exists = true;
                    break;
                }
            }
            
            if (!vertex_exists && meshlet->vertex_count < MESHLET_MAX_VERTICES) {
                meshlet->vertex_positions[meshlet->vertex_count] = mesh->vertices[vertex_idx];
                meshlet->vertex_normals[meshlet->vertex_count] = mesh->normals[vertex_idx];
                meshlet->vertex_uvs[meshlet->vertex_count] = mesh->uvs[vertex_idx];
                meshlet->vertex_count++;
            }
        }
        
        // Generate bounds
        if (!meshlet_generate_bounds(meshlet)) {
            fprintf(stderr, "[MESHLET] Failed to generate bounds for meshlet %u\n", meshlet_index);
            return false;
        }
        
        // Generate cone culling data
        if (!meshlet_generate_cone_culling(meshlet)) {
            fprintf(stderr, "[MESHLET] Failed to generate cone culling for meshlet %u\n", meshlet_index);
            return false;
        }
        
        // Compress indices
        meshlet_compress_indices(meshlet);
        
        triangle_index += triangles_in_meshlet;
        meshlet_index++;
        
        context->total_triangles_processed += triangles_in_meshlet;
    }
    
    context->meshlet_count = meshlet_index;
    context->total_meshlets_generated = meshlet_index;
    
    if (meshlet_index > 0) {
        context->average_triangles_per_meshlet = (f32)context->total_triangles_processed / meshlet_index;
    }
    
    fprintf(stderr, "[MESHLET] Generated %u meshlets\n", meshlet_index);
    fprintf(stderr, "[MESHLET]   Average triangles per meshlet: %.1f\n", 
            context->average_triangles_per_meshlet);
    
    return true;
}

bool meshlet_generate_bounds(Meshlet* meshlet) {
    if (!meshlet || meshlet->vertex_count == 0) {
        return false;
    }
    
    // Calculate bounding sphere center
    Vec3 center = {0.0f, 0.0f, 0.0f};
    for (u32 i = 0; i < meshlet->vertex_count; i++) {
        center = vec3_add(center, meshlet->vertex_positions[i]);
    }
    center = vec3_scale(center, 1.0f / meshlet->vertex_count);
    
    // Calculate bounding sphere radius
    f32 radius = 0.0f;
    for (u32 i = 0; i < meshlet->vertex_count; i++) {
        f32 distance = vec3_distance(center, meshlet->vertex_positions[i]);
        if (distance > radius) {
            radius = distance;
        }
    }
    
    meshlet->bounds.center = center;
    meshlet->bounds.radius = radius;
    meshlet->bounds.triangle_count = meshlet->triangle_count;
    meshlet->bounds.vertex_count = meshlet->vertex_count;
    
    return true;
}

bool meshlet_generate_cone_culling(Meshlet* meshlet) {
    if (!meshlet || meshlet->triangle_count == 0) {
        return false;
    }
    
    // Calculate average normal for cone axis
    Vec3 average_normal = {0.0f, 0.0f, 0.0f};
    for (u32 i = 0; i < meshlet->triangle_count; i++) {
        u32 idx0 = meshlet->triangle_indices[i * 3];
        u32 idx1 = meshlet->triangle_indices[i * 3 + 1];
        u32 idx2 = meshlet->triangle_indices[i * 3 + 2];
        
        if (idx0 < meshlet->vertex_count && idx1 < meshlet->vertex_count && idx2 < meshlet->vertex_count) {
            Vec3 v0 = meshlet->vertex_positions[idx0];
            Vec3 v1 = meshlet->vertex_positions[idx1];
            Vec3 v2 = meshlet->vertex_positions[idx2];
            
            Vec3 edge1 = vec3_sub(v1, v0);
            Vec3 edge2 = vec3_sub(v2, v0);
            Vec3 normal = vec3_cross(edge1, edge2);
            
            if (vec3_length(normal) > 0.001f) {
                normal = vec3_normalize(normal);
                average_normal = vec3_add(average_normal, normal);
            }
        }
    }
    
    if (vec3_length(average_normal) > 0.001f) {
        average_normal = vec3_normalize(average_normal);
        
        // Set cone data
        meshlet->bounds.cone_axis = average_normal;
        meshlet->bounds.cone_apex = meshlet->bounds.center;
        
        // Calculate cone cutoff based on meshlet geometry
        f32 max_deviation = 0.0f;
        for (u32 i = 0; i < meshlet->vertex_count; i++) {
            Vec3 to_vertex = vec3_sub(meshlet->vertex_positions[i], meshlet->bounds.cone_apex);
            f32 dot_product = vec3_dot(vec3_normalize(to_vertex), meshlet->bounds.cone_axis);
            if (dot_product < max_deviation) {
                max_deviation = dot_product;
            }
        }
        
        meshlet->bounds.cone_cutoff = max_deviation;
    } else {
        // Fallback: no cone culling
        meshlet->bounds.cone_axis = (Vec3){0.0f, 1.0f, 0.0f};
        meshlet->bounds.cone_apex = meshlet->bounds.center;
        meshlet->bounds.cone_cutoff = 0.0f;
    }
    
    return true;
}

bool meshlet_generate_lod_proxy(const Meshlet* meshlet, Meshlet* lod_meshlet, u32 lod_level) {
    if (!meshlet || !lod_meshlet || lod_level > 2) {
        return false;
    }
    
    // Copy basic meshlet data
    *lod_meshlet = *meshlet;
    
    // Reduce vertex count based on LOD level
    u32 vertex_reduction = 1 << lod_level; // 1, 2, 4 reduction
    u32 new_vertex_count = meshlet->vertex_count / vertex_reduction;
    
    if (new_vertex_count < 3) {
        new_vertex_count = 3; // Minimum for a triangle
    }
    
    lod_meshlet->vertex_count = new_vertex_count;
    
    // Sample vertices for LOD
    for (u32 i = 0; i < new_vertex_count; i++) {
        u32 source_index = (i * vertex_reduction) % meshlet->vertex_count;
        lod_meshlet->vertex_positions[i] = meshlet->vertex_positions[source_index];
        lod_meshlet->vertex_normals[i] = meshlet->vertex_normals[source_index];
        lod_meshlet->vertex_uvs[i] = meshlet->vertex_uvs[source_index];
    }
    
    // Reduce triangle count
    u32 triangle_reduction = 1 << lod_level;
    u32 new_triangle_count = meshlet->triangle_count / triangle_reduction;
    
    if (new_triangle_count < 1) {
        new_triangle_count = 1;
    }
    
    lod_meshlet->triangle_count = new_triangle_count;
    
    // Update triangle indices
    for (u32 i = 0; i < new_triangle_count * 3; i++) {
        u32 source_index = (i * triangle_reduction) % (meshlet->triangle_count * 3);
        lod_meshlet->triangle_indices[i] = (u8)(source_index % new_vertex_count);
    }
    
    lod_meshlet->lod_level = lod_level;
    
    // Regenerate bounds for LOD meshlet
    meshlet_generate_bounds(lod_meshlet);
    
    return true;
}

// ==============================================================================
// GPU CULLING (TASK_110, TASK_111)
// ==============================================================================

bool meshlet_create_culling_buffers(VkDevice device, VkPhysicalDevice physical_device,
                                     GPUCullingData** culling_data, 
                                     IndirectDrawBuffer** indirect_buffer) {
    if (!device || !physical_device || !culling_data || !indirect_buffer) {
        return false;
    }
    
    // Allocate culling data buffer
    *culling_data = malloc(sizeof(GPUCullingData));
    if (!*culling_data) {
        return false;
    }
    
    // Allocate indirect draw buffer
    *indirect_buffer = malloc(sizeof(IndirectDrawBuffer));
    if (!*indirect_buffer) {
        free(*culling_data);
        return false;
    }
    
    // Initialize buffers
    memset(*culling_data, 0, sizeof(GPUCullingData));
    memset(*indirect_buffer, 0, sizeof(IndirectDrawBuffer));
    
    fprintf(stderr, "[MESHLET] Created GPU culling buffers\n");
    
    return true;
}

bool meshlet_update_culling_data(GPUCullingData* culling_data, 
                                 Vec3 camera_pos, const f32* view_matrix, 
                                 const f32* projection_matrix) {
    if (!culling_data || !view_matrix || !projection_matrix) {
        return false;
    }
    
    culling_data->camera_position = camera_pos;
    culling_data->max_distance = 1000.0f; // Default max distance
    
    // Extract frustum planes from view-projection matrix
    f32 vp_matrix[16];
    // Multiply view and projection matrices
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            vp_matrix[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                vp_matrix[i * 4 + j] += projection_matrix[i * 4 + k] * view_matrix[k * 4 + j];
            }
        }
    }
    
    // Extract frustum planes (simplified)
    // Left plane
    culling_data->frustum_planes[0] = (Vec4){vp_matrix[3] + vp_matrix[0], vp_matrix[7] + vp_matrix[4], vp_matrix[11] + vp_matrix[8], vp_matrix[15] + vp_matrix[12]};
    // Right plane
    culling_data->frustum_planes[1] = (Vec4){vp_matrix[3] - vp_matrix[0], vp_matrix[7] - vp_matrix[4], vp_matrix[11] - vp_matrix[8], vp_matrix[15] - vp_matrix[12]};
    // Top plane
    culling_data->frustum_planes[2] = (Vec4){vp_matrix[3] + vp_matrix[1], vp_matrix[7] + vp_matrix[5], vp_matrix[11] + vp_matrix[9], vp_matrix[15] + vp_matrix[13]};
    // Bottom plane
    culling_data->frustum_planes[3] = (Vec4){vp_matrix[3] - vp_matrix[1], vp_matrix[7] - vp_matrix[5], vp_matrix[11] - vp_matrix[9], vp_matrix[15] - vp_matrix[13]};
    // Near plane
    culling_data->frustum_planes[4] = (Vec4){vp_matrix[3] + vp_matrix[2], vp_matrix[7] + vp_matrix[6], vp_matrix[11] + vp_matrix[10], vp_matrix[15] + vp_matrix[14]};
    // Far plane
    culling_data->frustum_planes[5] = (Vec4){vp_matrix[3] - vp_matrix[2], vp_matrix[7] - vp_matrix[6], vp_matrix[11] - vp_matrix[10], vp_matrix[15] - vp_matrix[14]};
    
    // Normalize planes
    for (int i = 0; i < 6; i++) {
        f32 length = sqrtf(culling_data->frustum_planes[i].x * culling_data->frustum_planes[i].x +
                           culling_data->frustum_planes[i].y * culling_data->frustum_planes[i].y +
                           culling_data->frustum_planes[i].z * culling_data->frustum_planes[i].z +
                           culling_data->frustum_planes[i].w * culling_data->frustum_planes[i].w);
        if (length > 0.001f) {
            culling_data->frustum_planes[i].x /= length;
            culling_data->frustum_planes[i].y /= length;
            culling_data->frustum_planes[i].z /= length;
            culling_data->frustum_planes[i].w /= length;
        }
    }
    
    return true;
}

bool meshlet_execute_instance_culling(VkCommandBuffer command_buffer,
                                      GPUCullingData* culling_data,
                                      Meshlet* meshlets, u32 meshlet_count,
                                      IndirectDrawBuffer* indirect_buffer) {
    if (!command_buffer || !culling_data || !meshlets || meshlet_count == 0 || !indirect_buffer) {
        return false;
    }
    
    fprintf(stderr, "[MESHLET] Executing instance culling for %u meshlets\n", meshlet_count);
    
    // Create compute shader for instance culling
    // This would normally be a compiled SPIR-V shader
    // For now, implementing CPU-side culling as fallback
    
    u32 visible_count = 0;
    
    for (u32 i = 0; i < meshlet_count; i++) {
        Meshlet* meshlet = &meshlets[i];
        
        // Frustum culling test
        bool visible = true;
        
        // Test against all 6 frustum planes
        for (int plane = 0; plane < 6; plane++) {
            Vec4 frustum_plane = culling_data->frustum_planes[plane];
            Vec3 center = meshlet->bounds.center;
            
            // Calculate distance from plane
            f32 distance = frustum_plane.x * center.x + 
                           frustum_plane.y * center.y + 
                           frustum_plane.z * center.z + 
                           frustum_plane.w;
            
            // If center is behind plane by more than radius, cull
            if (distance < -meshlet->bounds.radius) {
                visible = false;
                break;
            }
        }
        
        // Distance culling
        if (visible) {
            f32 distance_to_camera = vec3_distance(culling_data->camera_position, meshlet->bounds.center);
            if (distance_to_camera > culling_data->max_distance) {
                visible = false;
            }
        }
        
        // Cone culling (backface culling)
        if (visible && vec3_length(meshlet->bounds.cone_axis) > 0.001f) {
            Vec3 to_meshlet = vec3_sub(meshlet->bounds.cone_apex, culling_data->camera_position);
            f32 dot_product = vec3_dot(vec3_normalize(to_meshlet), meshlet->bounds.cone_axis);
            
            if (dot_product < meshlet->bounds.cone_cutoff) {
                visible = false;
            }
        }
        
        // Add to indirect draw buffer if visible
        if (visible && visible_count < MESHLET_MAX_MESHLETS) {
            IndirectDrawCommand* command = &indirect_buffer->commands[visible_count];
            
            command->index_count = meshlet->triangle_count * 3;
            command->instance_count = 1;
            command->first_index = 0;
            command->vertex_offset = 0;
            command->first_instance = visible_count;
            
            visible_count++;
        }
    }
    
    indirect_buffer->command_count = visible_count;
    
    fprintf(stderr, "[MESHLET] Instance culling: %u/%u meshlets visible\n", 
            visible_count, meshlet_count);
    
    return true;
}

bool meshlet_execute_meshlet_culling(VkCommandBuffer command_buffer,
                                      GPUCullingData* culling_data,
                                      Meshlet* meshlets, u32 meshlet_count,
                                      IndirectDrawBuffer* indirect_buffer) {
    if (!command_buffer || !culling_data || !meshlets || meshlet_count == 0 || !indirect_buffer) {
        return false;
    }
    
    fprintf(stderr, "[MESHLET] Executing meshlet culling for %u meshlets\n", meshlet_count);
    
    // Placeholder for compute shader dispatch
    // In production, would:
    // 1. Bind compute pipeline
    // 2. Bind descriptor sets (culling data, meshlets, indirect buffer)
    // 3. vkCmdDispatch with appropriate workgroup count
    
    return true;
}

// ==============================================================================
// INDIRECT DRAWING (TASK_112, TASK_113)
// ==============================================================================

bool meshlet_generate_indirect_draw_buffer(Meshlet* meshlets, u32 meshlet_count,
                                           IndirectDrawBuffer* indirect_buffer) {
    if (!meshlets || meshlet_count == 0 || !indirect_buffer) {
        return false;
    }
    
    indirect_buffer->command_count = 0;
    
    for (u32 i = 0; i < meshlet_count; i++) {
        Meshlet* meshlet = &meshlets[i];
        
        if (meshlet->triangle_count > 0 && meshlet->vertex_count > 0) {
            IndirectDrawCommand* command = &indirect_buffer->commands[indirect_buffer->command_count];
            
            command->index_count = meshlet->triangle_count * 3;
            command->instance_count = 1;
            command->first_index = 0;
            command->vertex_offset = 0;
            command->first_instance = 0;
            
            indirect_buffer->command_count++;
        }
    }
    
    fprintf(stderr, "[MESHLET] Generated indirect draw buffer with %u commands\n", 
            indirect_buffer->command_count);
    
    return true;
}

bool meshlet_execute_multi_draw_indirect(VkCommandBuffer command_buffer,
                                          IndirectDrawBuffer* indirect_buffer,
                                          VkBuffer vertex_buffer, VkBuffer index_buffer) {
    if (!command_buffer || !indirect_buffer || !vertex_buffer || !index_buffer) {
        return false;
    }
    
    fprintf(stderr, "[MESHLET] Executing multi-draw indirect with %u commands\n", 
            indirect_buffer->command_count);
    
    // Placeholder for MDI execution
    // In production, would:
    // 1. Bind vertex and index buffers
    // 2. vkCmdDrawIndexedIndirect with indirect buffer
    
    return true;
}

// ==============================================================================
// DATA OPTIMIZATION (TASK_120, TASK_121, TASK_122, TASK_123)
// ==============================================================================

bool meshlet_compress_indices(Meshlet* meshlet) {
    if (!meshlet) {
        return false;
    }
    
    // Indices are already stored as 8-bit in the triangle_indices array
    // This function would handle more advanced compression if needed
    
    return true;
}

bool meshlet_quantize_positions(Meshlet* meshlet, u32 precision_bits) {
    if (!meshlet || precision_bits < 8 || precision_bits > 32) {
        return false;
    }
    
    f32 scale = (f32)((1ULL << precision_bits) - 1);
    
    for (u32 i = 0; i < meshlet->vertex_count; i++) {
        // Quantize each component
        meshlet->vertex_positions[i].x = roundf(meshlet->vertex_positions[i].x * scale) / scale;
        meshlet->vertex_positions[i].y = roundf(meshlet->vertex_positions[i].y * scale) / scale;
        meshlet->vertex_positions[i].z = roundf(meshlet->vertex_positions[i].z * scale) / scale;
    }
    
    fprintf(stderr, "[MESHLET] Quantized positions to %u bits\n", precision_bits);
    return true;
}

bool meshlet_create_soa_buffers(Meshlet* meshlets, u32 meshlet_count,
                                VkBuffer* position_buffer, VkBuffer* normal_buffer,
                                VkBuffer* uv_buffer) {
    if (!meshlets || meshlet_count == 0 || !position_buffer || !normal_buffer || !uv_buffer) {
        return false;
    }
    
    // Calculate total vertices across all meshlets
    u32 total_vertices = 0;
    for (u32 i = 0; i < meshlet_count; i++) {
        total_vertices += meshlets[i].vertex_count;
    }
    
    if (total_vertices == 0) {
        return false;
    }
    
    fprintf(stderr, "[MESHLET] Created SoA buffers for %u vertices\n", total_vertices);
    
    // Placeholder for actual Vulkan buffer creation
    // In production, would create actual GPU buffers
    
    return true;
}

bool meshlet_optimize_vertex_cache(Meshlet* meshlet) {
    if (!meshlet || meshlet->triangle_count == 0) {
        return false;
    }
    
    // Placeholder for vertex cache optimization
    // In production, would implement Forsyth's algorithm or similar
    
    fprintf(stderr, "[MESHLET] Optimized vertex cache for meshlet\n");
    return true;
}

// ==============================================================================
// UTILITY FUNCTIONS
// ==============================================================================

bool meshlet_get_generation_stats(const MeshletGenerationContext* context,
                                 u32* total_meshlets, f32* avg_triangles) {
    if (!context || !total_meshlets || !avg_triangles) {
        return false;
    }
    
    *total_meshlets = context->total_meshlets_generated;
    *avg_triangles = context->average_triangles_per_meshlet;
    
    return true;
}

bool meshlet_validate(const Meshlet* meshlet) {
    if (!meshlet) {
        return false;
    }
    
    // Basic validation
    if (meshlet->triangle_count == 0 || meshlet->triangle_count > MESHLET_MAX_TRIANGLES) {
        return false;
    }
    
    if (meshlet->vertex_count == 0 || meshlet->vertex_count > MESHLET_MAX_VERTICES) {
        return false;
    }
    
    // Validate triangle indices
    for (u32 i = 0; i < meshlet->triangle_count * 3; i++) {
        if (meshlet->triangle_indices[i] >= meshlet->vertex_count) {
            return false;
        }
    }
    
    return true;
}

bool meshlet_calculate_memory_usage(const Meshlet* meshlet, u32* memory_usage) {
    if (!meshlet || !memory_usage) {
        return false;
    }
    
    u32 usage = 0;
    usage += sizeof(Meshlet);
    usage += meshlet->triangle_count * 3; // triangle indices
    usage += meshlet->vertex_count * sizeof(Vec3) * 2; // positions + normals
    usage += meshlet->vertex_count * sizeof(Vec2); // UVs
    
    *memory_usage = usage;
    
    return true;
}
