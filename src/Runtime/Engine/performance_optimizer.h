#ifndef PERFORMANCE_OPTIMIZER_H
#define PERFORMANCE_OPTIMIZER_H

/*
 * Performance Optimization System
 * Consolidates all performance optimization techniques across the engine
 * Provides unified API for LOD, culling, batching, and other optimizations
 */

#include "unified_logging.h"
#include "unified_memory.h"
#include "error_handling.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * PERFORMANCE OPTIMIZATION FLAGS
 * ============================================================================ */

typedef enum {
    PERF_OPT_NONE = 0,
    PERF_OPT_LOD = (1 << 0),           // Level of Detail optimization
    PERF_OPT_OCCLUSION = (1 << 1),       // Occlusion culling
    PERF_OPT_FRUSTUM = (1 << 2),        // Frustum culling
    PERF_OPT_BATCHING = (1 << 3),        // Batch rendering
    PERF_OPT_INSTANCING = (1 << 4),      // GPU instancing
    PERF_OPT_CULLING = (1 << 5),         // General culling
    PERF_OPT_STREAMING = (1 << 6),       // Texture streaming
    PERF_OPT_COMPRESSION = (1 << 7),     // Data compression
    PERF_OPT_CACHING = (1 << 8),         // Result caching
    PERF_OPT_THREADED = (1 << 9),        // Multi-threading
    PERF_OPT_SIMD = (1 << 10),          // SIMD optimizations
    PERF_OPT_GPU_COMPUTE = (1 << 11),    // GPU compute
    PERF_OPT_ASYNC = (1 << 12),          // Async operations
    PERF_OPT_COUNT
} performance_optimization_flags_t;

/* ============================================================================
 * LEVEL OF DETAIL SYSTEM
 * ============================================================================ */

typedef struct {
    float* vertices;
    float* normals;
    float* texcoords;
    uint32_t* indices;
    uint32_t vertex_count;
    uint32_t index_count;
    float screen_size_threshold;
    float distance_threshold;
    uint32_t lod_level;
} lod_mesh_t;

typedef struct {
    lod_mesh_t meshes[8];  // Support up to 8 LOD levels
    uint32_t mesh_count;
    uint32_t current_mesh;
    float position[3];
    float bounds_radius;
    float bounds_min[3];
    float bounds_max[3];
    bool visible;
    uint32_t entity_id;
} lod_object_t;

typedef struct {
    lod_object_t* objects;
    uint32_t object_count;
    uint32_t capacity;
    float camera_position[3];
    float camera_direction[3];
    float fov;
    float aspect_ratio;
    float near_plane;
    float far_plane;
    bool auto_lod;
    float lod_bias;
    uint32_t max_lod_levels;
} lod_system_t;

/* ============================================================================
 * OCCLUSION CULLING SYSTEM
 * ============================================================================ */

typedef struct {
    float min[3];
    float max[3];
} aabb_t;

typedef struct {
    aabb_t bounds;
    uint32_t entity_id;
    bool visible;
    bool last_visible;
    uint32_t visibility_frame;
    float distance_to_camera;
} occlusion_object_t;

typedef struct {
    float* depth_buffer;
    uint32_t buffer_width;
    uint32_t buffer_height;
    occlusion_object_t* objects;
    uint32_t object_count;
    uint32_t capacity;
    float* hi_z_buffer;  // Hierarchical Z-buffer
    uint32_t hi_z_levels;
    bool enabled;
    uint32_t cull_frame;
} occlusion_culling_system_t;

/* ============================================================================
 * FRUSTUM CULLING SYSTEM
 * ============================================================================ */

typedef struct {
    float plane[4];  // Plane equation: ax + by + cz + d = 0
} frustum_plane_t;

typedef struct {
    frustum_plane_t planes[6];  // Left, Right, Top, Bottom, Near, Far
    float position[3];
    float direction[3];
    float up[3];
    float right[3];
    float fov;
    float aspect_ratio;
    float near_plane;
    float far_plane;
} frustum_t;

typedef struct {
    frustum_t frustum;
    uint32_t culled_objects;
    uint32_t total_objects;
    bool enabled;
} frustum_culling_system_t;

/* ============================================================================
 * BATCH RENDERING SYSTEM
 * ============================================================================ */

typedef struct {
    uint32_t* indices;
    float* vertices;
    float* normals;
    float* texcoords;
    uint32_t index_count;
    uint32_t vertex_count;
    uint32_t material_id;
    uint32_t texture_id;
    bool transparent;
    uint32_t start_index;
    uint32_t count;
} render_batch_t;

