#include "ocean_mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// =============================================================================
// Default Configuration
// =============================================================================

OceanMeshConfig ocean_mesh_default_config(void) {
    OceanMeshConfig config = {0};
    
    config.base_resolution = 128;
    config.lod_count = 3;
    config.lod_distances[0] = 500.0f;    // LOD 0: 0-500m
    config.lod_distances[1] = 2000.0f;   // LOD 1: 500-2000m
    config.lod_distances[2] = 10000.0f;  // LOD 2: 2000-10000m
    
    config.tile_size = 1000.0f;
    config.use_triangle_strips = true;
    
    return config;
}

// =============================================================================
// Mesh Generation Helpers
// =============================================================================

static bool ocean_mesh_generate_vertices(
    uint32_t resolution,
    float tile_size,
    float** out_vertices,
    uint32_t* out_vertex_count
) {
    // Generate flat grid in XZ plane
    // Vertices are positioned at [-tile_size/2, tile_size/2] in X and Z
    
    uint32_t grid_size = resolution + 1;  // +1 for inclusive endpoints
    uint32_t vertex_count = grid_size * grid_size;
    
    // Each vertex: (x, y, z) - y will be 0 (flat), displaced by shader
    float* vertices = (float*)malloc(vertex_count * 3 * sizeof(float));
    if (!vertices) {
        return false;
    }
    
    float half_tile = tile_size * 0.5f;
    float step = tile_size / (float)resolution;
    
    for (uint32_t z = 0; z < grid_size; z++) {
        for (uint32_t x = 0; x < grid_size; x++) {
            uint32_t index = z * grid_size + x;
            
            vertices[index * 3 + 0] = -half_tile + (float)x * step;  // X
            vertices[index * 3 + 1] = 0.0f;                          // Y (will be displaced)
            vertices[index * 3 + 2] = -half_tile + (float)z * step;  // Z
        }
    }
    
    *out_vertices = vertices;
    *out_vertex_count = vertex_count;
    
    return true;
}

static bool ocean_mesh_generate_indices(
    uint32_t resolution,
    bool use_triangle_strips,
    uint32_t** out_indices,
    uint32_t* out_index_count
) {
    uint32_t grid_size = resolution + 1;
    
    if (use_triangle_strips) {
        // TODO: Implement triangle strip generation
        // For now, fall through to indexed triangles
        use_triangle_strips = false;
    }
    
    // Indexed triangles: 2 triangles per quad
    uint32_t quad_count = resolution * resolution;
    uint32_t triangle_count = quad_count * 2;
    uint32_t index_count = triangle_count * 3;
    
    uint32_t* indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
    if (!indices) {
        return false;
    }
    
    uint32_t idx = 0;
    for (uint32_t z = 0; z < resolution; z++) {
        for (uint32_t x = 0; x < resolution; x++) {
            // Quad vertices:
            // v0 --- v1
            // |      |
            // v2 --- v3
            
            uint32_t v0 = z * grid_size + x;
            uint32_t v1 = z * grid_size + (x + 1);
            uint32_t v2 = (z + 1) * grid_size + x;
            uint32_t v3 = (z + 1) * grid_size + (x + 1);
            
            // Triangle 1: v0, v2, v1
            indices[idx++] = v0;
            indices[idx++] = v2;
            indices[idx++] = v1;
            
            // Triangle 2: v1, v2, v3
            indices[idx++] = v1;
            indices[idx++] = v2;
            indices[idx++] = v3;
        }
    }
    
    *out_indices = indices;
    *out_index_count = index_count;
    
    return true;
}

static bool ocean_mesh_create_lod(
    MTLDeviceRef device,
    OceanMeshLOD* lod,
    uint32_t resolution,
    float tile_size,
    bool use_triangle_strips
) {
    // Generate vertices
    float* vertices = NULL;
    uint32_t vertex_count = 0;
    
    if (!ocean_mesh_generate_vertices(resolution, tile_size, &vertices, &vertex_count)) {
        fprintf(stderr, "[OceanMesh] Error: Failed to generate vertices\n");
        return false;
    }
    
    // Generate indices
    uint32_t* indices = NULL;
    uint32_t index_count = 0;
    
    if (!ocean_mesh_generate_indices(resolution, use_triangle_strips, &indices, &index_count)) {
        fprintf(stderr, "[OceanMesh] Error: Failed to generate indices\n");
        free(vertices);
        return false;
    }
    
    printf("[OceanMesh] LOD: resolution=%u, vertices=%u, indices=%u\n",
           resolution, vertex_count, index_count);
    
    // TODO: Create Metal buffers
    // lod->vertex_buffer = create_metal_buffer(device, vertices, vertex_count * 3 * sizeof(float));
    // lod->index_buffer = create_metal_buffer(device, indices, index_count * sizeof(uint32_t));
    
    lod->vertex_buffer = NULL;  // Placeholder
    lod->index_buffer = NULL;   // Placeholder
    lod->vertex_count = vertex_count;
    lod->index_count = index_count;
    lod->resolution = resolution;
    
    // Free temporary CPU data
    free(vertices);
    free(indices);
    
    return true;
}

