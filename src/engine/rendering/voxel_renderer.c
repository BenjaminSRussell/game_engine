// Voxel Renderer Implementation
// 3D voxel-based rendering system with block-based world representation

#include "voxel_renderer.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include "rendering/core/renderer.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include "engine/include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Chunk dimensions
#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 256
#define CHUNK_SIZE_Z 16
#define CHUNK_VOLUME (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)

// Maximum vertices per chunk (conservative estimate)
#define MAX_VERTICES_PER_CHUNK 65536
#define MAX_INDICES_PER_CHUNK 98304

// Block face directions
typedef enum {
    FACE_NEG_X = 0,
    FACE_POS_X = 1,
    FACE_NEG_Y = 2,
    FACE_POS_Y = 3,
    FACE_NEG_Z = 4,
    FACE_POS_Z = 5,
    FACE_COUNT
} BlockFace;

// Face vertices and normals
static const Vec3 FACE_NORMALS[FACE_COUNT] = {
    {-1.0f, 0.0f, 0.0f},  // NEG_X
    { 1.0f, 0.0f, 0.0f},  // POS_X
    { 0.0f,-1.0f, 0.0f},  // NEG_Y
    { 0.0f, 1.0f, 0.0f},  // POS_Y
    { 0.0f, 0.0f,-1.0f},  // NEG_Z
    { 0.0f, 0.0f, 1.0f}   // POS_Z
};

static const Vec2 FACE_UVS[4] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

static const u32 FACE_INDICES[6] = {
    0, 1, 2, 2, 3, 0  // Triangle strip for quad
};

// Block colors (temporary, should use textures)
static const Vec3 BLOCK_COLORS[BLOCK_TYPE_COUNT] = {
    {0.0f, 0.0f, 0.0f},  // AIR (transparent)
    {0.5f, 0.5f, 0.5f},  // STONE
    {0.1f, 0.8f, 0.2f},  // GRASS
    {0.5f, 0.3f, 0.1f},  // DIRT
    {0.6f, 0.4f, 0.2f},  // WOOD
    {0.2f, 0.6f, 0.1f},  // LEAVES
    {0.2f, 0.4f, 0.8f},  // WATER
    {0.8f, 0.8f, 0.4f},  // SAND
    {0.2f, 0.2f, 0.2f},  // COAL
    {0.7f, 0.7f, 0.7f},  // IRON
    {1.0f, 0.8f, 0.2f},  // GOLD
    {0.8f, 0.8f, 1.0f}   // DIAMOND
};

// Helper functions
static inline u32 get_chunk_index(u32 x, u32 y, u32 z) {
    return x + y * CHUNK_SIZE_X + z * CHUNK_SIZE_X * CHUNK_SIZE_Y;
}

static inline bool is_block_opaque(VoxelBlockType block_type) {
    return block_type != BLOCK_TYPE_AIR && block_type != BLOCK_TYPE_WATER;
}

static inline bool should_render_face(VoxelRenderer *renderer, VoxelChunk *chunk, 
                                  u32 x, u32 y, u32 z, BlockFace face) {
    // Check if adjacent block is opaque
    Vec3 offset = FACE_NORMALS[face];
    i32 nx = (i32)x + (i32)offset.x;
    i32 ny = (i32)y + (i32)offset.y;
    i32 nz = (i32)z + (i32)offset.z;
    
    // Check chunk boundaries
    if (nx < 0 || nx >= CHUNK_SIZE_X || 
        ny < 0 || ny >= CHUNK_SIZE_Y || 
        nz < 0 || nz >= CHUNK_SIZE_Z) {
        return true;  // Render face at chunk boundary
    }
    
    u32 neighbor_index = get_chunk_index((u32)nx, (u32)ny, (u32)nz);
    VoxelBlockType neighbor_block = chunk->blocks[neighbor_index];
    
    return !is_block_opaque(neighbor_block);
}

