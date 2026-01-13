/*
 * mtl_pipeline.h
 * Metal Pipeline State Objects
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_PIPELINE_H
#define PLATFORM_MTL_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS (Objective-C types)
 * ============================================================================ */

typedef void* MTLDeviceRef;
typedef void* MTLRenderPipelineStateRef;
typedef void* MTLComputePipelineStateRef;
typedef void* MTLDepthStencilStateRef;
typedef void* MTLFunctionRef;
typedef void* MTLLibraryRef;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_pixel_format {
    METAL_PIXEL_FORMAT_INVALID = 0,
    METAL_PIXEL_FORMAT_BGRA8_UNORM = 80,
    METAL_PIXEL_FORMAT_RGBA8_UNORM = 70,
    METAL_PIXEL_FORMAT_RGBA16_FLOAT = 115,
    METAL_PIXEL_FORMAT_DEPTH32_FLOAT = 252,
    METAL_PIXEL_FORMAT_DEPTH24_UNORM_STENCIL8 = 255,
} metal_pixel_format_t;

typedef enum metal_vertex_format {
    METAL_VERTEX_FORMAT_INVALID = 0,
    METAL_VERTEX_FORMAT_FLOAT = 28,
    METAL_VERTEX_FORMAT_FLOAT2 = 29,
    METAL_VERTEX_FORMAT_FLOAT3 = 30,
    METAL_VERTEX_FORMAT_FLOAT4 = 31,
    METAL_VERTEX_FORMAT_UCHAR4_NORMALIZED = 42,
} metal_vertex_format_t;

typedef enum metal_cull_mode {
    METAL_CULL_MODE_NONE = 0,
    METAL_CULL_MODE_FRONT = 1,
    METAL_CULL_MODE_BACK = 2,
} metal_cull_mode_t;

typedef enum metal_triangle_fill_mode {
    METAL_FILL_MODE_FILL = 0,
    METAL_FILL_MODE_LINES = 1,
} metal_triangle_fill_mode_t;

typedef enum metal_winding_order {
    METAL_WINDING_CLOCKWISE = 0,
    METAL_WINDING_COUNTER_CLOCKWISE = 1,
} metal_winding_order_t;

typedef enum metal_compare_function {
    METAL_COMPARE_NEVER = 0,
    METAL_COMPARE_LESS = 1,
    METAL_COMPARE_EQUAL = 2,
    METAL_COMPARE_LESS_EQUAL = 3,
    METAL_COMPARE_GREATER = 4,
    METAL_COMPARE_NOT_EQUAL = 5,
    METAL_COMPARE_GREATER_EQUAL = 6,
    METAL_COMPARE_ALWAYS = 7,
} metal_compare_function_t;

typedef enum metal_blend_factor {
    METAL_BLEND_ZERO = 0,
    METAL_BLEND_ONE = 1,
    METAL_BLEND_SRC_COLOR = 2,
    METAL_BLEND_ONE_MINUS_SRC_COLOR = 3,
    METAL_BLEND_SRC_ALPHA = 4,
    METAL_BLEND_ONE_MINUS_SRC_ALPHA = 5,
    METAL_BLEND_DST_COLOR = 6,
    METAL_BLEND_ONE_MINUS_DST_COLOR = 7,
    METAL_BLEND_DST_ALPHA = 8,
    METAL_BLEND_ONE_MINUS_DST_ALPHA = 9,
} metal_blend_factor_t;

typedef enum metal_blend_operation {
    METAL_BLEND_OP_ADD = 0,
    METAL_BLEND_OP_SUBTRACT = 1,
    METAL_BLEND_OP_REVERSE_SUBTRACT = 2,
    METAL_BLEND_OP_MIN = 3,
    METAL_BLEND_OP_MAX = 4,
} metal_blend_operation_t;

typedef enum metal_stencil_operation {
    METAL_STENCIL_OP_KEEP = 0,
    METAL_STENCIL_OP_ZERO = 1,
    METAL_STENCIL_OP_REPLACE = 2,
    METAL_STENCIL_OP_INCREMENT_CLAMP = 3,
    METAL_STENCIL_OP_DECREMENT_CLAMP = 4,
    METAL_STENCIL_OP_INVERT = 5,
    METAL_STENCIL_OP_INCREMENT_WRAP = 6,
    METAL_STENCIL_OP_DECREMENT_WRAP = 7,
} metal_stencil_operation_t;

/* ============================================================================
 * VERTEX DESCRIPTOR
 * ============================================================================ */

#define METAL_MAX_VERTEX_ATTRIBUTES 16
#define METAL_MAX_VERTEX_BUFFERS 16

typedef struct metal_vertex_attribute {
    metal_vertex_format_t format;
    uint32_t offset;
    uint32_t buffer_index;
} metal_vertex_attribute_t;

typedef struct metal_vertex_buffer_layout {
    uint32_t stride;
    uint32_t step_rate;  // 0 = per-vertex, 1+ = per-instance
} metal_vertex_buffer_layout_t;

