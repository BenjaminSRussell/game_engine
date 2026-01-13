#ifndef CLUSTER_BUILDER_H
#define CLUSTER_BUILDER_H

#include "../geometry_types.h"
#include <Metal/Metal.h>
#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>

#define CLUSTER_TRIANGLE_COUNT 128
#define CLUSTER_VERTEX_COUNT 256
#define MAX_CLUSTER_LEVELS 12
#define VSM_CASCADE_COUNT 4
#define MAX_MATERIAL_IDS 256
#define QUANTIZATION_BITS 16
#define LRU_CACHE_SIZE 1024
#define BVH_MAX_DEPTH 16
#define VISIBILITY_HISTORY_FRAMES 8

// Virtual Shadow Map structures
typedef struct {
    simd_float4x4 view_proj;
    simd_float4x4 shadow_matrix;
    float cascade_splits[VSM_CASCADE_COUNT];
    uint32_t resolution;
    id<MTLTexture> depth_textures[VSM_CASCADE_COUNT];
    id<MTLTexture> variance_textures[VSM_CASCADE_COUNT];
} virtual_shadow_map_t;

// Material quantization
typedef struct {
    uint16_t material_id;
    uint8_t material_count;
    uint8_t padding;
} cluster_material_info_t;

// BVH node for hierarchical culling
typedef struct bvh_node {
    simd_float3 bounds_min;
    uint32_t left_child;
    simd_float3 bounds_max;
    uint32_t right_child;
    uint32_t cluster_start;
    uint32_t cluster_count;
    uint32_t is_leaf;
} bvh_node_t;

// Quantized vertex for memory optimization
typedef struct {
    int16_t position[3];     // 16-bit quantized position
    uint16_t normal;         // 16-bit quantized normal (octahedral)
    uint16_t texcoord[2];    // 16-bit quantized UV
    uint8_t material_id;      // 8-bit material ID
    uint8_t padding[3];
} quantized_vertex_t;

// LRU cache entry for streaming
typedef struct {
    uint32_t cluster_id;
    uint32_t last_accessed_frame;
    uint32_t access_count;
    bool is_resident;
    uint8_t priority;
} lru_cache_entry_t;

// Spatio-temporal visibility tracking
typedef struct {
    uint32_t cluster_id;
    bool visible_history[VISIBILITY_HISTORY_FRAMES];
    uint32_t visibility_score;
    float last_distance;
    uint32_t frame_counter;
} visibility_history_t;

// Ray tracing BLAS data
typedef struct {
    id<MTLAccelerationStructure> blas;
    id<MTLBuffer> triangle_buffer;
    uint32_t triangle_count;
    uint32_t cluster_offset;
} cluster_blas_t;

// Animation data for D-Nanite
typedef struct {
    simd_float4x4 bone_transforms[256];
    uint32_t bone_count;
    uint32_t vertex_offset;
    uint32_t vertex_count;
} cluster_animation_data_t;

typedef struct mesh_cluster {
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t triangle_count;
    simd_float3 bounds_center;
    simd_float3 bounds_extent;
    float lod_error;         // Screen-space error threshold
    uint32_t parent_cluster; // For DAG
    uint32_t child_clusters[8];
    uint32_t child_count;
    
    // Enhanced features
    cluster_material_info_t material_info;
    uint32_t quantized_vertex_offset;
    uint32_t bvh_node_index;
    uint32_t visibility_history_index;
    uint32_t blas_index;
    uint32_t animation_data_index;
    bool has_displacement;
    bool is_animated;
    uint8_t lod_level;
    uint8_t padding[2];
} mesh_cluster_t;