static u8 calculate_ao(VoxelRenderer *renderer, VoxelChunk *chunk, 
                       u32 x, u32 y, u32 z, BlockFace face) {
    // Simple ambient occlusion calculation
    // Check corners of the face
    u8 ao = 0;
    Vec3 normal = FACE_NORMALS[face];
    
    // Check adjacent blocks for AO
    for (int i = 0; i < 4; i++) {
        Vec3 corner_offset = {0};
        // Simplified AO calculation
        if (normal.x != 0) {
            corner_offset.x = normal.x;
            corner_offset.y = (i & 1) ? 0.5f : -0.5f;
            corner_offset.z = (i & 2) ? 0.5f : -0.5f;
        } else if (normal.y != 0) {
            corner_offset.x = (i & 1) ? 0.5f : -0.5f;
            corner_offset.y = normal.y;
            corner_offset.z = (i & 2) ? 0.5f : -0.5f;
        } else {
            corner_offset.x = (i & 1) ? 0.5f : -0.5f;
            corner_offset.y = (i & 2) ? 0.5f : -0.5f;
            corner_offset.z = normal.z;
        }
        
        i32 nx = (i32)x + (i32)corner_offset.x;
        i32 ny = (i32)y + (i32)corner_offset.y;
        i32 nz = (i32)z + (i32)corner_offset.z;
        
        if (nx >= 0 && nx < CHUNK_SIZE_X && 
            ny >= 0 && ny < CHUNK_SIZE_Y && 
            nz >= 0 && nz < CHUNK_SIZE_Z) {
            u32 neighbor_index = get_chunk_index((u32)nx, (u32)ny, (u32)nz);
            VoxelBlockType neighbor_block = chunk->blocks[neighbor_index];
            if (is_block_opaque(neighbor_block)) {
                ao++;
            }
        }
    }
    
    return 255 - (ao * 63);  // Scale AO to 0-255 range
}

// Create voxel renderer
VoxelRenderer *voxel_renderer_create(void) {
    VoxelRenderer *renderer = malloc(sizeof(VoxelRenderer));
    if (!renderer) {
        LOG_ERROR("Failed to allocate voxel renderer");
        return NULL;
    }
    
    memset(renderer, 0, sizeof(VoxelRenderer));
    
    // Initialize chunk array
    renderer->max_chunks = 1024;  // Support up to 1024 chunks
    renderer->chunks = malloc(sizeof(VoxelChunk) * renderer->max_chunks);
    if (!renderer->chunks) {
        LOG_ERROR("Failed to allocate chunk array");
        free(renderer);
        return NULL;
    }
    
    memset(renderer->chunks, 0, sizeof(VoxelChunk) * renderer->max_chunks);
    
    // Create block texture atlas (temporary 1x1 per block)
    TextureDesc texture_desc = {
        .width = BLOCK_TYPE_COUNT,
        .height = 1,
        .format = TEXTURE_FORMAT_RGB8,
        .usage = TEXTURE_USAGE_SAMPLED
    };
    
    renderer->block_texture = texture_create(&texture_desc);
    if (!texture_is_valid(renderer->block_texture)) {
        LOG_ERROR("Failed to create block texture");
        free(renderer->chunks);
        free(renderer);
        return NULL;
    }
    
    // Upload block colors as texture data
    u8 texture_data[BLOCK_TYPE_COUNT * 3];
    for (int i = 0; i < BLOCK_TYPE_COUNT; i++) {
        texture_data[i * 3 + 0] = (u8)(BLOCK_COLORS[i].x * 255.0f);
        texture_data[i * 3 + 1] = (u8)(BLOCK_COLORS[i].y * 255.0f);
        texture_data[i * 3 + 2] = (u8)(BLOCK_COLORS[i].z * 255.0f);
    }
    texture_upload(renderer->block_texture, texture_data);
    
    // Create uniform buffer
    BufferDesc uniform_desc = {
        .size = sizeof(Mat4) * 3,  // view, projection, view_projection
        .usage = BUFFER_USAGE_UNIFORM
    };
    
    renderer->uniform_buffer = buffer_create(&uniform_desc);
    if (!buffer_is_valid(renderer->uniform_buffer)) {
        LOG_ERROR("Failed to create uniform buffer");
        texture_destroy(renderer->block_texture);
        free(renderer->chunks);
        free(renderer);
        return NULL;
    }
    
    // Initialize matrices
    renderer->view_matrix = mat4_identity();
    renderer->projection_matrix = mat4_identity();
    renderer->view_projection_matrix = mat4_identity();
    
    renderer->initialized = true;
    LOG_INFO("Voxel renderer created successfully");
    
    return renderer;
}

