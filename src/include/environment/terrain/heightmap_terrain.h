// environment/terrain/heightmap_terrain.h
// Advanced terrain system with runtime virtual texturing
#ifndef HEIGHTMAP_TERRAIN_H
#define HEIGHTMAP_TERRAIN_H

#include "include/common.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include <Metal/Metal.h>

#define TERRAIN_PATCH_SIZE 64      // Vertices per patch side
#define TERRAIN_MAX_LOD_LEVELS 8
#define TERRAIN_MAX_LAYERS 16      // Texture splatting layers

// Terrain material layer
typedef struct {
    char name[64];
    id<MTLTexture> albedo;
    id<MTLTexture> normal;
    id<MTLTexture> roughness;
    f32 tiling_scale;
    f32 blend_sharpness;
} TerrainLayer;

// Terrain patch (quadtree node)
typedef struct {
    Vec2 position;           // World space XZ
    f32 size;               // World space size
    u32 lod_level;
    
    Bounds bounds;
    f32 min_height;
    f32 max_height;
    
    id<MTLBuffer> heightmap_buffer;  // GPU heightmap
    id<MTLBuffer> normal_buffer;     // Precomputed normals
    id<MTLTexture> weight_map;       // Layer blend weights
} TerrainPatch;

// Main terrain system
typedef struct {
    // Heightmap data
    f32* heightmap;
    u32 heightmap_width;
    u32 heightmap_height;
    f32 height_scale;
    Vec3 world_offset;
    f32 world_scale;
    
    // Material layers
    TerrainLayer layers[TERRAIN_MAX_LAYERS];
    u32 layer_count;
    
    // LOD system
    TerrainPatch* patches;
    u32 patch_count;
    f32 lod_distances[TERRAIN_MAX_LOD_LEVELS];
    
    // Runtime virtual texturing
    id<MTLTexture> virtual_texture_cache;
    u32 cache_size;
    
    // GPU resources
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffers[TERRAIN_MAX_LOD_LEVELS];
    id<MTLComputePipelineState> tessellation_pipeline;
    
} TerrainSystem;

#ifdef __cplusplus
extern "C" {
#endif

// Terrain creation
TerrainSystem* terrain_create(id<MTLDevice> device,
                              u32 heightmap_width, u32 heightmap_height,
                              f32 world_scale, f32 height_scale);

// Load heightmap from image (16-bit grayscale)
void terrain_load_heightmap(TerrainSystem* terrain, const char* path);

// Procedural generation
void terrain_generate_perlin(TerrainSystem* terrain, u32 seed, u32 octaves, f32 persistence);
void terrain_apply_erosion(TerrainSystem* terrain, u32 iterations, f32 strength);

// Material layers
void terrain_add_layer(TerrainSystem* terrain, const char* name,
                       id<MTLTexture> albedo, id<MTLTexture> normal,
                       f32 tiling_scale);
void terrain_paint_layer(TerrainSystem* terrain, Vec2 world_pos, f32 radius, u32 layer_id, f32 strength);

// LOD management
void terrain_build_patches(TerrainSystem* terrain);
void terrain_update_lod(TerrainSystem* terrain, const Vec3* camera_pos);

// Rendering
void terrain_render(TerrainSystem* terrain,
                   id<MTLRenderCommandEncoder> encoder,
                   const Mat4* view_proj,
                   const Vec3* camera_pos);

// Terrain queries
f32 terrain_get_height(const TerrainSystem* terrain, f32 world_x, f32 world_z);
Vec3 terrain_get_normal(const TerrainSystem* terrain, f32 world_x, f32 world_z);

void terrain_destroy(TerrainSystem* terrain);

#ifdef __cplusplus
}
#endif

#endif // HEIGHTMAP_TERRAIN_H
