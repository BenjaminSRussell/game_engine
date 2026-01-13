// src/engine/rendering/lod/lod_system.c
// LOD System - Level of Detail with mesh optimization and distance-based scaling

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// LOD System Types
// ============================================================================

typedef enum {
    LOD_LEVEL_0 = 0,  // Highest quality (closest)
    LOD_LEVEL_1,
    LOD_LEVEL_2,
    LOD_LEVEL_3,
    LOD_LEVEL_4,  // Lowest quality (farthest)
    LOD_LEVEL_COUNT
} LODLevel;

typedef struct {
    float distance;      // Distance at which this LOD becomes active
    float reduction;     // Triangle reduction factor (0.0-1.0)
    float screen_size;     // Minimum screen space size to maintain
    bool enabled;        // Whether this LOD level is enabled
} LODConfig;

typedef struct {
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t index_count;
    float bounding_radius;
    
    // LOD meshes
    void *lod_meshes[LOD_LEVEL_COUNT];
    uint32_t lod_vertex_counts[LOD_LEVEL_COUNT];
    uint32_t lod_triangle_counts[LOD_LEVEL_COUNT];
    bool lod_generated[LOD_LEVEL_COUNT];
    
    // Optimization data
    float *bounding_boxes;  // [min_x, min_y, min_z, max_x, max_y, max_z] * lod_count
    uint32_t *material_ids;   // Material ID per triangle
    uint32_t *triangle_ids;   // Triangle ID per triangle
    
    // Original mesh data
    float *original_vertices;
    uint32_t *original_indices;
    float *original_normals;
    float *original_uvs;
    uint32_t original_vertex_count;
    uint32_t original_index_count;
    
    // Mesh optimization
    bool is_optimized;
    bool has_vertex_cache;
    bool has_index_cache;
    
    char name[256];
    uint32_t mesh_id;
    
    bool initialized;
} LODMesh;

typedef struct {
    LODMesh *meshes[1024];
    uint32_t mesh_count;
    uint32_t mesh_capacity;
    
    // Global LOD configuration
    LODConfig configs[LOD_LEVEL_COUNT];
    float global_lod_bias;
    bool enable_lod_system;
    
    // Distance calculations
    float camera_position[3];
    float view_matrix[16];
    float proj_matrix[16];
    float fov_tangent;
    
    // Performance settings
    bool enable_mesh_optimization;
    bool enable_vertex_cache;
    bool enable_index_cache;
    uint32_t max_cache_size;
    
    // Statistics
    uint32_t total_lods_generated;
    uint32_t meshes_with_lods;
    uint32_t active_lods[LOD_LEVEL_COUNT];
    uint32_t lod_switches_frame;
    float lod_generation_time_ms;
    float lod_render_time_ms;
    
    // Thread safety
    pthread_mutex_t lod_mutex;
    
    bool initialized;
} LODSystem;

static LODSystem g_lod_system = {0};

// ============================================================================
// Mesh Optimization Algorithms
// ============================================================================

static void calculate_bounding_box(float *vertices, uint32_t vertex_count, float *min_bounds, float *max_bounds) {
    if (!vertices || vertex_count == 0) {
        return;
    }
    
    // Initialize bounds
    min_bounds[0] = vertices[0];
    min_bounds[1] = vertices[1];
    min_bounds[2] = vertices[2];
    max_bounds[0] = vertices[0];
    max_bounds[1] = vertices[1];
    max_bounds[2] = vertices[2];
    
    // Find min/max bounds
    for (uint32_t i = 1; i < vertex_count; i++) {
        float *vertex = &vertices[i * 3];
        
        min_bounds[0] = fminf(min_bounds[0], vertex[0]);
        min_bounds[1] = fminf(min_bounds[1], vertex[1]);
        min_bounds[2] = fminf(min_bounds[2], vertex[2]);
        
        max_bounds[0] = fmaxf(max_bounds[0], vertex[0]);
        max_bounds[1] = fmaxf(max_bounds[1], vertex[1]);
        max_bounds[2] = fmaxf(max_bounds[2], vertex[2]);
    }
}