// Destroy voxel renderer
void voxel_renderer_destroy(VoxelRenderer *renderer) {
    if (!renderer) return;
    
    // Destroy all chunks
    for (u32 i = 0; i < renderer->chunk_count; i++) {
        voxel_renderer_destroy_chunk(renderer, &renderer->chunks[i]);
    }
    
    // Destroy resources
    if (buffer_is_valid(renderer->uniform_buffer)) {
        buffer_destroy(renderer->uniform_buffer);
    }
    
    if (texture_is_valid(renderer->block_texture)) {
        texture_destroy(renderer->block_texture);
    }
    
    free(renderer->chunks);
    free(renderer);
    
    LOG_INFO("Voxel renderer destroyed");
}

// Create chunk
VoxelChunk *voxel_renderer_create_chunk(VoxelRenderer *renderer, const Vec3 *position, 
                                       u32 size_x, u32 size_y, u32 size_z) {
    if (renderer->chunk_count >= renderer->max_chunks) {
        LOG_ERROR("Maximum chunk count reached");
        return NULL;
    }
    
    VoxelChunk *chunk = &renderer->chunks[renderer->chunk_count];
    chunk->position = *position;
    chunk->size_x = size_x;
    chunk->size_y = size_y;
    chunk->size_z = size_z;
    
    // Allocate block data
    u32 block_count = size_x * size_y * size_z;
    chunk->blocks = malloc(sizeof(u16) * block_count);
    if (!chunk->blocks) {
        LOG_ERROR("Failed to allocate chunk block data");
        return NULL;
    }
    
    // Initialize to air
    memset(chunk->blocks, 0, sizeof(u16) * block_count);
    
    // Create vertex and index buffers
    BufferDesc vertex_desc = {
        .size = sizeof(VoxelVertex) * MAX_VERTICES_PER_CHUNK,
        .usage = BUFFER_USAGE_VERTEX
    };
    
    BufferDesc index_desc = {
        .size = sizeof(u32) * MAX_INDICES_PER_CHUNK,
        .usage = BUFFER_USAGE_INDEX
    };
    
    chunk->vertex_buffer = buffer_create(&vertex_desc);
    chunk->index_buffer = buffer_create(&index_desc);
    
    if (!buffer_is_valid(chunk->vertex_buffer) || !buffer_is_valid(chunk->index_buffer)) {
        LOG_ERROR("Failed to create chunk buffers");
        free(chunk->blocks);
        return NULL;
    }
    
    chunk->vertex_count = 0;
    chunk->index_count = 0;
    chunk->dirty = true;
    chunk->visible = false;
    
    renderer->chunk_count++;
    return chunk;
}

// Destroy chunk
void voxel_renderer_destroy_chunk(VoxelRenderer *renderer, VoxelChunk *chunk) {
    if (!chunk || !chunk->blocks) return;
    
    if (buffer_is_valid(chunk->vertex_buffer)) {
        buffer_destroy(chunk->vertex_buffer);
    }
    
    if (buffer_is_valid(chunk->index_buffer)) {
        buffer_destroy(chunk->index_buffer);
    }
    
    free(chunk->blocks);
    memset(chunk, 0, sizeof(VoxelChunk));
    
    if (renderer->chunk_count > 0) {
        renderer->chunk_count--;
    }
}