typedef struct cluster_mesh {
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    id<MTLBuffer> cluster_buffer;
    id<MTLBuffer> quantized_vertex_buffer;
    id<MTLBuffer> bvh_buffer;
    id<MTLBuffer> lru_cache_buffer;
    id<MTLBuffer> visibility_history_buffer;
    id<MTLBuffer> blas_buffer;
    id<MTLBuffer> animation_buffer;
    
    uint32_t cluster_count;
    uint32_t total_triangles;
    uint32_t quantized_vertex_count;
    uint32_t bvh_node_count;
    uint32_t lru_cache_size;
    uint32_t visibility_history_count;
    uint32_t blas_count;
    uint32_t animation_data_count;
    
    // Streaming state
    uint32_t current_frame;
    uint32_t resident_cluster_count;
    float memory_budget_mb;
    
    // Shadow mapping
    virtual_shadow_map_t vsm;
    
    // LOD selection parameters
    float screen_error_threshold;
    float distance_scale;
    bool use_hierarchical_culling;
    bool use_virtual_shadow_maps;
} cluster_mesh_t;

// Build clusters from input mesh using meshoptimizer-style algorithm
cluster_mesh_t *cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t *vertices,
                                   uint32_t vertex_count,
                                   const uint32_t *indices,
                                   uint32_t index_count);

void cluster_mesh_free(cluster_mesh_t *mesh);

// Virtual Shadow Map functions
void cluster_mesh_init_virtual_shadow_maps(cluster_mesh_t *mesh, id<MTLDevice> device, uint32_t resolution);
void cluster_mesh_update_virtual_shadow_maps(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd, const simd_float4x4 *light_view_proj);
void cluster_mesh_render_shadow_cascades(cluster_mesh_t *mesh, id<MTLRenderCommandEncoder> encoder, uint32_t cascade_index);

// Vertex quantization functions
void cluster_mesh_quantize_vertices(cluster_mesh_t *mesh, const vertex_t *vertices, uint32_t vertex_count);
void cluster_mesh_dequantize_vertices(const cluster_mesh_t *mesh, vertex_t *out_vertices, uint32_t start_index, uint32_t count);

// BVH construction and culling
void cluster_mesh_build_bvh(cluster_mesh_t *mesh);
void cluster_mesh_cull_bvh_gpu(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd, const simd_float4x4 *view_proj, const simd_float3 *camera_pos);

// LRU streaming functions
void cluster_mesh_init_lru_cache(cluster_mesh_t *mesh, uint32_t cache_size, float memory_budget_mb);
void cluster_mesh_update_lru_cache(cluster_mesh_t *mesh, uint32_t frame);
bool cluster_mesh_is_cluster_resident(cluster_mesh_t *mesh, uint32_t cluster_id);
void cluster_mesh_mark_cluster_accessed(cluster_mesh_t *mesh, uint32_t cluster_id, uint32_t frame, uint8_t priority);

// Material quantization
void cluster_mesh_quantize_materials(cluster_mesh_t *mesh, const uint32_t *material_ids, uint32_t material_count);

// Visibility tracking
void cluster_mesh_init_visibility_history(cluster_mesh_t *mesh);
void cluster_mesh_update_visibility_history(cluster_mesh_t *mesh, const uint32_t *visible_clusters, uint32_t visible_count, uint32_t frame);
bool cluster_mesh_should_cull_temporal(cluster_mesh_t *mesh, uint32_t cluster_id, uint32_t frame);

// Ray tracing BLAS
void cluster_mesh_build_blas(cluster_mesh_t *mesh, id<MTLDevice> device);
void cluster_mesh_update_blas(cluster_mesh_t *mesh, id<MTLCommandBuffer> cmd);

// Animation support (D-Nanite)
void cluster_mesh_init_animation_data(cluster_mesh_t *mesh);
void cluster_mesh_update_animation(cluster_mesh_t *mesh, const cluster_animation_data_t *animation_data, uint32_t cluster_id);

// Displacement and tessellation
void cluster_mesh_enable_displacement(cluster_mesh_t *mesh, uint32_t cluster_id, bool enable);
void cluster_mesh_update_displacement(cluster_mesh_t *mesh, id<MTLTexture> heightmap, float strength);

// Geometry simplification for parent clusters
void cluster_mesh_simplify_parent_geometry(cluster_mesh_t *mesh, uint32_t parent_cluster_id, uint32_t target_triangle_count);

// LOD selection
void cluster_mesh_select_lod_clusters(cluster_mesh_t *mesh, const simd_float3 *camera_pos, float screen_error_threshold, uint32_t *visible_clusters, uint32_t *visible_count);

#endif // CLUSTER_BUILDER_H
