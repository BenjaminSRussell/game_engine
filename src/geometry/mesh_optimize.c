#include "geometry/mesh_optimize.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// CONSTANTS AND DEFAULTS
// ============================================================================

#define MESH_OPTIMIZE_EPSILON 1e-6f
#define MESH_OPTIMIZE_DEFAULT_ANGLE_THRESHOLD 60.0f
#define MESH_OPTIMIZE_CACHE_SIZE 32

const normal_calculation_options_t NORMAL_CALCULATION_DEFAULT = {
    .smooth_normals = true,
    .angle_threshold = MESH_OPTIMIZE_DEFAULT_ANGLE_THRESHOLD,
    .preserve_existing = false,
    .normalize_normals = true,
    .calculate_tangents = false
};

// ============================================================================
// INTERNAL STRUCTURES AND HELPERS
// ============================================================================

typedef struct vertex_normal_info_t {
    Vec3 normal_sum;
    uint32_t triangle_count;
    Vec3 position;
} vertex_normal_info_t;

typedef struct triangle_info_t {
    Vec3 vertices[3];
    Vec3 normal;
    float area;
    uint32_t vertex_indices[3];
} triangle_info_t;

static bool vertices_equal(const vertex_t* v1, const vertex_t* v2, float epsilon) {
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

static Vec3 calculate_triangle_normal(const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    Vec3 edge1 = vec3_sub(*v1, *v0);
    Vec3 edge2 = vec3_sub(*v2, *v0);
    Vec3 normal = vec3_cross(edge1, edge2);
    return vec3_normalize(normal);
}

static float calculate_triangle_area(const Vec3* v0, const Vec3* v1, const Vec3* v2) {
    Vec3 edge1 = vec3_sub(*v1, *v0);
    Vec3 edge2 = vec3_sub(*v2, *v0);
    Vec3 cross = vec3_cross(edge1, edge2);
    return vec3_length(cross) * 0.5f;
}

static float angle_between_normals(const Vec3* n1, const Vec3* n2) {
    float dot = vec3_dot(*n1, *n2);
    dot = fmaxf(-1.0f, fminf(1.0f, dot)); // Clamp to [-1, 1]
    return acosf(dot) * 180.0f / M_PI;
}

static bool is_valid_normal(const Vec3* normal) {
    return !isnan(normal->x) && !isnan(normal->y) && !isnan(normal->z) &&
           !isinf(normal->x) && !isinf(normal->y) && !isinf(normal->z) &&
           vec3_length(*normal) > MESH_OPTIMIZE_EPSILON;
}

// ============================================================================
// VERTEX NORMAL CALCULATION
// ============================================================================

void mesh_calculate_face_normals(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0 || mesh->index_count < 3) {
        return;
    }
    
    printf("Calculating face normals for mesh '%s'...\n", mesh->name);
    
    // Initialize all normals to zero
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        mesh->vertices[i].normal = (Vec3){0.0f, 0.0f, 0.0f};
    }
    
    // Calculate face normals and assign to vertices
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            continue; // Skip invalid indices
        }
        
        Vec3 v0 = mesh->vertices[i0].position;
        Vec3 v1 = mesh->vertices[i1].position;
        Vec3 v2 = mesh->vertices[i2].position;
        
        Vec3 face_normal = calculate_triangle_normal(&v0, &v1, &v2);
        
        // Assign face normal to all three vertices
        mesh->vertices[i0].normal = face_normal;
        mesh->vertices[i1].normal = face_normal;
        mesh->vertices[i2].normal = face_normal;
    }
    
    printf("Face normals calculation complete\n");
}

