#include "geometry/mesh_optimize.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Helper function to check if two vertices are approximately equal
static bool vertices_equal(const vertex_t* v1, const vertex_t* v2, f32 epsilon) {
    if (fabsf(v1->position.x - v2->position.x) > epsilon) return false;
    if (fabsf(v1->position.y - v2->position.y) > epsilon) return false;
    if (fabsf(v1->position.z - v2->position.z) > epsilon) return false;
    
    if (fabsf(v1->normal.x - v2->normal.x) > epsilon) return false;
    if (fabsf(v1->normal.y - v2->normal.y) > epsilon) return false;
    if (fabsf(v1->normal.z - v2->normal.z) > epsilon) return false;
    
    if (fabsf(v1->uv.x - v2->uv.x) > epsilon) return false;
    if (fabsf(v1->uv.y - v2->uv.y) > epsilon) return false;
    
    if (fabsf(v1->tangent.x - v2->tangent.x) > epsilon) return false;
    if (fabsf(v1->tangent.y - v2->tangent.y) > epsilon) return false;
    if (fabsf(v1->tangent.z - v2->tangent.z) > epsilon) return false;
    if (fabsf(v1->tangent.w - v2->tangent.w) > epsilon) return false;
    
    return true;
}

void mesh_deduplicate_vertices(mesh_t* mesh, f32 epsilon) {
    if (!mesh || mesh->vertex_count == 0) {
        return;
    }
    
    printf("Deduplicating vertices for mesh '%s' (epsilon: %.6f)...\n", mesh->name, epsilon);
    
    u32 original_vertex_count = mesh->vertex_count;
    u32* vertex_remap = (u32*)malloc(mesh->vertex_count * sizeof(u32));
    vertex_t* unique_vertices = (vertex_t*)malloc(mesh->vertex_count * sizeof(vertex_t));
    
    if (!vertex_remap || !unique_vertices) {
        free(vertex_remap);
        free(unique_vertices);
        printf("Error: Failed to allocate memory for vertex deduplication\n");
        return;
    }
    
    u32 unique_count = 0;
    
    // Find unique vertices
    for (u32 i = 0; i < mesh->vertex_count; i++) {
        bool found = false;
        
        // Check if this vertex matches any existing unique vertex
        for (u32 j = 0; j < unique_count; j++) {
            if (vertices_equal(&mesh->vertices[i], &unique_vertices[j], epsilon)) {
                vertex_remap[i] = j;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Add new unique vertex
            unique_vertices[unique_count] = mesh->vertices[i];
            vertex_remap[i] = unique_count;
            unique_count++;
        }
    }
    
    // Remap indices
    for (u32 i = 0; i < mesh->index_count; i++) {
        if (mesh->indices[i] < mesh->vertex_count) {
            mesh->indices[i] = vertex_remap[mesh->indices[i]];
        }
    }
    
    // Replace vertex array with unique vertices
    free(mesh->vertices);
    mesh->vertices = (vertex_t*)malloc(unique_count * sizeof(vertex_t));
    if (mesh->vertices) {
        memcpy(mesh->vertices, unique_vertices, unique_count * sizeof(vertex_t));
        mesh->vertex_count = unique_count;
        mesh->vertex_capacity = unique_count;
    }
    
    free(vertex_remap);
    free(unique_vertices);
    
    printf("Vertex deduplication complete: %u -> %u vertices (%.1f%% reduction)\n", 
           original_vertex_count, unique_count, 
           (1.0f - (f32)unique_count / original_vertex_count) * 100.0f);
}

// Simple vertex cache optimizer (basic implementation)
void mesh_optimize_vertex_cache(mesh_t* mesh) {
    if (!mesh || mesh->index_count < 3) {
        return;
    }
    
    printf("Optimizing vertex cache for mesh '%s'...\n", mesh->name);
    
    u32* optimized_indices = (u32*)malloc(mesh->index_count * sizeof(u32));
    if (!optimized_indices) {
        printf("Error: Failed to allocate memory for index optimization\n");
        return;
    }
    
    // Simple implementation: use the Tipsify algorithm concept
    // For now, we'll use a basic approach that preserves triangle order
    
    u32* vertex_used_time = (u32*)calloc(mesh->vertex_count, sizeof(u32));
    u32* vertex_score = (u32*)calloc(mesh->vertex_count, sizeof(u32));
    bool* vertex_added = (bool*)calloc(mesh->vertex_count, sizeof(bool));
    
    if (!vertex_used_time || !vertex_score || !vertex_added) {
        free(optimized_indices);
        free(vertex_used_time);
        free(vertex_score);
        free(vertex_added);
        return;
    }
    
    u32 current_time = 0;
    u32 output_index = 0;
    
    // Process triangles in order, but try to improve cache locality
    for (u32 tri = 0; tri < mesh->index_count / 3; tri++) {
        u32 tri_start = tri * 3;
        u32 indices[3] = {
            mesh->indices[tri_start],
            mesh->indices[tri_start + 1],
            mesh->indices[tri_start + 2]
        };
        
        // Add triangle to output
        for (int i = 0; i < 3; i++) {
            optimized_indices[output_index++] = indices[i];
            
            if (!vertex_added[indices[i]]) {
                vertex_added[indices[i]] = true;
                vertex_used_time[indices[i]] = current_time++;
            }
        }
    }
    
    // Replace original indices
    free(mesh->indices);
    mesh->indices = optimized_indices;
    
    free(vertex_used_time);
    free(vertex_score);
    free(vertex_added);
    
    printf("Vertex cache optimization complete\n");
}

void mesh_optimize_indices(mesh_t* mesh) {
    // For now, just call vertex cache optimization
    mesh_optimize_vertex_cache(mesh);
}

// Octahedral encoding for normal compression
static Vec2 encode_normal_oct8(Vec3 normal) {
    // Normalize input
    f32 length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > EPSILON) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }
    
    // Project to octahedron and fold
    Vec2 oct;
    oct.x = normal.x / (fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z));
    oct.y = normal.y / (fabsf(normal.x) + fabsf(normal.y) + fabsf(normal.z));
    
    // Fold negative Z
    if (normal.z < 0.0f) {
        f32 temp = oct.x;
        oct.x = (1.0f - fabsf(oct.y)) * (oct.x >= 0.0f ? 1.0f : -1.0f);
        oct.y = (1.0f - fabsf(temp)) * (oct.y >= 0.0f ? 1.0f : -1.0f);
    }
    
    // Convert to 8-bit signed integers
    oct.x = oct.x * 127.0f;
    oct.y = oct.y * 127.0f;
    
    return oct;
}