typedef struct {
    render_batch_t* batches;
    uint32_t batch_count;
    uint32_t capacity;
    uint32_t max_vertices_per_batch;
    uint32_t max_indices_per_batch;
    bool enabled;
    uint32_t current_vertex_count;
    uint32_t current_index_count;
    float* vertex_buffer;
    uint32_t* index_buffer;
} batch_rendering_system_t;

/* ============================================================================
 * GPU INSTANCING SYSTEM
 * ============================================================================ */

typedef struct {
    float transform_matrix[16];  // 4x4 transformation matrix
    float color[4];
    float uv_offset[2];
    float scale;
    uint32_t instance_id;
} instance_data_t;

typedef struct {
    uint32_t mesh_id;
    instance_data_t* instances;
    uint32_t instance_count;
    uint32_t capacity;
    uint32_t vertex_buffer;
    uint32_t instance_buffer;
    bool enabled;
} instancing_system_t;

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================ */

typedef struct {
    uint64_t frame_time_ns;
    uint64_t cpu_time_ns;
    uint64_t gpu_time_ns;
    uint32_t draw_calls;
    uint32_t triangles_rendered;
    uint32_t vertices_processed;
    uint32_t objects_culled;
    uint32_t lod_switches;
    uint32_t batch_count;
    uint32_t instance_count;
    float memory_usage_mb;
    float gpu_memory_usage_mb;
    float cpu_utilization;
    float gpu_utilization;
    uint32_t frame_number;
} performance_metrics_t;

typedef struct {
    performance_metrics_t current;
    performance_metrics_t average;
    performance_metrics_t peak;
    performance_metrics_t history[60];  // 60 frame history
    uint32_t history_index;
    bool enabled;
    uint32_t sample_count;
} performance_monitor_t;

/* ============================================================================
 * MAIN PERFORMANCE OPTIMIZER
 * ============================================================================ */

typedef struct {
    // Optimization systems
    lod_system_t* lod_system;
    occlusion_culling_system_t* occlusion_system;
    frustum_culling_system_t* frustum_system;
    batch_rendering_system_t* batch_system;
    instancing_system_t* instancing_system;
    
    // Performance monitoring
    performance_monitor_t* monitor;
    
    // Configuration
    performance_optimization_flags_t enabled_optimizations;
    bool auto_optimize;
    float target_frame_time_ms;
    uint32_t max_frame_time_samples;
    
    // State
    bool initialized;
    uint32_t frame_number;
    float last_frame_time;
    float average_frame_time;
    
    // Thread pool for async operations
    void* thread_pool;
    uint32_t worker_threads;
    
    // Memory management
    void* optimization_allocator;
    size_t memory_usage;
    
} performance_optimizer_t;

/* ============================================================================
 * PERFORMANCE OPTIMIZER API
 * ============================================================================ */

// Lifecycle management
performance_optimizer_t* performance_optimizer_create(const performance_optimization_flags_t flags);
void performance_optimizer_destroy(performance_optimizer_t* optimizer);
void performance_optimizer_update(performance_optimizer_t* optimizer, float dt);

// Configuration
void performance_optimizer_enable_optimization(performance_optimizer_t* optimizer, 
                                            performance_optimization_flags_t flag);
void performance_optimizer_disable_optimization(performance_optimizer_t* optimizer, 
                                             performance_optimization_flags_t flag);
bool performance_optimizer_is_optimization_enabled(performance_optimizer_t* optimizer, 
                                              performance_optimization_flags_t flag);

void performance_optimizer_set_target_framerate(performance_optimizer_t* optimizer, float fps);
void performance_optimizer_set_auto_optimize(performance_optimizer_t* optimizer, bool enabled);

// LOD system
lod_system_t* performance_optimizer_get_lod_system(performance_optimizer_t* optimizer);
void performance_optimizer_add_lod_object(performance_optimizer_t* optimizer, const lod_object_t* object);
void performance_optimizer_remove_lod_object(performance_optimizer_t* optimizer, uint32_t entity_id);
void performance_optimizer_update_camera(performance_optimizer_t* optimizer, 
                                       const float position[3], const float direction[3],
                                       float fov, float aspect_ratio);