static void simplify_mesh_edge_collapse(float *vertices, uint32_t *indices, 
                                 uint32_t *vertex_count, uint32_t *index_count,
                                 float reduction_factor) {
    if (!vertices || !indices || reduction_factor <= 0.0f || reduction_factor >= 1.0f) {
        return;
    }
    
    uint32_t target_triangles = (uint32_t)((*index_count / 3) * reduction_factor);
    uint32_t current_triangles = *index_count / 3;
    
    if (target_triangles >= current_triangles) {
        return; // No simplification needed
    }
    
    // Simple edge collapse algorithm
    // TODO: Implement proper mesh simplification
    // This would involve:
    // 1. Calculate edge costs
    // 2. Collapse cheapest edges
    // 3. Update connectivity
    // 4. Preserve texture coordinates and normals
    
    LOG_DEBUG("Mesh simplification: %u -> %u triangles (%.2f reduction)",
             current_triangles, target_triangles, reduction_factor);
}

static void optimize_vertex_cache(float *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count,
                                uint32_t cache_size) {
    // TODO: Implement vertex cache optimization
    // This would reorder vertices for better cache locality
    // based on triangle strip order or other heuristics
    
    LOG_DEBUG("Vertex cache optimization: %u vertices, cache size: %u", vertex_count, cache_size);
}

static void optimize_index_cache(uint32_t *indices, uint32_t index_count, uint32_t cache_size) {
    // TODO: Implement index cache optimization
    // This would reorder indices for better GPU cache performance
    // using algorithms like Forsyth's or tipsy
    
    LOG_DEBUG("Index cache optimization: %u indices, cache size: %u", index_count, cache_size);
}

// ============================================================================
// LOD Generation
// ============================================================================

static void generate_lod_mesh(LODMesh *lod_mesh, LODLevel level, float reduction_factor) {
    if (!lod_mesh || !lod_mesh->original_vertices || !lod_mesh->original_indices) {
        return;
    }
    
    uint32_t target_triangles = (uint32_t)(lod_mesh->original_triangle_count * reduction_factor);
    
    if (target_triangles == 0) {
        target_triangles = 1; // Always keep at least one triangle
    }
    
    // Calculate target vertex and index counts
    uint32_t target_vertices = target_triangles * 3;
    uint32_t target_indices = target_triangles * 3;
    
    // Allocate LOD mesh data
    float *lod_vertices = malloc(target_vertices * 3 * sizeof(float));
    uint32_t *lod_indices = malloc(target_indices * sizeof(uint32_t));
    
    if (!lod_vertices || !lod_indices) {
        LOG_ERROR("Failed to allocate LOD mesh data");
        return;
    }
    
    // Simple vertex copying and downsampling
    uint32_t vertex_stride = 0;
    uint32_t index_stride = 0;
    
    for (uint32_t i = 0; i < lod_mesh->original_index_count && index_stride < target_indices; i += 3) {
        uint32_t original_index = lod_mesh->original_indices[i];
        
        // Copy vertex data
        if (original_index < lod_mesh->original_vertex_count) {
            lod_vertices[vertex_stride + 0] = lod_mesh->original_vertices[original_index * 3 + 0];
            lod_vertices[vertex_stride + 1] = lod_mesh->original_vertices[original_index * 3 + 1];
            lod_vertices[vertex_stride + 2] = lod_mesh->original_vertices[original_index * 3 + 2];
            vertex_stride++;
        }
        
        // Copy index data
        lod_indices[index_stride++] = vertex_stride - 3;
        lod_indices[index_stride++] = vertex_stride - 2;
        lod_indices[index_stride++] = vertex_stride - 1;
    }
    
    // Create GPU resources for LOD mesh
    // TODO: Create vertex and index buffers
    lod_mesh->lod_meshes[level] = create_vertex_buffer(lod_vertices, vertex_stride * 3 * sizeof(float));
    lod_mesh->lod_vertex_counts[level] = vertex_stride;
    lod_mesh->lod_triangle_counts[level] = index_stride / 3;
    
    lod_mesh->lod_generated[level] = true;
    
    free(lod_vertices);
    free(lod_indices);
    
    LOG_DEBUG("Generated LOD %d for mesh '%s': %u vertices, %u triangles (%.2f reduction)",
             (int)level, lod_mesh->name, vertex_stride, index_stride / 3, reduction_factor);
}

static void generate_all_lods(LODMesh *mesh) {
    if (!mesh || !mesh->original_vertices) return;
    
    for (int level = 0; level < LOD_LEVEL_COUNT; level++) {
        if (!mesh->configs[level].enabled) {
            mesh->lod_generated[level] = false;
            continue;
        }
        
        float reduction = mesh->configs[level].reduction;
        generate_lod_mesh(mesh, (LODLevel)level, reduction);
    }
    
    mesh->meshes_with_lods = true;
    mesh->total_lods_generated = LOD_LEVEL_COUNT;
    
    LOG_INFO("Generated all LODs for mesh '%s'", mesh->name);
}