typedef struct metal_vertex_descriptor {
    metal_vertex_attribute_t attributes[METAL_MAX_VERTEX_ATTRIBUTES];
    metal_vertex_buffer_layout_t layouts[METAL_MAX_VERTEX_BUFFERS];
    uint32_t attribute_count;
    uint32_t layout_count;
} metal_vertex_descriptor_t;

/* ============================================================================
 * SHADER LIBRARY
 * ============================================================================ */

typedef struct metal_shader_library {
    MTLLibraryRef library;
    char name[256];
} metal_shader_library_t;

/* ============================================================================
 * DEPTH/STENCIL STATE
 * ============================================================================ */

typedef struct metal_stencil_desc {
    metal_compare_function_t stencil_compare;
    metal_stencil_operation_t stencil_fail_op;
    metal_stencil_operation_t depth_fail_op;
    metal_stencil_operation_t depth_stencil_pass_op;
    uint32_t read_mask;
    uint32_t write_mask;
} metal_stencil_desc_t;

typedef struct metal_depth_stencil_desc {
    metal_compare_function_t depth_compare;
    bool depth_write_enabled;
    bool stencil_enabled;
    metal_stencil_desc_t stencil_front;
    metal_stencil_desc_t stencil_back;
    uint32_t stencil_reference_value;
} metal_depth_stencil_desc_t;

typedef struct metal_depth_stencil_state {
    MTLDepthStencilStateRef state;
    metal_depth_stencil_desc_t desc;
} metal_depth_stencil_state_t;

/* ============================================================================
 * BLEND STATE
 * ============================================================================ */

typedef struct metal_blend_desc {
    bool blend_enabled;
    metal_blend_factor_t src_rgb_blend;
    metal_blend_factor_t dst_rgb_blend;
    metal_blend_operation_t rgb_blend_op;
    metal_blend_factor_t src_alpha_blend;
    metal_blend_factor_t dst_alpha_blend;
    metal_blend_operation_t alpha_blend_op;
} metal_blend_desc_t;

/* ============================================================================
 * RENDER PIPELINE
 * ============================================================================ */

typedef struct metal_render_pipeline_desc {
    MTLFunctionRef vertex_function;
    MTLFunctionRef fragment_function;
    metal_pixel_format_t color_format;
    metal_pixel_format_t depth_format;
    metal_pixel_format_t stencil_format;
    metal_vertex_descriptor_t vertex_descriptor;
    metal_blend_desc_t blend_state;
    metal_cull_mode_t cull_mode;
    metal_triangle_fill_mode_t fill_mode;
    metal_winding_order_t winding_order;
    uint32_t sample_count;  // MSAA sample count (1, 2, 4, 8)
    bool depth_clip_enabled;
    bool conservative_rasterization_enabled;
} metal_render_pipeline_desc_t;

typedef struct metal_render_pipeline {
    MTLRenderPipelineStateRef state;
    MTLDepthStencilStateRef depth_stencil;
    metal_cull_mode_t cull_mode;
    metal_triangle_fill_mode_t fill_mode;
    uint64_t hash;
} metal_render_pipeline_t;

/* ============================================================================
 * COMPUTE PIPELINE
 * ============================================================================ */

typedef struct metal_compute_pipeline_desc {
    MTLFunctionRef compute_function;
    uint32_t threadgroup_size_x;
    uint32_t threadgroup_size_y;
    uint32_t threadgroup_size_z;
    uint32_t threadgroup_memory_length;  // Bytes of threadgroup memory
} metal_compute_pipeline_desc_t;

typedef struct metal_compute_pipeline {
    MTLComputePipelineStateRef state;
    uint32_t threadgroup_size_x;
    uint32_t threadgroup_size_y;
    uint32_t threadgroup_size_z;
    uint64_t hash;
} metal_compute_pipeline_t;

/* ============================================================================
 * PIPELINE CACHE
 * ============================================================================ */

#define METAL_PIPELINE_CACHE_SIZE 256

typedef struct metal_pipeline_cache_entry {
    uint64_t hash;
    void* pipeline;  // metal_render_pipeline_t* or metal_compute_pipeline_t*
    bool is_compute;
    bool in_use;
    uint64_t last_used_frame;  // For LRU eviction
    size_t memory_size;  // Estimated memory usage
} metal_pipeline_cache_entry_t;

typedef struct metal_pipeline_cache_stats {
    uint32_t total_pipelines;
    uint32_t cache_hits;
    uint32_t cache_misses;
    size_t total_memory_bytes;
    uint32_t evictions;
} metal_pipeline_cache_stats_t;

typedef struct metal_pipeline_cache {
    metal_pipeline_cache_entry_t entries[METAL_PIPELINE_CACHE_SIZE];
    uint32_t count;
    uint64_t current_frame;
    size_t max_memory_bytes;  // Memory limit (0 = no limit)
    metal_pipeline_cache_stats_t stats;
} metal_pipeline_cache_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int metal_pipeline_init(void);
void metal_pipeline_shutdown(void);