// Culling systems
occlusion_culling_system_t* performance_optimizer_get_occlusion_system(performance_optimizer_t* optimizer);
frustum_culling_system_t* performance_optimizer_get_frustum_system(performance_optimizer_t* optimizer);

bool performance_optimizer_is_object_visible(performance_optimizer_t* optimizer, uint32_t entity_id);
void performance_optimizer_cull_objects(performance_optimizer_t* optimizer);

// Batch rendering
batch_rendering_system_t* performance_optimizer_get_batch_system(performance_optimizer_t* optimizer);
void performance_optimizer_begin_batch(performance_optimizer_t* optimizer, uint32_t material_id);
void performance_optimizer_end_batch(performance_optimizer_t* optimizer);
void performance_optimizer_add_to_batch(performance_optimizer_t* optimizer, 
                                       const float* vertices, const float* normals,
                                       const float* texcoords, uint32_t vertex_count,
                                       const uint32_t* indices, uint32_t index_count);

// GPU instancing
instancing_system_t* performance_optimizer_get_instancing_system(performance_optimizer_t* optimizer);
void performance_optimizer_add_instance(performance_optimizer_t* optimizer, uint32_t mesh_id,
                                      const instance_data_t* instance);
void performance_optimizer_render_instances(performance_optimizer_t* optimizer, uint32_t mesh_id);

// Performance monitoring
performance_monitor_t* performance_optimizer_get_monitor(performance_optimizer_t* optimizer);
void performance_optimizer_begin_frame(performance_optimizer_t* optimizer);
void performance_optimizer_end_frame(performance_optimizer_t* optimizer);
void performance_optimizer_get_metrics(performance_optimizer_t* optimizer, performance_metrics_t* metrics);
void performance_optimizer_reset_metrics(performance_optimizer_t* optimizer);

// Auto-optimization
void performance_optimizer_auto_adjust_quality(performance_optimizer_t* optimizer);
void performance_optimizer_optimize_for_target_framerate(performance_optimizer_t* optimizer);

/* ============================================================================
 * LOD SYSTEM API
 * ============================================================================ */

lod_system_t* lod_system_create(uint32_t capacity);
void lod_system_destroy(lod_system_t* lod);
void lod_system_update(lod_system_t* lod, const float camera_pos[3], const float camera_dir[3],
                        float fov, float aspect_ratio);

void lod_system_add_object(lod_system_t* lod, const lod_object_t* object);
void lod_system_remove_object(lod_system_t* lod, uint32_t entity_id);
lod_object_t* lod_system_get_object(lod_system_t* lod, uint32_t entity_id);

void lod_system_set_auto_lod(lod_system_t* lod, bool enabled);
void lod_system_set_lod_bias(lod_system_t* lod, float bias);
void lod_system_set_max_lod_levels(lod_system_t* lod, uint32_t max_levels);

uint32_t lod_system_get_current_mesh(lod_system_t* lod, uint32_t entity_id);
bool lod_system_is_object_visible(lod_system_t* lod, uint32_t entity_id);

/* ============================================================================
 * OCCLUSION CULLING API
 * ============================================================================ */

occlusion_culling_system_t* occlusion_culling_create(uint32_t capacity, uint32_t buffer_width,
                                                   uint32_t buffer_height);
void occlusion_culling_destroy(occlusion_culling_system_t* occ);
void occlusion_culling_update(occlusion_culling_system_t* occ);

void occlusion_culling_add_object(occlusion_culling_system_t* occ, const occlusion_object_t* object);
void occlusion_culling_remove_object(occlusion_culling_system_t* occ, uint32_t entity_id);
bool occlusion_culling_is_visible(occlusion_culling_system_t* occ, uint32_t entity_id);

void occlusion_culling_set_camera(occlusion_culling_system_t* occ, const float view_matrix[16],
                                 const float projection_matrix[16]);
void occlusion_culling_cull(occlusion_culling_system_t* occ);

void occlusion_culling_build_hi_z(occlusion_culling_system_t* occ);
bool occlusion_culling_test_aabb_hi_z(occlusion_culling_system_t* occ, const aabb_t* bounds);

/* ============================================================================
 * FRUSTUM CULLING API
 * ============================================================================ */

frustum_culling_system_t* frustum_culling_create(void);
void frustum_culling_destroy(frustum_culling_system_t* frustum);
void frustum_culling_update(frustum_culling_system_t* frustum, const float view_matrix[16],
                           const float projection_matrix[16]);

