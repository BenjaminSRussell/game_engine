/**
 * @file voxel_mesher.c
 * @brief Voxel Terrain Mesh Generator with Greedy Meshing Algorithm
 *
 * Implements optimized greedy meshing for blocky voxel terrain.
 * Merges adjacent voxel faces to reduce triangle/vertex count dramatically.
 * Target: ~90% reduction in mesh size compared to naive approach.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <terrain/core/voxel_mesher.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Chunk dimensions
#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256
#define MAX_VERTICES (CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 24) // 6 faces * 4 verts
#define MAX_INDICES (CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT * 36)  // 6 faces * 6 indices

// Face directions
typedef enum {
    FACE_FRONT = 0,
    FACE_BACK,
    FACE_LEFT,
    FACE_RIGHT,
    FACE_TOP,
    FACE_BOTTOM
} FaceDirection;

// Vertex format for voxel meshes
typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
    uint8_t color[4];
} VoxelVertex;

// Mesh output structure
typedef struct {
    VoxelVertex *vertices;
    uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t max_vertices;
    uint32_t max_indices;
} VoxelMeshData;

// Direction vectors for each face
static const int face_normals[6][3] = {
    {0, 0, 1},   // FRONT
    {0, 0, -1},  // BACK
    {-1, 0, 0},  // LEFT
    {1, 0, 0},   // RIGHT
    {0, 1, 0},   // TOP
    {0, -1, 0}   // BOTTOM
};

// UV coordinates for quad (0,0) -> (1,1)
static const float quad_uvs[4][2] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

/**
 * Get voxel block type at position (returns 0 for air)
 */
static inline uint8_t get_voxel(const uint8_t *voxels, int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return 0; // Air outside chunk bounds
    }
    return voxels[x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE];
}

/**
 * Check if face should be rendered (neighbor is transparent/air)
 */
static inline bool should_render_face(const uint8_t *voxels, int x, int y, int z, FaceDirection face) {
    int nx = x + face_normals[face][0];
    int ny = y + face_normals[face][1];
    int nz = z + face_normals[face][2];
    
    uint8_t neighbor = get_voxel(voxels, nx, ny, nz);
    return neighbor == 0; // Render if neighbor is air
}

/**
 * Add quad to mesh (4 vertices, 6 indices forming 2 triangles)
 */