/* ============================================================================
 * API - SHADER LIBRARY
 * ============================================================================ */

metal_shader_library_t* metal_load_shader_library(
    MTLDeviceRef device,
    const char* path
);

metal_shader_library_t* metal_load_shader_library_data(
    MTLDeviceRef device,
    const void* data,
    size_t size
);

MTLFunctionRef metal_get_function(
    metal_shader_library_t* library,
    const char* name
);

void metal_destroy_shader_library(metal_shader_library_t* library);

/* ============================================================================
 * API - VERTEX DESCRIPTOR
 * ============================================================================ */

void metal_vertex_descriptor_init(metal_vertex_descriptor_t* desc);

void metal_vertex_descriptor_add_attribute(
    metal_vertex_descriptor_t* desc,
    uint32_t location,
    metal_vertex_format_t format,
    uint32_t offset,
    uint32_t buffer_index
);

void metal_vertex_descriptor_set_layout(
    metal_vertex_descriptor_t* desc,
    uint32_t buffer_index,
    uint32_t stride,
    uint32_t step_rate
);

/* ============================================================================
 * API - DEPTH/STENCIL STATE
 * ============================================================================ */

metal_depth_stencil_state_t* metal_create_depth_stencil_state(
    MTLDeviceRef device,
    const metal_depth_stencil_desc_t* desc
);

void metal_destroy_depth_stencil_state(metal_depth_stencil_state_t* state);

/* ============================================================================
 * API - RENDER PIPELINE
 * ============================================================================ */

metal_render_pipeline_t* metal_create_render_pipeline(
    MTLDeviceRef device,
    const metal_render_pipeline_desc_t* desc
);

void metal_destroy_render_pipeline(metal_render_pipeline_t* pipeline);

/* ============================================================================
 * API - COMPUTE PIPELINE
 * ============================================================================ */

metal_compute_pipeline_t* metal_create_compute_pipeline(
    MTLDeviceRef device,
    const metal_compute_pipeline_desc_t* desc
);

void metal_destroy_compute_pipeline(metal_compute_pipeline_t* pipeline);

/* ============================================================================
 * API - PIPELINE CACHE
 * ============================================================================ */

void metal_pipeline_cache_init(metal_pipeline_cache_t* cache);
void metal_pipeline_cache_shutdown(metal_pipeline_cache_t* cache);

metal_render_pipeline_t* metal_pipeline_cache_get_render(
    metal_pipeline_cache_t* cache,
    MTLDeviceRef device,
    const metal_render_pipeline_desc_t* desc
);

metal_compute_pipeline_t* metal_pipeline_cache_get_compute(
    metal_pipeline_cache_t* cache,
    MTLDeviceRef device,
    const metal_compute_pipeline_desc_t* desc
);

void metal_pipeline_cache_advance_frame(metal_pipeline_cache_t* cache);
void metal_pipeline_cache_set_memory_limit(metal_pipeline_cache_t* cache, size_t max_bytes);
metal_pipeline_cache_stats_t metal_pipeline_cache_get_stats(const metal_pipeline_cache_t* cache);

bool metal_pipeline_cache_save_to_disk(const metal_pipeline_cache_t* cache, const char* path);
bool metal_pipeline_cache_load_from_disk(metal_pipeline_cache_t* cache, MTLDeviceRef device, const char* path);

/* ============================================================================
 * UTILITIES
 * ============================================================================ */

uint64_t metal_hash_render_pipeline_desc(const metal_render_pipeline_desc_t* desc);
uint64_t metal_hash_compute_pipeline_desc(const metal_compute_pipeline_desc_t* desc);

/* ============================================================================
 * EXTENSIONS (Statistics & Validation)
 * ============================================================================ */

typedef struct metal_render_pipeline_stats {
    bool has_vertex_shader;
    bool has_fragment_shader;
    bool uses_depth_testing;
    bool uses_blending;
    size_t estimated_memory_bytes;
    uint64_t creation_time_ns;
} metal_render_pipeline_stats_t;

typedef struct metal_compute_pipeline_stats {
    uint32_t max_total_threads_per_threadgroup;
    size_t estimated_memory_bytes;
    uint64_t creation_time_ns;
} metal_compute_pipeline_stats_t;

metal_render_pipeline_stats_t metal_render_pipeline_get_stats(const metal_render_pipeline_t *pipeline);
metal_compute_pipeline_stats_t metal_compute_pipeline_get_stats(const metal_compute_pipeline_t *pipeline);

bool metal_compute_pipeline_validate(MTLDeviceRef device_ref,
                                     const metal_compute_pipeline_desc_t *desc,
                                     char *error_message,
                                     size_t error_message_size);

uint32_t metal_get_max_threads_per_threadgroup(MTLDeviceRef device_ref);

bool metal_pipeline_cache_invalidate_by_hash(metal_pipeline_cache_t *cache, uint64_t hash);
void metal_pipeline_cache_invalidate_all(metal_pipeline_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_PIPELINE_H */