void frustum_culling_add_object(frustum_culling_system_t* frustum, const occlusion_object_t* object);
void frustum_culling_remove_object(frustum_culling_system_t* frustum, uint32_t entity_id);
bool frustum_culling_is_visible(frustum_culling_system_t* frustum, uint32_t entity_id);

void frustum_culling_extract_planes(frustum_t* frustum, const float view_matrix[16],
                                  const float projection_matrix[16]);
bool frustum_culling_test_aabb(const frustum_t* frustum, const aabb_t* bounds);
bool frustum_culling_test_sphere(const frustum_t* frustum, const float center[3], float radius);

/* ============================================================================
 * BATCH RENDERING API
 * ============================================================================ */

batch_rendering_system_t* batch_rendering_create(uint32_t max_vertices, uint32_t max_indices);
void batch_rendering_destroy(batch_rendering_system_t* batch);
void batch_rendering_begin_frame(batch_rendering_system_t* batch);
void batch_rendering_end_frame(batch_rendering_system_t* batch);

void batch_rendering_begin_batch(batch_rendering_system_t* batch, uint32_t material_id);
void batch_rendering_end_batch(batch_rendering_system_t* batch);
void batch_rendering_add_geometry(batch_rendering_system_t* batch,
                                 const float* vertices, const float* normals,
                                 const float* texcoords, uint32_t vertex_count,
                                 const uint32_t* indices, uint32_t index_count);

uint32_t batch_rendering_get_batch_count(batch_rendering_system_t* batch);
render_batch_t* batch_rendering_get_batches(batch_rendering_system_t* batch);

/* ============================================================================
 * GPU INSTANCING API
 * ============================================================================ */

instancing_system_t* instancing_create(uint32_t capacity);
void instancing_destroy(instancing_system_t* instancing);
void instancing_update(instancing_system_t* instancing);

void instancing_add_instance(instancing_system_t* instancing, uint32_t mesh_id,
                            const instance_data_t* instance);
void instancing_remove_instance(instancing_system_t* instancing, uint32_t mesh_id, uint32_t instance_id);
void instancing_clear_instances(instancing_system_t* instancing, uint32_t mesh_id);

uint32_t instancing_get_instance_count(instancing_system_t* instancing, uint32_t mesh_id);
instance_data_t* instancing_get_instances(instancing_system_t* instancing, uint32_t mesh_id);

/* ============================================================================
 * PERFORMANCE MONITORING API
 * ============================================================================ */

performance_monitor_t* performance_monitor_create(void);
void performance_monitor_destroy(performance_monitor_t* monitor);
void performance_monitor_begin_frame(performance_monitor_t* monitor);
void performance_monitor_end_frame(performance_monitor_t* monitor);

void performance_monitor_update_metrics(performance_monitor_t* monitor, 
                                     const performance_metrics_t* metrics);
void performance_monitor_get_current_metrics(performance_monitor_t* monitor, 
                                          performance_metrics_t* metrics);
void performance_monitor_get_average_metrics(performance_monitor_t* monitor, 
                                          performance_metrics_t* metrics);
void performance_monitor_get_peak_metrics(performance_monitor_t* monitor, 
                                        performance_metrics_t* metrics);

void performance_monitor_reset(performance_monitor_t* monitor);
void performance_monitor_enable(performance_monitor_t* monitor, bool enabled);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

// AABB utilities
void aabb_from_points(aabb_t* aabb, const float* points, uint32_t point_count);
void aabb_merge(aabb_t* result, const aabb_t* a, const aabb_t* b);
bool aabb_intersects(const aabb_t* a, const aabb_t* b);
bool aabb_contains_point(const aabb_t* a, const float point[3]);
float aabb_get_volume(const aabb_t* a);
float aabb_get_surface_area(const aabb_t* a);

// Distance utilities
float distance_point_to_aabb(const float point[3], const aabb_t* aabb);
float distance_point_to_plane(const float point[3], const float plane[4]);
float distance_aabb_to_plane(const aabb_t* aabb, const float plane[4]);

// Visibility utilities
bool sphere_intersects_frustum(const float center[3], float radius, const frustum_t* frustum);
bool aabb_intersects_frustum(const aabb_t* aabb, const frustum_t* frustum);

// LOD utilities
float calculate_screen_size(const float object_radius, const float distance, 
                          float fov, float viewport_height);
uint32_t select_lod_level(const lod_mesh_t* meshes, uint32_t mesh_count,
                          float screen_size, float lod_bias);