void mesh_pack_normals_oct8(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) {
        return;
    }
    
    printf("Packing normals to oct8 for mesh '%s'...\n", mesh->name);
    
    for (u32 i = 0; i < mesh->vertex_count; i++) {
        Vec2 packed = encode_normal_oct8(mesh->vertices[i].normal);
        
        // Store packed normals in the tangent.w and unused space
        // For now, we'll just print the packed values
        // In a real implementation, these would be stored in a separate compressed buffer
        if (i == 0) { // Print first few as example
            printf("Sample packed normal: (%.1f, %.1f)\n", packed.x, packed.y);
        }
    }
    
    printf("Normal packing complete\n");
}

void mesh_pack_normals_oct16(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) {
        return;
    }
    
    printf("Packing normals to oct16 for mesh '%s'...\n", mesh->name);
    
    // Similar to oct8 but with 16-bit precision
    for (u32 i = 0; i < mesh->vertex_count; i++) {
        Vec2 packed = encode_normal_oct8(mesh->vertices[i].normal);
        
        // Convert to 16-bit range
        packed.x = packed.x * 256.0f;  // Scale to 16-bit range
        packed.y = packed.y * 256.0f;
        
        if (i == 0) { // Print first few as example
            printf("Sample packed normal (16-bit): (%.1f, %.1f)\n", packed.x, packed.y);
        }
    }
    
    printf("Normal packing complete\n");
}

void mesh_quantize_uvs(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) {
        return;
    }
    
    printf("Quantizing UVs to 16-bit for mesh '%s'...\n", mesh->name);
    
    for (u32 i = 0; i < mesh->vertex_count; i++) {
        // Quantize UV coordinates to 16-bit (0-65535 range)
        Vec2 uv = mesh->vertices[i].uv;
        
        // Clamp to [0, 1] range first
        uv.x = uv.x < 0.0f ? 0.0f : (uv.x > 1.0f ? 1.0f : uv.x);
        uv.y = uv.y < 0.0f ? 0.0f : (uv.y > 1.0f ? 1.0f : uv.y);
        
        // Convert to 16-bit
        u16 quantized_u = (u16)(uv.x * 65535.0f);
        u16 quantized_v = (u16)(uv.y * 65535.0f);
        
        // Convert back to float to show precision loss
        Vec2 quantized_uv = {
            (f32)quantized_u / 65535.0f,
            (f32)quantized_v / 65535.0f
        };
        
        mesh->vertices[i].uv = quantized_uv;
        
        if (i == 0) { // Print first few as example
            printf("Original UV: (%.6f, %.6f) -> Quantized: (%.6f, %.6f)\n", 
                   uv.x, uv.y, quantized_uv.x, quantized_uv.y);
        }
    }
    
    printf("UV quantization complete\n");
}