// Set block
void voxel_renderer_set_block(VoxelRenderer *renderer, VoxelChunk *chunk, 
                          u32 x, u32 y, u32 z, VoxelBlockType block_type) {
    if (!chunk || x >= chunk->size_x || y >= chunk->size_y || z >= chunk->size_z) {
        return;
    }
    
    u32 index = get_chunk_index(x, y, z);
    chunk->blocks[index] = (u16)block_type;
    chunk->dirty = true;
}

// Get block
VoxelBlockType voxel_renderer_get_block(VoxelRenderer *renderer, const VoxelChunk *chunk, 
                                    u32 x, u32 y, u32 z) {
    if (!chunk || x >= chunk->size_x || y >= chunk->size_y || z >= chunk->size_z) {
        return BLOCK_TYPE_AIR;
    }
    
    u32 index = get_chunk_index(x, y, z);
    return (VoxelBlockType)chunk->blocks[index];
}

// Rebuild chunk mesh
void voxel_renderer_rebuild_chunk_mesh(VoxelRenderer *renderer, VoxelChunk *chunk) {
    if (!chunk || !chunk->dirty) return;
    
    // Temporary vertex and index arrays
    VoxelVertex vertices[MAX_VERTICES_PER_CHUNK];
    u32 indices[MAX_INDICES_PER_CHUNK];
    u32 vertex_count = 0;
    u32 index_count = 0;
    
    // Generate mesh for each block
    for (u32 y = 0; y < chunk->size_y; y++) {
        for (u32 z = 0; z < chunk->size_z; z++) {
            for (u32 x = 0; x < chunk->size_x; x++) {
                u32 block_index = get_chunk_index(x, y, z);
                VoxelBlockType block_type = (VoxelBlockType)chunk->blocks[block_index];
                
                if (block_type == BLOCK_TYPE_AIR) continue;
                
                // Check each face
                for (int face = 0; face < FACE_COUNT; face++) {
                    if (!should_render_face(renderer, chunk, x, y, z, (BlockFace)face)) {
                        continue;
                    }
                    
                    // Add quad for this face
                    Vec3 block_pos = {
                        chunk->position.x + (f32)x,
                        chunk->position.y + (f32)y,
                        chunk->position.z + (f32)z
                    };
                    
                    Vec3 normal = FACE_NORMALS[face];
                    
                    // Calculate face vertices
                    for (int i = 0; i < 4; i++) {
                        if (vertex_count >= MAX_VERTICES_PER_CHUNK) break;
                        
                        VoxelVertex *vertex = &vertices[vertex_count++];
                        vertex->block_id = (u16)block_type;
                        vertex->normal = normal;
                        vertex->ao = calculate_ao(renderer, chunk, x, y, z, (BlockFace)face);
                        vertex->light = 255;  // Full brightness for now
                        
                        // Calculate vertex position
                        Vec3 offset = {0};
                        if (normal.x != 0) {
                            offset.x = normal.x * 0.5f;
                            offset.y = (i & 1) ? 0.5f : -0.5f;
                            offset.z = (i & 2) ? 0.5f : -0.5f;
                        } else if (normal.y != 0) {
                            offset.x = (i & 1) ? 0.5f : -0.5f;
                            offset.y = normal.y * 0.5f;
                            offset.z = (i & 2) ? 0.5f : -0.5f;
                        } else {
                            offset.x = (i & 1) ? 0.5f : -0.5f;
                            offset.y = (i & 2) ? 0.5f : -0.5f;
                            offset.z = normal.z * 0.5f;
                        }
                        
                        vertex->position = vec3_add(block_pos, offset);
                        vertex->uv = FACE_UVS[i];
                    }
                    
                    // Add indices for quad
                    if (index_count + 6 <= MAX_INDICES_PER_CHUNK) {
                        u32 base = vertex_count - 4;
                        for (int i = 0; i < 6; i++) {
                            indices[index_count++] = base + FACE_INDICES[i];
                        }
                    }
                }
            }
        }
    }
    
    // Upload to GPU buffers
    if (vertex_count > 0) {
        buffer_upload(chunk->vertex_buffer, vertices, sizeof(VoxelVertex) * vertex_count);
        buffer_upload(chunk->index_buffer, indices, sizeof(u32) * index_count);
    }
    
    chunk->vertex_count = vertex_count;
    chunk->index_count = index_count;
    chunk->dirty = false;
    
    LOG_DEBUG("Rebuilt chunk mesh: %d vertices, %d indices", vertex_count, index_count);
}