// Performance utilities
uint64_t get_time_ns(void);
float calculate_framerate(uint64_t frame_time_ns);
void smooth_value(float* current, float target, float smoothing_factor);

/* ============================================================================
 * MACROS AND CONVENIENCE FUNCTIONS
 * ============================================================================ */

// Optimization flag macros
#define PERF_OPT_ALL (PERF_OPT_LOD | PERF_OPT_OCCLUSION | PERF_OPT_FRUSTUM | \
                      PERF_OPT_BATCHING | PERF_OPT_INSTANCING | PERF_OPT_CULLING | \
                      PERF_OPT_STREAMING | PERF_OPT_COMPRESSION | PERF_OPT_CACHING | \
                      PERF_OPT_THREADED | PERF_OPT_SIMD | PERF_OPT_GPU_COMPUTE | PERF_OPT_ASYNC)

#define PERF_OPT_DEFAULT (PERF_OPT_LOD | PERF_OPT_FRUSTUM | PERF_OPT_BATCHING)

// Performance monitoring macros
#define PERF_BEGIN_FRAME(optimizer) performance_optimizer_begin_frame(optimizer)
#define PERF_END_FRAME(optimizer) performance_optimizer_end_frame(optimizer)
#define PERF_GET_METRICS(optimizer, metrics) performance_optimizer_get_metrics(optimizer, metrics)

// LOD macros
#define LOD_ADD_OBJECT(optimizer, obj) performance_optimizer_add_lod_object(optimizer, obj)
#define LOD_REMOVE_OBJECT(optimizer, id) performance_optimizer_remove_lod_object(optimizer, id)
#define LOD_UPDATE_CAMERA(optimizer, pos, dir, fov, aspect) \
    performance_optimizer_update_camera(optimizer, pos, dir, fov, aspect)

// Culling macros
#define CULL_OBJECTS(optimizer) performance_optimizer_cull_objects(optimizer)
#define IS_VISIBLE(optimizer, id) performance_optimizer_is_object_visible(optimizer, id)

// Batching macros
#define BATCH_BEGIN(optimizer, material) performance_optimizer_begin_batch(optimizer, material)
#define BATCH_END(optimizer) performance_optimizer_end_batch(optimizer)
#define BATCH_ADD(optimizer, verts, norms, texs, vcount, inds, icount) \
    performance_optimizer_add_to_batch(optimizer, verts, norms, texs, vcount, inds, icount)

// Instancing macros
#define INSTANCE_ADD(optimizer, mesh, data) performance_optimizer_add_instance(optimizer, mesh, data)
#define INSTANCE_RENDER(optimizer, mesh) performance_optimizer_render_instances(optimizer, mesh)

/* ============================================================================
 * DEBUG AND PROFILING SUPPORT
 * ============================================================================ */

#ifdef DEBUG_BUILD
#define PERF_DEBUG_ENABLED 1
#else
#define PERF_DEBUG_ENABLED 0
#endif

#if PERF_DEBUG_ENABLED
#define PERF_DEBUG_LOG(message, ...) LOG_CORE_DEBUG(message, ##__VA_ARGS__)
#define PERF_DEBUG_ASSERT(condition, message) \
    do { if (!(condition)) { LOG_CORE_ERROR(message); } } while(0)
#else
#define PERF_DEBUG_LOG(message, ...) ((void)0)
#define PERF_DEBUG_ASSERT(condition, message) ((void)0)
#endif

// Performance profiling macros
#define PERF_PROFILE_START(name) \
    uint64_t perf_start_##name = get_time_ns()

#define PERF_PROFILE_END(name) \
    do { \
        uint64_t perf_end_##name = get_time_ns(); \
        PERF_DEBUG_LOG("Profile [%s]: %.3f ms", #name, \
                       (perf_end_##name - perf_start_##name) / 1000000.0f); \
    } while(0)

#define PERF_PROFILE_SCOPE(name) \
    struct PerfProfiler { \
        const char* name; \
        uint64_t start_time; \
        PerfProfiler(const char* n) : name(n), start_time(get_time_ns()) {} \
        ~PerfProfiler() { \
            uint64_t end_time = get_time_ns(); \
            PERF_DEBUG_LOG("Profile [%s]: %.3f ms", name, \
                           (end_time - start_time) / 1000000.0f); \
        } \
    } perf_profiler_##name(name)

#ifdef __cplusplus
}
#endif

#endif /* PERFORMANCE_OPTIMIZER_H */