// =============================================================================
// Ocean Mesh Public API
// =============================================================================

OceanMesh* ocean_mesh_create(
    MTLDeviceRef device,
    const OceanMeshConfig* config
) {
    if (!device || !config) {
        fprintf(stderr, "[OceanMesh] Error: Invalid parameters\n");
        return NULL;
    }
    
    if (config->lod_count < 1 || config->lod_count > OCEAN_MESH_MAX_LOD_LEVELS) {
        fprintf(stderr, "[OceanMesh] Error: Invalid LOD count %u\n", config->lod_count);
        return NULL;
    }
    
    printf("[OceanMesh] Creating mesh: resolution=%u, LODs=%u, tile_size=%.1f\n",
           config->base_resolution, config->lod_count, config->tile_size);
    
    OceanMesh* mesh = (OceanMesh*)calloc(1, sizeof(OceanMesh));
    if (!mesh) {
        fprintf(stderr, "[OceanMesh] Error: Failed to allocate mesh\n");
        return NULL;
    }
    
    mesh->config = *config;
    mesh->device = device;
    mesh->lod_count = config->lod_count;
    mesh->active_lod_index = 0;
    
    // Create LOD levels with decreasing resolution
    for (uint32_t i = 0; i < config->lod_count; i++) {
        // Each LOD halves the resolution
        uint32_t lod_resolution = config->base_resolution >> i;
        
        if (lod_resolution < 4) {
            lod_resolution = 4;  // Minimum resolution
        }
        
        OceanMeshLOD* lod = &mesh->lods[i];
        
        if (!ocean_mesh_create_lod(device, lod, lod_resolution, config->tile_size, config->use_triangle_strips)) {
            fprintf(stderr, "[OceanMesh] Error: Failed to create LOD %u\n", i);
            ocean_mesh_destroy(mesh);
            return NULL;
        }
        
        // Set distance thresholds
        lod->min_distance = (i == 0) ? 0.0f : config->lod_distances[i - 1];
        lod->max_distance = (i < config->lod_count - 1) ? config->lod_distances[i] : 1e6f;
        
        printf("[OceanMesh] LOD %u: resolution=%u, distance=[%.1f, %.1f]\n",
               i, lod_resolution, lod->min_distance, lod->max_distance);
    }
    
    printf("[OceanMesh] Mesh created successfully\n");
    return mesh;
}

void ocean_mesh_destroy(OceanMesh* mesh) {
    if (!mesh) return;
    
    printf("[OceanMesh] Destroying mesh\n");
    
    // Release LOD resources
    for (uint32_t i = 0; i < mesh->lod_count; i++) {
        // TODO: Release Metal buffers
        // release_metal_buffer(mesh->lods[i].vertex_buffer);
        // release_metal_buffer(mesh->lods[i].index_buffer);
    }
    
    free(mesh);
}

uint32_t ocean_mesh_select_lod(
    OceanMesh* mesh,
    float camera_distance
) {
    if (!mesh) return 0;
    
    // Select LOD based on camera distance
    for (uint32_t i = 0; i < mesh->lod_count; i++) {
        if (camera_distance >= mesh->lods[i].min_distance &&
            camera_distance < mesh->lods[i].max_distance) {
            mesh->active_lod_index = i;
            return i;
        }
    }
    
    // Default to highest LOD (lowest detail)
    mesh->active_lod_index = mesh->lod_count - 1;
    return mesh->active_lod_index;
}

MTLBufferRef ocean_mesh_get_vertex_buffer(
    const OceanMesh* mesh,
    uint32_t lod_index
) {
    if (!mesh || lod_index >= mesh->lod_count) {
        return NULL;
    }
    
    return mesh->lods[lod_index].vertex_buffer;
}

MTLBufferRef ocean_mesh_get_index_buffer(
    const OceanMesh* mesh,
    uint32_t lod_index
) {
    if (!mesh || lod_index >= mesh->lod_count) {
        return NULL;
    }
    
    return mesh->lods[lod_index].index_buffer;
}

uint32_t ocean_mesh_get_index_count(
    const OceanMesh* mesh,
    uint32_t lod_index
) {
    if (!mesh || lod_index >= mesh->lod_count) {
        return 0;
    }
    
    return mesh->lods[lod_index].index_count;
}