// Frustum culling (simplified)
bool voxel_renderer_is_chunk_visible(VoxelRenderer *renderer, const VoxelChunk *chunk) {
    // Simple distance-based culling for now
    Vec3 chunk_center = vec3_add(chunk->position, 
        vec3_create(chunk->size_x * 0.5f, chunk->size_y * 0.5f, chunk->size_z * 0.5f));
    
    f32 distance = vec3_distance(&renderer->camera_position, &chunk_center);
    f32 max_distance = 128.0f;  // Render distance
    
    return distance <= max_distance;
}

// Update renderer
void voxel_renderer_update(VoxelRenderer *renderer, float delta_time) {
    if (!renderer || !renderer->initialized) return;
    
    // Update camera matrices
    renderer->view_projection_matrix = mat4_multiply(&renderer->projection_matrix, &renderer->view_matrix);
    
    // Update uniform buffer
    Mat4 uniforms[3] = {
        renderer->view_matrix,
        renderer->projection_matrix,
        renderer->view_projection_matrix
    };
    buffer_upload(renderer->uniform_buffer, uniforms, sizeof(uniforms));
    
    // Update chunk visibility and rebuild dirty chunks
    renderer->chunks_rendered = 0;
    renderer->vertices_rendered = 0;
    renderer->triangles_rendered = 0;
    
    for (u32 i = 0; i < renderer->chunk_count; i++) {
        VoxelChunk *chunk = &renderer->chunks[i];
        
        // Update visibility
        chunk->visible = voxel_renderer_is_chunk_visible(renderer, chunk);
        
        // Rebuild dirty chunks
        if (chunk->dirty && chunk->visible) {
            voxel_renderer_rebuild_chunk_mesh(renderer, chunk);
        }
        
        // Update stats
        if (chunk->visible) {
            renderer->chunks_rendered++;
            renderer->vertices_rendered += chunk->vertex_count;
            renderer->triangles_rendered += chunk->index_count / 3;
        }
    }
}

// Render frame
void voxel_renderer_render(VoxelRenderer *renderer, const Mat4 *view, const Mat4 *projection) {
    if (!renderer || !renderer->initialized) return;
    
    // Update matrices
    renderer->view_matrix = *view;
    renderer->projection_matrix = *projection;
    
    // Bind resources and render visible chunks
    texture_bind(renderer->block_texture, 0);
    buffer_bind(renderer->uniform_buffer, BUFFER_TYPE_UNIFORM);
    
    for (u32 i = 0; i < renderer->chunk_count; i++) {
        VoxelChunk *chunk = &renderer->chunks[i];
        
        if (!chunk->visible || chunk->vertex_count == 0) {
            continue;
        }
        
        // Bind vertex and index buffers
        buffer_bind(chunk->vertex_buffer, BUFFER_TYPE_VERTEX);
        buffer_bind(chunk->index_buffer, BUFFER_TYPE_INDEX);
        
        // Draw chunk
        // This would call the actual rendering API (OpenGL/Vulkan/Metal)
        // draw_elements(chunk->index_count);
    }
}

// Get statistics
void voxel_renderer_get_stats(VoxelRenderer *renderer, u32 *chunks_rendered, 
                            u32 *vertices_rendered, u32 *triangles_rendered) {
    if (chunks_rendered) *chunks_rendered = renderer->chunks_rendered;
    if (vertices_rendered) *vertices_rendered = renderer->vertices_rendered;
    if (triangles_rendered) *triangles_rendered = renderer->triangles_rendered;
}