static void add_quad(VoxelMeshData *mesh, 
                     float x, float y, float z,
                     int width, int height,
                     FaceDirection face,
                     uint8_t block_type) {
    
    if (mesh->vertex_count + 4 > mesh->max_vertices || 
        mesh->index_count + 6 > mesh->max_indices) {
        return; // Mesh buffer full
    }
    
    uint32_t base_vertex = mesh->vertex_count;
    VoxelVertex *verts = &mesh->vertices[base_vertex];
    
    // Calculate quad corners based on face direction and dimensions
    float positions[4][3];
    
    switch (face) {
        case FACE_FRONT: // +Z
            positions[0][0] = x;         positions[0][1] = y;          positions[0][2] = z + 1;
            positions[1][0] = x + width; positions[1][1] = y;          positions[1][2] = z + 1;
            positions[2][0] = x + width; positions[2][1] = y + height; positions[2][2] = z + 1;
            positions[3][0] = x;         positions[3][1] = y + height; positions[3][2] = z + 1;
            break;
        case FACE_BACK: // -Z
            positions[0][0] = x + width; positions[0][1] = y;          positions[0][2] = z;
            positions[1][0] = x;         positions[1][1] = y;          positions[1][2] = z;
            positions[2][0] = x;         positions[2][1] = y + height; positions[2][2] = z;
            positions[3][0] = x + width; positions[3][1] = y + height; positions[3][2] = z;
            break;
        case FACE_LEFT: // -X
            positions[0][0] = x; positions[0][1] = y;          positions[0][2] = z;
            positions[1][0] = x; positions[1][1] = y;          positions[1][2] = z + width;
            positions[2][0] = x; positions[2][1] = y + height; positions[2][2] = z + width;
            positions[3][0] = x; positions[3][1] = y + height; positions[3][2] = z;
            break;
        case FACE_RIGHT: // +X
            positions[0][0] = x + 1; positions[0][1] = y;          positions[0][2] = z + width;
            positions[1][0] = x + 1; positions[1][1] = y;          positions[1][2] = z;
            positions[2][0] = x + 1; positions[2][1] = y + height; positions[2][2] = z;
            positions[3][0] = x + 1; positions[3][1] = y + height; positions[3][2] = z + width;
            break;
        case FACE_TOP: // +Y
            positions[0][0] = x;         positions[0][1] = y + 1; positions[0][2] = z + height;
            positions[1][0] = x + width; positions[1][1] = y + 1; positions[1][2] = z + height;
            positions[2][0] = x + width; positions[2][1] = y + 1; positions[2][2] = z;
            positions[3][0] = x;         positions[3][1] = y + 1; positions[3][2] = z;
            break;
        case FACE_BOTTOM: // -Y
            positions[0][0] = x;         positions[0][1] = y; positions[0][2] = z;
            positions[1][0] = x + width; positions[1][1] = y; positions[1][2] = z;
            positions[2][0] = x + width; positions[2][1] = y; positions[2][2] = z + height;
            positions[3][0] = x;         positions[3][1] = y; positions[3][2] = z + height;
            break;
    }
    
    // Create vertices
    for (int i = 0; i < 4; i++) {
        verts[i].position[0] = positions[i][0];
        verts[i].position[1] = positions[i][1];
        verts[i].position[2] = positions[i][2];
        
        verts[i].normal[0] = (float)face_normals[face][0];
        verts[i].normal[1] = (float)face_normals[face][1];
        verts[i].normal[2] = (float)face_normals[face][2];
        
        verts[i].uv[0] = quad_uvs[i][0] * width;
        verts[i].uv[1] = quad_uvs[i][1] * height;
        
        // Simple block coloring based on type
        verts[i].color[0] = block_type * 40;
        verts[i].color[1] = 128;
        verts[i].color[2] = 64;
        verts[i].color[3] = 255;
    }
    
    // Add indices (two triangles)
    uint32_t *indices = &mesh->indices[mesh->index_count];
    indices[0] = base_vertex + 0;
    indices[1] = base_vertex + 1;
    indices[2] = base_vertex + 2;
    indices[3] = base_vertex + 0;
    indices[4] = base_vertex + 2;
    indices[5] = base_vertex + 3;
    
    mesh->vertex_count += 4;
    mesh->index_count += 6;
}

/**
 * Greedy meshing algorithm for a single layer
 * Merges adjacent faces of same type into larger quads
 */
