// environment/vegetation/foliage_system.h
// GPU-instanced procedural foliage with LOD
#ifndef FOLIAGE_SYSTEM_H
#define FOLIAGE_SYSTEM_H

#include <common.h>
#include "math/vec3.h"
#include <Metal/Metal.h>

#define FOLIAGE_MAX_TYPES 32
#define FOLIAGE_INSTANCES_PER_CELL 10000

// Foliage type (grass, tree, rock, etc.)
typedef struct {
    char name[64];
    id<MTLBuffer> mesh_vertices;
    id<MTLBuffer> mesh_indices;
    u32 vertex_count;
    u32 index_count;
    
    // LOD meshes
    id<MTLBuffer> lod_meshes[4];
    f32 lod_distances[4];
    
    // Material
    id<MTLTexture> albedo;
    id<MTLTexture> normal;
    f32 wind_strength;
    f32 sss_strength;  // Subsurface scattering
    
    // Placement rules
    f32 density;       // Instances per m
    f32 min_slope;
    f32 max_slope;
    f32 min_height;
    f32 max_height;
    f32 scale_min;
    f32 scale_max;
    
} FoliageType;

// Spatial cell for culling
typedef struct {
    Vec3 center;
    f32 radius;
    u32 instance_offset;
    u32 instance_count;
} FoliageCell;

// Instance data (GPU format)
typedef struct {
    Mat4 transform;
    f32 wind_phase;
    u32 lod_level;
} FoliageInstance;

typedef struct {
    FoliageType types[FOLIAGE_MAX_TYPES];
    u32 type_count;
    
    // Instance data (all types combined)
    FoliageInstance* instances;
    u32 instance_count;
    u32 max_instances;
    
    // Spatial partitioning
    FoliageCell* cells;
    u32 cell_count;
    f32 cell_size;
    
    // GPU buffers
    id<MTLBuffer> instance_buffer;
    id<MTLBuffer> culled_instances;
    id<MTLBuffer> indirect_args;
    
    // Wind simulation
    f32 wind_time;
    Vec3 wind_direction;
    f32 wind_speed;
    
} FoliageSystem;

#ifdef __cplusplus
extern "C" {
#endif

FoliageSystem* foliage_create(id<MTLDevice> device, f32 world_size, f32 cell_size);

// Add foliage types
void foliage_add_type(FoliageSystem* foliage, const char* name,
                     id<MTLBuffer> mesh, u32 vertex_count, u32 index_count);

// Procedural placement
void foliage_place_on_terrain(FoliageSystem* foliage,
                              const void* terrain,
                              u32 foliage_type_id);

// Per-frame update
void foliage_update(FoliageSystem* foliage, f32 delta_time);
void foliage_cull_and_lod(FoliageSystem* foliage,
                         id<MTLCommandBuffer> cmd,
                         const Vec3* camera_pos,
                         const Mat4* view_proj);

// Render all visible instances
void foliage_render(FoliageSystem* foliage,
                   id<MTLRenderCommandEncoder> encoder,
                   const Mat4* view_proj);

void foliage_destroy(FoliageSystem* foliage);

#ifdef __cplusplus
}
#endif

#endif // FOLIAGE_SYSTEM_H