void mesh_calculate_smooth_normals(mesh_t* mesh, float angle_threshold) {
    if (!mesh || mesh->vertex_count == 0 || mesh->index_count < 3) {
        return;
    }
    
    printf("Calculating smooth normals for mesh '%s' (angle threshold: %.1f°)...\n", 
           mesh->name, angle_threshold);
    
    // Create temporary storage for vertex normal accumulation
    vertex_normal_info_t* vertex_info = calloc(mesh->vertex_count, sizeof(vertex_normal_info_t));
    triangle_info_t* triangles = malloc((mesh->index_count / 3) * sizeof(triangle_info_t));
    
    if (!vertex_info || !triangles) {
        free(vertex_info);
        free(triangles);
        printf("Error: Failed to allocate memory for normal calculation\n");
        return;
    }
    
    // Initialize vertex info
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        vertex_info[i].normal_sum = (Vec3){0.0f, 0.0f, 0.0f};
        vertex_info[i].triangle_count = 0;
        vertex_info[i].position = mesh->vertices[i].position;
    }
    
    // Process all triangles
    uint32_t triangle_count = 0;
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            continue; // Skip invalid indices
        }
        
        triangle_info_t* tri = &triangles[triangle_count];
        tri->vertices[0] = mesh->vertices[i0].position;
        tri->vertices[1] = mesh->vertices[i1].position;
        tri->vertices[2] = mesh->vertices[i2].position;
        tri->vertex_indices[0] = i0;
        tri->vertex_indices[1] = i1;
        tri->vertex_indices[2] = i2;
        
        tri->normal = calculate_triangle_normal(&tri->vertices[0], &tri->vertices[1], &tri->vertices[2]);
        tri->area = calculate_triangle_area(&tri->vertices[0], &tri->vertices[1], &tri->vertices[2]);
        
        triangle_count++;
    }
    
    // Accumulate normals for each vertex
    for (uint32_t i = 0; i < triangle_count; i++) {
        const triangle_info_t* tri = &triangles[i];
        
        for (int j = 0; j < 3; j++) {
            uint32_t vertex_idx = tri->vertex_indices[j];
            vertex_normal_info_t* info = &vertex_info[vertex_idx];
            
            // Add weighted normal (area-weighted)
            Vec3 weighted_normal = vec3_mul(tri->normal, tri->area);
            info->normal_sum = vec3_add(info->normal_sum, weighted_normal);
            info->triangle_count++;
        }
    }
    
    // Calculate smooth normals with angle threshold
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        if (vertex_info[i].triangle_count == 0) {
            mesh->vertices[i].normal = (Vec3){0.0f, 1.0f, 0.0f}; // Default normal
            continue;
        }
        
        // Calculate average normal
        Vec3 average_normal = vec3_normalize(vertex_info[i].normal_sum);
        
        // Check angle against adjacent triangles
        bool use_smooth = true;
        uint32_t valid_adjacent = 0;
        Vec3 smooth_normal = average_normal;
        
        for (uint32_t j = 0; j < triangle_count; j++) {
            const triangle_info_t* tri = &triangles[j];
            
            // Check if this triangle uses our vertex
            bool uses_vertex = false;
            for (int k = 0; k < 3; k++) {
                if (tri->vertex_indices[k] == i) {
                    uses_vertex = true;
                    break;
                }
            }
            
            if (!uses_vertex) continue;
            
            // Check angle between triangle normal and average
            float angle = angle_between_normals(&tri->normal, &average_normal);
            
            if (angle <= angle_threshold) {
                // Add this triangle's normal to smooth normal
                Vec3 weighted_normal = vec3_mul(tri->normal, tri->area);
                smooth_normal = vec3_add(smooth_normal, weighted_normal);
                valid_adjacent++;
            } else {
                use_smooth = false;
                break; // Sharp edge detected, use face normal
            }
        }
        
        if (use_smooth && valid_adjacent > 0) {
            mesh->vertices[i].normal = vec3_normalize(smooth_normal);
        } else {
            // Use face normal (find a triangle that uses this vertex)
            for (uint32_t j = 0; j < triangle_count; j++) {
                const triangle_info_t* tri = &triangles[j];
                for (int k = 0; k < 3; k++) {
                    if (tri->vertex_indices[k] == i) {
                        mesh->vertices[i].normal = tri->normal;
                        break;
                    }
                }
            }
        }
    }
    
    free(vertex_info);
    free(triangles);
    
    printf("Smooth normals calculation complete\n");
}