// ============================================================================
// LOD System API
// ============================================================================

bool lod_system_init(bool enable_optimization, bool enable_caching, uint32_t cache_size) {
    if (g_lod_system.initialized) {
        LOG_WARN("LOD system already initialized");
        return true;
    }
    
    memset(&g_lod_system, 0, sizeof(LODSystem));
    
    g_lod_system.enable_lod_system = true;
    g_lod_system.enable_mesh_optimization = enable_optimization;
    g_lod_system.enable_vertex_cache = enable_caching;
    g_lod_system.enable_index_cache = enable_caching;
    g_lod_system.max_cache_size = cache_size;
    
    g_lod_system.mesh_capacity = 1024;
    g_lod_system.meshes = calloc(g_lod_system.mesh_capacity, sizeof(LODMesh*));
    
    if (!g_lod_system.meshes) {
        LOD_ERROR("Failed to allocate LOD mesh array");
        return false;
    }
    
    // Initialize LOD configurations
    for (int level = 0; level < LOD_LEVEL_COUNT; level++) {
        g_lod_system.configs[level].distance = 10.0f * (float)(level + 1);
        g_lod_system.configs[level].reduction = 0.5f * (float)(level + 1);
        g_lod_system.configs[level].screen_size = 50.0f / (float)(level + 1);
        g_l_system.configs[level].enabled = true;
    }
    
    g_lod_system.global_lod_bias = 0.0f;
    
    // Initialize mutex
    if (pthread_mutex_init(&g_lod_system.lod_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize LOD system mutex");
        free(g_lod_system.meshes);
        return false;
    }
    
    g_lod_system.initialized = true;
    LOG_INFO("LOD system initialized (optimization: %s, caching: %s, cache size: %u)",
             enable_optimization ? "yes" : "no",
             enable_caching ? "yes" : "no", cache_size);
    return true;
}

void lod_system_shutdown(void) {
    if (!g_lod_system.initialized)
        return;
    
    LOG_INFO("Shutting down LOD system");
    
    // Destroy all LOD meshes
    for (uint32_t i = 0; i < g_lod_system.mesh_count; i++) {
        lod_mesh_destroy(g_lod_system.meshes[i]);
    }
    
    // Free memory
    free(g_lod_system.meshes);
    
    // Destroy mutex
    pthread_mutex_destroy(&g_lod_system.lod_mutex);
    
    memset(&g_lod_system, 0, sizeof(LODSystem));
    
    LOG_INFO("LOD system shutdown complete");
}

LODMesh* lod_mesh_create(const char *name, float *vertices, uint32_t vertex_count,
                     float *indices, uint32_t index_count, float *normals, float *uvs) {
    if (!g_lod_system.initialized || !name) {
        LOG_ERROR("LOD system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_lod_system.lod_mutex);
    
    if (g_lod_system.mesh_count >= g_lod_system.mesh_capacity) {
        LOG_ERROR("Too many LOD meshes");
        pthread_mutex_unlock(&g_lod_system.lod_mutex);
        return NULL;
    }
    
    LODMesh *mesh = calloc(1, sizeof(LODMesh));
    if (!mesh) {
        LOG_ERROR("Failed to allocate LOD mesh");
        pthread_mutex_unlock(&g_lod_system.lod_system.mesh_capacity);
        return NULL;
    }
    
    strncpy(mesh->name, name, sizeof(mesh->name) - 1);
    mesh->mesh_id = g_lod_system.mesh_count++;
    
    // Store original mesh data
    mesh->original_vertices = malloc(vertex_count * 3 * sizeof(float));
    mesh->original_indices = malloc(index_count * sizeof(uint32_t));
    mesh->original_normals = normals ? malloc(vertex_count * 3 * sizeof(float)) : NULL;
    mesh->original_uvs = uvs ? malloc(vertex_count * 2 * sizeof(float)) : NULL;
    
    if (!mesh->original_vertices || !mesh->original_indices) {
        LOG_ERROR("Failed to allocate original mesh data");
        free(mesh);
        pthread_mutex_unlock(&g_lod_system.lod_mutex);
        return NULL;
    }
    
    // Copy original mesh data
    memcpy(mesh->original_vertices, vertices, vertex_count * 3 * sizeof(float));
    memcpy(mesh->original_indices, indices, index_count * sizeof(uint32_t));
    
    if (normals) {
        memcpy(mesh->original_normals, normals, vertex_count * 3 * sizeof(float));
    }
    
    if (uvs) {
        memcpy(mesh->original_uvs, uvs, vertex_count * 2 * sizeof(float));
    }
    
    mesh->original_vertex_count = vertex_count;
    mesh->original_index_count = index_count;
    mesh->triangle_count = index_count / 3;
    
    // Calculate bounding box and radius
    mesh->bounding_boxes = malloc(LOD_LEVEL_COUNT * 6 * sizeof(float));
    calculate_bounding_box(vertices, vertex_count, 
                       &mesh->bounding_boxes[0], &mesh->bounding_boxes[3]);
    
    mesh->bounding_radius = 0.0f;
    for (int i = 0; i < 3; i++) {
        float extent = mesh->bounding_boxes[i + 3] - mesh->bounding_boxes[i];
        mesh->bounding_radius = fmaxf(mesh->bounding_radius, extent);
    }
    
    // Generate LOD meshes
    generate_all_lods(mesh);
    
    // Add to system
    g_lod_system.meshes[g_lod_system.mesh_count++] = mesh;
    
    g_lod_system.meshes_with_lods++;
    
    pthread_mutex_unlock(&g_lod_system.lod_mutex);
    
    LOG_INFO("Created LOD mesh: %s (%u vertices, %u triangles)", name, vertex_count, mesh->triangle_count);
    return mesh;
}

void lod_mesh_destroy(LODMesh *mesh) {
    if (!mesh) return;
    
    pthread_mutex_lock(&g_lod_system.lod_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_lod_system.mesh_count; i++) {
        if (g_lod_system.meshes[i] == mesh) {
            g_lod_system.meshes[i] = g_lod_system.meshes[g_lod_system.mesh_count - 1];
            g_lod_system.mesh_count--;
            break;
        }
    }
    
    // Destroy all LOD meshes
    for (int level = 0; level < LOD_LEVEL_COUNT; level++) {
        if (mesh->lod_meshes[level]) {
            // TODO: Destroy GPU resources
            // destroy_vertex_buffer(mesh->lod_meshes[level]);
            mesh->lod_meshes[level] = NULL;
        }
    }
    
    // Free original data
    free(mesh->original_vertices);
    free(mesh->original_indices);
    free(mesh->original_normals);
    free(mesh->original_uvs);
    free(mesh->bounding_boxes);
    
    free(mesh);
    
    pthread_mutex_unlock(&god_system.lod_mutex);
    
    LOG_DEBUG("Destroyed LOD mesh: %s", mesh->name);
}

void lod_system_update_camera(const float *camera_pos, const float *view_matrix, const float *proj_matrix) {
    if (!g_lod_system.initialized) return;
    
    pthread_mutex_lock(&g_lod_system.lod_mutex);
    
    // Update camera position
    g_lod_system.camera_position[0] = camera_pos[0];
    g_lod_system.camera_position[1] = camera_pos[1];
    g_lod_system.camera_position[2] = camera_pos[2];
    
    // Update matrices
    memcpy(g_lod_system.view_matrix, view_matrix, 16 * sizeof(float));
    memcpy(g_lod_system.proj_matrix, proj_matrix, 16 * sizeof(float));
    
    // Calculate FOV tangent for screen size calculations
    float fov = 2.0f / (proj_matrix[5] + 1.0f); // Assuming perspective projection
    g_lod_system.fov_tangent = tanf(fov * 0.5f);
    
    pthread_mutex_unlock(&g_lod_system.lod_mutex);
}

LODLevel lod_system_calculate_lod_level(const float *object_position, float object_radius, const float *camera_pos) {
    if (!g_lod_system.initialized || !object_position || !camera_pos) {
        return LOD_LEVEL_0;
    }
    
    float distance = sqrtf(
        (object_position[0] - camera_pos[0]) * (object_position[0] - camera_pos[0]) +
        (object_position[1] - camera_pos[1]) * (object_position[1] - camera_pos[1]) +
        (object_position[2] - camera_pos[2]) * (object_position[2] - camera_pos[2])
    );
    
    // Apply global LOD bias
    distance *= (1.0f + g_lod_system.global_lod_bias);
    
    // Calculate screen size
    float screen_size = (2.0f * object_radius) / (distance * g_l_system.fov_tangent);
    
    // Find appropriate LOD level based on distance and screen size
    for (int level = LOD_LEVEL_0; level < LOD_LEVEL_COUNT; level++) {
        const LODConfig *config = &g_lod_system.configs[level];
        
        if (!config->enabled) continue;
        
        if (distance > config->distance || screen_size < config->screen_size) {
            return (LODLevel)level;
        }
    }
    
    return LOD_LEVEL_0; // Default to highest quality
}

void lod_system_set_lod_bias(float bias) {
    if (!g_lod_system.initialized) return;
    
    g_lod_system.global_lod_bias = bias;
    LOG_DEBUG("LOD bias set to %.2f", bias);
}

void lod_system_set_lod_config(LODLevel level, float distance, float reduction, float screen_size, bool enabled) {
    if (!g_lod_system.initialized || level >= LOD_LEVEL_COUNT) return;
    
    pthread_mutex_lock(&g_lod_system.lod_mutex);
    
    g_lod_system.configs[level].distance = distance;
    g_lod_system.configs[level].reduction = reduction;
    g_lod_system.configs[level].screen_size = screen_size;
    g_lod_system.configs[level].enabled = enabled;
    
    // Regenerate all LOD meshes with new configuration
    for (uint32_t i = 0; i < g_lod_system.mesh_count; i++) {
        LODMesh *mesh = g_lod_system.meshes[i];
        if (mesh->meshes_with_lods) {
            generate_lod_mesh(mesh, level, g_l_system.configs[level].reduction);
        }
    }
    
    pthread_mutex_unlock(&g_system.lod_mutex);
    
    LOG_DEBUG("LOD config updated for level %d: distance=%.1f, reduction=%.2f, screen_size=%.1f, enabled=%s",
             (int)level, distance, reduction, screen_size, enabled ? "yes" : "no");
}

void lod_system_get_stats(uint32_t *total_meshes, uint32_t *meshes_with_lods,
                           uint32_t *active_lods, uint32_t *total_lods_generated,
                           float *generation_time, float *render_time) {
    if (!g_lod_system.initialized) return;
    
    pthread_mutex_lock(&g_lod_system.lod_mutex);
    
    if (total_meshes) *total_meshes = g_lod_system.mesh_count;
    if (meshes_with_lods) *meshes_with_lods = g_lod_system.meshes_with_lods;
    if (active_lods) {
        for (int level = 0; level < LOD_LEVEL_COUNT; level++) {
            active_lods[level] = g_lod_system.active_lods[level];
        }
    }
    if (total_lods_generated) *total_lods_generated = g_lod_system.total_lods_generated;
    if (generation_time) *generation_time = g_lod_system.lod_generation_time_ms;
    if (render_time) *render_time = g_lod_system.lod_render_time_ms;
    
    pthread_mutex_unlock(&g_lod_system.lod_mutex);
}

void lod_system_print_stats(void) {
    if (!g_lod_system.initialized) return;
    
    pthread_mutex_lock(&g_lod_system.lod_system.lod_mutex);
    
    LOG_INFO("=== LOD System Statistics ===");
    LOG_INFO("Total meshes: %u", g_lod_system.mesh_count);
    LOG_INFO("Meshes with LODs: %u", g_lod_system.meshes_with_lods);
    LOG_INFO("Total LODs generated: %u", g_lod_system.total_lods_generated);
    
    LOG_INFO("Active LODs by level:");
    for (int level = 0; level < LOD_LEVEL_COUNT; level++) {
        const LODConfig *config = &g_lod_system.configs[level];
        LOG_INFO("  Level %d: %u active (%s)", level, g_lod_system.active_lods[level],
                 config->enabled ? "enabled" : "disabled");
    }
    
    LOG_INFO("Performance:");
    LOG_INFO("  LOD generation time: %.2f ms", g_lod_system.lod_generation_time_ms);
    LOG_INFO("  LOD render time: %.2f ms", g_lod_system.lod_render_time_ms);
    LOG_INFO("  LOD switches per frame: %u", g_lod_system.lod_switches_frame);
    
    pthread_mutex_unlock(&g_lod_system.lod_mutex);
}

bool lod_system_is_initialized(void) {
    return g_lod_system.initialized;
}