static void greedy_mesh_layer(const uint8_t *voxels, VoxelMeshData *mesh,
                               int layer_coord, FaceDirection face) {
    
    // Determine which axes to iterate based on face direction
    int axis_u, axis_v, axis_n; // u=horizontal, v=vertical, n=normal
    int size_u, size_v;
    
    switch (face) {
        case FACE_FRONT:
        case FACE_BACK:
            axis_u = 0; axis_v = 1; axis_n = 2; // XY plane
            size_u = CHUNK_SIZE; size_v = CHUNK_HEIGHT;
            break;
        case FACE_LEFT:
        case FACE_RIGHT:
            axis_u = 2; axis_v = 1; axis_n = 0; // ZY plane
            size_u = CHUNK_SIZE; size_v = CHUNK_HEIGHT;
            break;
        case FACE_TOP:
        case FACE_BOTTOM:
            axis_u = 0; axis_v = 2; axis_n = 1; // XZ plane
            size_u = CHUNK_SIZE; size_v = CHUNK_SIZE;
            break;
        default:
            return;
    }
    
    // Mask to track which voxels have been merged
    bool merged[CHUNK_SIZE * CHUNK_HEIGHT];
    memset(merged, 0, sizeof(merged));
    
    // Voxel type mask for current layer
    uint8_t voxel_mask[CHUNK_SIZE * CHUNK_HEIGHT];
    
    // Build voxel mask for this layer
    for (int v = 0; v < size_v; v++) {
        for (int u = 0; u < size_u; u++) {
            int coords[3] = {0, 0, 0};
            coords[axis_u] = u;
            coords[axis_v] = v;
            coords[axis_n] = layer_coord;
            
            int x = coords[0], y = coords[1], z = coords[2];
            uint8_t voxel = get_voxel(voxels, x, y, z);
            
            if (voxel != 0 && should_render_face(voxels, x, y, z, face)) {
                voxel_mask[u + v * size_u] = voxel;
            } else {
                voxel_mask[u + v * size_u] = 0;
            }
        }
    }
    
    // Greedy meshing: merge adjacent quads
    for (int v = 0; v < size_v; v++) {
        for (int u = 0; u < size_u; u++) {
            int idx = u + v * size_u;
            
            if (merged[idx] || voxel_mask[idx] == 0) {
                continue;
            }
            
            uint8_t voxel_type = voxel_mask[idx];
            
            // Find maximum width (extend in u direction)
            int width = 1;
            while (u + width < size_u) {
                int next_idx = (u + width) + v * size_u;
                if (merged[next_idx] || voxel_mask[next_idx] != voxel_type) {
                    break;
                }
                width++;
            }
            
            // Find maximum height (extend in v direction)
            int height = 1;
            bool can_extend = true;
            while (v + height < size_v && can_extend) {
                // Check entire row
                for (int du = 0; du < width; du++) {
                    int check_idx = (u + du) + (v + height) * size_u;
                    if (merged[check_idx] || voxel_mask[check_idx] != voxel_type) {
                        can_extend = false;
                        break;
                    }
                }
                if (can_extend) {
                    height++;
                }
            }
            
            // Mark all voxels in this quad as merged
            for (int dv = 0; dv < height; dv++) {
                for (int du = 0; du < width; du++) {
                    merged[(u + du) + (v + dv) * size_u] = true;
                }
            }
            
            // Add merged quad to mesh
            int coords[3] = {0, 0, 0};
            coords[axis_u] = u;
            coords[axis_v] = v;
            coords[axis_n] = layer_coord;
            
            add_quad(mesh, coords[0], coords[1], coords[2], 
                     axis_u == 0 ? width : 1,
                     axis_v == 1 ? height : axis_v == 2 ? width : 1,
                     face, voxel_type);
        }
    }
}

/**
 * Generate optimized mesh for entire voxel chunk using greedy meshing
 */
Mesh* generate_voxel_mesh(const uint8_t *voxels, vec3 chunk_pos) {
    // Allocate mesh data
    VoxelMeshData mesh_data = {0};
    mesh_data.max_vertices = MAX_VERTICES;
    mesh_data.max_indices = MAX_INDICES;
    mesh_data.vertices = (VoxelVertex*)malloc(sizeof(VoxelVertex) * mesh_data.max_vertices);
    mesh_data.indices = (uint32_t*)malloc(sizeof(uint32_t) * mesh_data.max_indices);
    
    if (!mesh_data.vertices || !mesh_data.indices) {
        free(mesh_data.vertices);
        free(mesh_data.indices);
        return NULL;
    }
    
    mesh_data.vertex_count = 0;
    mesh_data.index_count = 0;
    
    // Process each layer for each face direction
    for (int face = 0; face < 6; face++) {
        int layer_count = (face == FACE_TOP || face == FACE_BOTTOM) ? CHUNK_HEIGHT : CHUNK_SIZE;
        
        for (int layer = 0; layer < layer_count; layer++) {
            greedy_mesh_layer(voxels, \u0026mesh_data, layer, (FaceDirection)face);
        }
    }
    
    // Convert to engine Mesh format
    Mesh *output_mesh = (Mesh*)malloc(sizeof(Mesh));
    if (!output_mesh) {
        free(mesh_data.vertices);
        free(mesh_data.indices);
        return NULL;
    }
    
    output_mesh->vertices = (void*)mesh_data.vertices;
    output_mesh->indices = mesh_data.indices;
    output_mesh->vertex_count = mesh_data.vertex_count;
    output_mesh->index_count = mesh_data.index_count;
    output_mesh->position = chunk_pos;
    
    return output_mesh;
}

/**
 * Free mesh data
 */
void free_voxel_mesh(Mesh *mesh) {
    if (mesh) {
        free(mesh->vertices);
        free(mesh->indices);
        free(mesh);
    }
}