void mesh_calculate_normals_advanced(mesh_t* mesh, const normal_calculation_options_t* options) {
    if (!mesh) return;
    
    const normal_calculation_options_t* opts = options ? options : &NORMAL_CALCULATION_DEFAULT;
    
    if (opts->smooth_normals) {
        mesh_calculate_smooth_normals(mesh, opts->angle_threshold);
    } else {
        mesh_calculate_face_normals(mesh);
    }
    
    if (opts->normalize_normals) {
        // Ensure all normals are unit length
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            if (is_valid_normal(&mesh->vertices[i].normal)) {
                mesh->vertices[i].normal = vec3_normalize(mesh->vertices[i].normal);
            } else {
                mesh->vertices[i].normal = (Vec3){0.0f, 1.0f, 0.0f}; // Default up vector
            }
        }
    }
    
    if (opts->calculate_tangents) {
        mesh_calculate_tangents(mesh);
    }
}

void mesh_optimize_normals(mesh_t* mesh, float angle_threshold) {
    if (!mesh) return;
    
    printf("Optimizing normals for mesh '%s'...\n", mesh->name);
    
    // Normalize existing normals
    uint32_t fixed_normals = 0;
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        if (!is_valid_normal(&mesh->vertices[i].normal)) {
            mesh->vertices[i].normal = (Vec3){0.0f, 1.0f, 0.0f}; // Default up vector
            fixed_normals++;
        } else {
            mesh->vertices[i].normal = vec3_normalize(mesh->vertices[i].normal);
        }
    }
    
    printf("Fixed %u invalid normals\n", fixed_normals);
    
    // Recalculate smooth normals if needed
    mesh_calculate_smooth_normals(mesh, angle_threshold);
}

void mesh_calculate_tangents(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0 || mesh->index_count < 3) {
        return;
    }
    
    printf("Calculating tangents for mesh '%s'...\n", mesh->name);
    
    // Initialize tangents
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        mesh->vertices[i].tangent = (Vec4){0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    // Calculate tangents for each triangle
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        
        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) {
            continue;
        }
        
        vertex_t* v0 = &mesh->vertices[i0];
        vertex_t* v1 = &mesh->vertices[i1];
        vertex_t* v2 = &mesh->vertices[i2];
        
        Vec3 edge1 = vec3_sub(v1->position, v0->position);
        Vec3 edge2 = vec3_sub(v2->position, v0->position);
        
        Vec2 uv1 = vec2_sub(v1->uv, v0->uv);
        Vec2 uv2 = vec2_sub(v2->uv, v0->uv);
        
        float r = 1.0f / (uv1.x * uv2.y - uv1.y * uv2.x);
        Vec3 tangent = vec3_mul(vec3_sub(vec3_mul(edge1, uv2.y), vec3_mul(edge2, uv1.y)), r);
        
        // Add tangent to each vertex
        Vec4 tangent4 = {tangent.x, tangent.y, tangent.z, 0.0f};
        v0->tangent = vec4_add(v0->tangent, tangent4);
        v1->tangent = vec4_add(v1->tangent, tangent4);
        v2->tangent = vec4_add(v2->tangent, tangent4);
    }
    
    // Orthogonalize tangents and calculate handedness
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        Vec3 normal = mesh->vertices[i].normal;
        Vec3 tangent = {mesh->vertices[i].tangent.x, mesh->vertices[i].tangent.y, mesh->vertices[i].tangent.z};
        
        // Gram-Schmidt orthogonalize
        tangent = vec3_sub(tangent, vec3_mul(normal, vec3_dot(normal, tangent)));
        tangent = vec3_normalize(tangent);
        
        // Calculate handedness
        Vec3 bitangent = vec3_cross(normal, tangent);
        float handedness = (vec3_dot(bitangent, (Vec3){0.0f, 0.0f, 1.0f}) < 0.0f) ? -1.0f : 1.0f;
        
        mesh->vertices[i].tangent = (Vec4){tangent.x, tangent.y, tangent.z, handedness};
    }
    
    printf("Tangent calculation complete\n");
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
