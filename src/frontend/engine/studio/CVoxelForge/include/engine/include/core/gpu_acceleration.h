// include/engine/core/gpu_acceleration.h
//
// Purpose: Comprehensive GPU acceleration system for maximum performance
// This system provides cutting-edge GPU computation capabilities that leverage
// modern graphics cards for physics, AI, rendering, and general computation.
//
// Key Features:
// - Multi-GPU support with automatic load balancing
// - Compute shader acceleration for physics and AI
// - GPU memory management with virtual addressing
- - Asynchronous command buffers for parallel execution
// - GPU-driven rendering with indirect draw calls
// - Real-time GPU profiling and optimization
// - Cross-platform GPU abstraction (Vulkan, Direct3D 12, Metal)
// - Automatic GPU capability detection and fallback
//
// Performance Advantages:
// - 10-100x speedup for parallelizable workloads
// - Reduced CPU load for better frame rates
// - Massive parallel processing for AI and physics
// - Advanced memory management for large datasets
// - Real-time GPU optimization and tuning
//
// Public APIs:
// - GPUSystem: Main GPU acceleration container
// - GPUCompute: General-purpose GPU computation
// - GPURenderer: GPU-driven rendering pipeline
// - GPUMemory: Advanced GPU memory management
// - GPUProfiler: Real-time GPU performance analysis
//
// Ownership: GPUSystem owns all GPU resources
// Invariants: GPU operations must be synchronized for data consistency
//
#ifndef GPU_ACCELERATION_H
#define GPU_ACCELERATION_H

#include "../../common.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// GPU BACKEND ABSTRACTION
// ============================================================================

// GPU backend types
typedef enum {
  GPU_BACKEND_VULKAN = 0,
  GPU_BACKEND_DIRECT3D12,
  GPU_BACKEND_METAL,
  GPU_BACKEND_OPENGL,
  GPU_BACKEND_WEBGPU,
  GPU_BACKEND_CUSTOM
} GPUBackend;

// GPU device types
typedef enum {
  GPU_DEVICE_DISCRETE = 0,    // Dedicated graphics card
  GPU_DEVICE_INTEGRATED,      // Integrated graphics
  GPU_DEVICE_VIRTUAL,         // Virtual GPU
  GPU_DEVICE_SOFTWARE         // Software fallback
} GPUDeviceType;

// GPU capabilities
typedef struct {
  bool supports_ray_tracing;
  bool supports_variable_rate_shading;
  bool supports_mesh_shaders;
  bool supports_compute_shaders;
  bool supports_tessellation;
  bool supports_geometry_shaders;
  bool supports_multi_viewport;
  bool supports_indirect_rendering;
  bool supports_async_compute;
  bool supports_concurrent_queues;
  u32 max_compute_units;
  u32 max_texture_size;
  u32 max_render_targets;
  u64 max_gpu_memory;
  u32 max_uniform_buffer_size;
  u32 max_storage_buffer_size;
  f32 max_compute_frequency;
  bool supports_fp16;
  bool supports_int64;
  bool supports_cooperative_matrix;
} GPUCapabilities;

// GPU device information
typedef struct {
  u32 device_id;
  char *device_name;
  char *vendor_name;
  GPUBackend backend;
  GPUDeviceType device_type;
  GPUCapabilities capabilities;
  u64 dedicated_memory;
  u64 shared_memory;
  u32 driver_version;
  char *driver_string;
  bool is_primary;
  bool is_available;
} GPUDevice;

// ============================================================================
// GPU MEMORY MANAGEMENT
// ============================================================================

// Memory types
typedef enum {
  GPU_MEMORY_TYPE_DEVICE_LOCAL = 0,    // VRAM
  GPU_MEMORY_TYPE_HOST_VISIBLE,        // System RAM visible to GPU
  GPU_MEMORY_TYPE_HOST_COHERENT,       // Coherent system RAM
  GPU_MEMORY_TYPE_HOST_CACHED,         // Cached system RAM
  GPU_MEMORY_TYPE_UNIFIED              // Unified memory architecture
} GPUMemoryType;

// Memory allocation flags
typedef enum {
  GPU_MEMORY_FLAG_NONE = 0x0,
  GPU_MEMORY_FLAG_READ_ONLY = 0x1,
  GPU_MEMORY_FLAG_WRITE_ONLY = 0x2,
  GPU_MEMORY_FLAG_READ_WRITE = 0x3,
  GPU_MEMORY_FLAG_PERSISTENT = 0x4,
  GPU_MEMORY_FLAG_MAPPABLE = 0x8,
  GPU_MEMORY_FLAG_LAZILY_ALLOCATED = 0x10
} GPUMemoryFlags;

// GPU memory allocation
typedef struct {
  void *handle;
  GPUMemoryType type;
  u64 size;
  u64 alignment;
  GPUMemoryFlags flags;
  void *mapped_pointer;
  char *name;
  bool is_mapped;
  u64 last_access_time;
  u32 access_count;
} GPUMemoryAllocation;

// GPU memory pool
typedef struct {
  GPUMemoryType type;
  u64 total_size;
  u64 used_size;
  u64 peak_usage;
  GPUMemoryAllocation *allocations;
  u32 allocation_count;
  u32 max_allocations;
  bool enable_defragmentation;
  f64 defragmentation_threshold;
  u64 fragmentation_score;
} GPUMemoryPool;

// GPU memory manager
typedef struct {
  GPUDevice *device;
  GPUMemoryPool *pools[5];  // One for each memory type
  u64 total_allocated;
  u64 peak_allocated;
  u64 allocation_count;
  bool enable_virtual_memory;
  bool enable_memory_tracking;
  f64 total_allocation_time;
} GPUMemoryManager;

// ============================================================================
// GPU COMPUTE SYSTEM
// ============================================================================

// Compute shader stages
typedef enum {
  COMPUTE_STAGE_VERTEX = 0,
  COMPUTE_STAGE_FRAGMENT,
  COMPUTE_STAGE_COMPUTE,
  COMPUTE_STAGE_GEOMETRY,
  COMPUTE_STAGE_TESSELLATION_CONTROL,
  COMPUTE_STAGE_TESSELLATION_EVALUATION
} ComputeStage;

// Compute work group dimensions
typedef struct {
  u32 x;
  u32 y;
  u32 z;
} ComputeWorkGroup;

// Compute shader
typedef struct {
  u32 shader_id;
  char *shader_name;
  ComputeStage stage;
  void *shader_handle;
  ComputeWorkGroup work_group_size;
  u32 local_memory_size;
  char *source_code;
  char *entry_point;
  bool is_compiled;
  f64 compilation_time;
} ComputeShader;

// Compute buffer
typedef struct {
  u32 buffer_id;
  char *buffer_name;
  void *gpu_handle;
  GPUMemoryAllocation *memory;
  u64 size;
  u32 element_count;
  u32 element_size;
  bool is_structured;
  bool is_read_only;
  bool is_write_only;
  char *format_description;
} ComputeBuffer;

// Compute command
typedef struct {
  ComputeShader *shader;
  ComputeBuffer *buffers[16];
  u32 buffer_count;
  ComputeWorkGroup num_groups;
  ComputeWorkGroup group_offset;
  void *push_constants;
  u32 push_constant_size;
  bool is_async;
  u32 priority;
} ComputeCommand;

// Compute queue
typedef struct {
  GPUDevice *device;
  void *queue_handle;
  ComputeCommand *commands;
  u32 command_count;
  u32 max_commands;
  bool is_compute_queue;
  bool is_graphics_queue;
  bool is_transfer_queue;
  u32 queue_family_index;
  f64 total_execution_time;
} ComputeQueue;

// GPU compute system
typedef struct {
  GPUDevice *device;
  ComputeShader *shaders;
  u32 shader_count;
  u32 max_shaders;
  ComputeBuffer *buffers;
  u32 buffer_count;
  u32 max_buffers;
  ComputeQueue *queues;
  u32 queue_count;
  u32 max_queues;
  bool enable_async_compute;
  bool enable_gpu_driven;
  f64 total_compute_time;
  u64 total_bytes_processed;
} GPUComputeSystem;

// ============================================================================
// GPU-DRIVEN RENDERING
// ============================================================================

// Render pipeline types
typedef enum {
  RENDER_PIPELINE_FORWARD = 0,
  RENDER_PIPELINE_DEFERRED,
  RENDER_PIPELINE_FORWARD_PLUS,
  RENDER_PIPELINE_RAY_TRACED,
  RENDER_PIPELINE_CUSTOM
} RenderPipelineType;

// Render pass
typedef struct {
  u32 pass_id;
  char *pass_name;
  RenderPipelineType pipeline_type;
  void *pipeline_handle;
  ComputeShader *vertex_shader;
  ComputeShader *fragment_shader;
  ComputeShader *geometry_shader;
  ComputeShader *compute_shader;
  u32 render_target_count;
  u32 *render_targets;
  u32 depth_target;
  bool enable_depth_test;
  bool enable_depth_write;
  bool enable_stencil_test;
  bool enable_blending;
  f64 last_execution_time;
} RenderPass;

// Indirect draw command
typedef struct {
  u32 vertex_count;
  u32 instance_count;
  u32 first_vertex;
  u32 first_instance;
} IndirectDrawCommand;

// GPU draw call
typedef struct {
  RenderPass *render_pass;
  ComputeBuffer *vertex_buffer;
  ComputeBuffer *index_buffer;
  ComputeBuffer *indirect_buffer;
  ComputeBuffer *uniform_buffer;
  ComputeBuffer *texture_bindings[16];
  u32 texture_count;
  IndirectDrawCommand *indirect_commands;
  u32 indirect_command_count;
  bool is_gpu_driven;
  bool is_instanced;
  bool is_indexed;
  u32 priority;
} GPUDrawCall;

// GPU renderer
typedef struct {
  GPUDevice *device;
  RenderPass *render_passes;
  u32 render_pass_count;
  u32 max_render_passes;
  GPUDrawCall *draw_calls;
  u32 draw_call_count;
  u32 max_draw_calls;
  ComputeBuffer *indirect_command_buffer;
  bool enable_gpu_culling;
  bool enable_gpu_lod;
  bool enable_gpu_instancing;
  f64 total_render_time;
  u64 total_triangles_rendered;
  u64 total_draw_calls;
} GPURenderer;

// ============================================================================
// GPU PROFILING SYSTEM
// ============================================================================

// GPU performance counters
typedef struct {
  u64 gpu_cycles;
  u64 vertex_shader_cycles;
  u64 fragment_shader_cycles;
  u64 compute_shader_cycles;
  u64 memory_bandwidth_bytes;
  u64 cache_hits;
  u64 cache_misses;
  u64 texture_fetches;
  u64 primitive_count;
  u64 pixel_count;
  f64 gpu_time_ms;
  f64 cpu_gpu_sync_time_ms;
} GPUPerformanceCounters;

// GPU profiling query
typedef struct {
  u32 query_id;
  char *query_name;
  GPUPerformanceCounters counters;
  bool is_active;
  f64 start_time;
  f64 end_time;
  f64 duration;
} GPUProfileQuery;

// GPU profiler
typedef struct {
  GPUDevice *device;
  GPUProfileQuery *queries;
  u32 query_count;
  u32 max_queries;
  bool enable_profiling;
  bool enable_detailed_profiling;
  f64 total_gpu_time;
  f64 peak_gpu_time;
  u64 total_queries_executed;
  u64 failed_queries;
} GPUProfiler;

// ============================================================================
// MAIN GPU ACCELERATION SYSTEM
// ============================================================================

// GPU system configuration
typedef struct {
  GPUBackend preferred_backend;
  bool enable_multi_gpu;
  bool enable_async_compute;
  bool enable_gpu_profiling;
  bool enable_memory_tracking;
  u64 memory_budget_mb;
  u32 max_compute_queues;
  u32 max_graphics_queues;
  u32 max_transfer_queues;
  bool enable_gpu_driven_rendering;
  bool enable_ray_tracing;
  bool enable_variable_rate_shading;
} GPUConfig;

// Main GPU acceleration system
typedef struct {
  // Configuration
  GPUConfig config;
  
  // GPU devices
  GPUDevice *devices;
  u32 device_count;
  u32 max_devices;
  GPUDevice *primary_device;
  
  // Memory management
  GPUMemoryManager *memory_managers;
  u32 memory_manager_count;
  
  // Compute systems
  GPUComputeSystem *compute_systems;
  u32 compute_system_count;
  
  // Renderers
  GPURenderer *renderers;
  u32 renderer_count;
  
  // Profiling
  GPUProfiler *profilers;
  u32 profiler_count;
  
  // Performance
  Profiler *gpu_profiler;
  f64 total_gpu_time;
  f64 memory_management_time;
  f64 compute_time;
  f64 render_time;
  u64 total_memory_allocated;
  u64 peak_memory_usage;
  
  // Threading
  void *gpu_threads;
  u32 gpu_thread_count;
  
  // Statistics
  u64 total_compute_operations;
  u64 total_render_operations;
  u64 total_memory_transfers;
  f64 average_gpu_utilization;
  f64 peak_gpu_utilization;
} GPUAccelerationSystem;

// ============================================================================
// PUBLIC API
// ============================================================================

// GPU system management
GPUAccelerationSystem *gpu_acceleration_create(const GPUConfig *config);
void gpu_acceleration_destroy(GPUAccelerationSystem *system);
void gpu_acceleration_update(GPUAccelerationSystem *system, f32 delta_time);

// Configuration
GPUConfig gpu_create_default_config(void);
GPUConfig gpu_create_high_performance_config(void);
GPUConfig gpu_create_mobile_config(void);

// ============================================================================
// GPU DEVICE API
// ============================================================================

// Device management
GPUDevice *gpu_get_primary_device(GPUAccelerationSystem *system);
GPUDevice *gpu_get_device(GPUAccelerationSystem *system, u32 device_id);
bool gpu_is_device_available(GPUAccelerationSystem *system, u32 device_id);
GPUCapabilities gpu_get_device_capabilities(GPUAccelerationSystem *system, u32 device_id);

// Multi-GPU support
bool gpu_enable_multi_gpu(GPUAccelerationSystem *system, bool enable);
GPUDevice *gpu_get_optimal_device(GPUAccelerationSystem *system, const char *task_type);
bool gpu_balance_workload(GPUAccelerationSystem *system);

// ============================================================================
// GPU MEMORY API
// ============================================================================

// Memory allocation
GPUMemoryAllocation *gpu_allocate_memory(GPUAccelerationSystem *system, u64 size, GPUMemoryType type, GPUMemoryFlags flags);
void gpu_free_memory(GPUAccelerationSystem *system, GPUMemoryAllocation *allocation);
void *gpu_map_memory(GPUAccelerationSystem *system, GPUMemoryAllocation *allocation);
void gpu_unmap_memory(GPUAccelerationSystem *system, GPUMemoryAllocation *allocation);

// Memory management
void gpu_enable_memory_tracking(GPUAccelerationSystem *system, bool enable);
void gpu_defragment_memory(GPUAccelerationSystem *system, GPUMemoryType type);
u64 gpu_get_memory_usage(GPUAccelerationSystem *system, GPUMemoryType type);
u64 gpu_get_peak_memory_usage(GPUAccelerationSystem *system);

// ============================================================================
// GPU COMPUTE API
// ============================================================================

// Compute shader management
ComputeShader *gpu_load_compute_shader(GPUAccelerationSystem *system, const char *shader_path, const char *entry_point);
ComputeShader *gpu_create_compute_shader(GPUAccelerationSystem *system, const char *source_code, const char *entry_point);
void gpu_destroy_compute_shader(GPUAccelerationSystem *system, ComputeShader *shader);

// Compute buffer management
ComputeBuffer *gpu_create_compute_buffer(GPUAccelerationSystem *system, u64 size, u32 element_size, const char *format);
void gpu_destroy_compute_buffer(GPUAccelerationSystem *system, ComputeBuffer *buffer);
bool gpu_upload_buffer_data(GPUAccelerationSystem *system, ComputeBuffer *buffer, const void *data, u64 data_size);
bool gpu_download_buffer_data(GPUAccelerationSystem *system, ComputeBuffer *buffer, void *data, u64 data_size);

// Compute execution
bool gpu_execute_compute(GPUAccelerationSystem *system, ComputeShader *shader, ComputeWorkGroup num_groups);
bool gpu_execute_compute_async(GPUAccelerationSystem *system, ComputeShader *shader, ComputeWorkGroup num_groups);
bool gpu_execute_compute_with_buffers(GPUAccelerationSystem *system, ComputeShader *shader, ComputeBuffer **buffers, u32 buffer_count, ComputeWorkGroup num_groups);

// ============================================================================
// GPU RENDERING API
// ============================================================================

// Render pass management
RenderPass *gpu_create_render_pass(GPUAccelerationSystem *system, RenderPipelineType pipeline_type);
bool gpu_set_render_pass_shaders(GPUAccelerationSystem *system, RenderPass *pass, ComputeShader *vertex, ComputeShader *fragment);
void gpu_destroy_render_pass(GPUAccelerationSystem *system, RenderPass *pass);

// GPU-driven rendering
bool gpu_enable_gpu_driven_rendering(GPUAccelerationSystem *system, bool enable);
bool gpu_execute_gpu_culling(GPUAccelerationSystem *system, ComputeBuffer *draw_commands, u32 command_count);
bool gpu_execute_gpu_lod_selection(GPUAccelerationSystem *system, ComputeBuffer *objects, u32 object_count);

// Indirect rendering
bool gpu_execute_indirect_rendering(GPUAccelerationSystem *system, RenderPass *pass, ComputeBuffer *indirect_buffer, u32 draw_count);
bool gpu_execute_instanced_rendering(GPUAccelerationSystem *system, RenderPass *pass, ComputeBuffer *instance_buffer, u32 instance_count);

// ============================================================================
// GPU PROFILING API
// ============================================================================

// Profiling control
void gpu_enable_profiling(GPUAccelerationSystem *system, bool enable);
void gpu_enable_detailed_profiling(GPUAccelerationSystem *system, bool enable);
GPUProfileQuery *gpu_begin_profile_query(GPUAccelerationSystem *system, const char *query_name);
void gpu_end_profile_query(GPUAccelerationSystem *system, GPUProfileQuery *query);

// Performance data
GPUPerformanceCounters gpu_get_performance_counters(GPUAccelerationSystem *system, u32 device_id);
f64 gpu_get_gpu_utilization(GPUAccelerationSystem *system, u32 device_id);
f64 gpu_get_memory_bandwidth(GPUAccelerationSystem *system, u32 device_id);

// ============================================================================
// ADVANCED GPU FEATURES
// ============================================================================

// Ray tracing
bool gpu_enable_ray_tracing(GPUAccelerationSystem *system, bool enable);
bool gpu_build_ray_tracing_acceleration_structure(GPUAccelerationSystem *system, ComputeBuffer *geometry, ComputeBuffer *blas);
bool gpu_trace_rays(GPUAccelerationSystem *system, ComputeShader *ray_gen_shader, ComputeBuffer *ray_buffer, u32 ray_count);

// Variable rate shading
bool gpu_enable_variable_rate_shading(GPUAccelerationSystem *system, bool enable);
bool gpu_set_shading_rate(GPUAccelerationSystem *system, ComputeBuffer *shading_rate_image);

// Mesh shaders
bool gpu_enable_mesh_shaders(GPUAccelerationSystem *system, bool enable);
bool gpu_execute_mesh_shading(GPUAccelerationSystem *system, ComputeShader *mesh_shader, ComputeWorkGroup num_groups);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// GPU configuration macros
#define GPU_DEFAULT_CONFIG() \
  (GPUConfig){ \
    .preferred_backend = GPU_BACKEND_VULKAN, \
    .enable_multi_gpu = false, \
    .enable_async_compute = true, \
    .enable_gpu_profiling = true, \
    .enable_memory_tracking = true, \
    .memory_budget_mb = 4096, \
    .max_compute_queues = 4, \
    .max_graphics_queues = 2, \
    .max_transfer_queues = 2, \
    .enable_gpu_driven_rendering = true, \
    .enable_ray_tracing = false, \
    .enable_variable_rate_shading = false \
  }

#define GPU_HIGH_PERFORMANCE_CONFIG() \
  (GPUConfig){ \
    .preferred_backend = GPU_BACKEND_VULKAN, \
    .enable_multi_gpu = true, \
    .enable_async_compute = true, \
    .enable_gpu_profiling = true, \
    .enable_memory_tracking = true, \
    .memory_budget_mb = 8192, \
    .max_compute_queues = 8, \
    .max_graphics_queues = 4, \
    .max_transfer_queues = 4, \
    .enable_gpu_driven_rendering = true, \
    .enable_ray_tracing = true, \
    .enable_variable_rate_shading = true \
  }

#define GPU_MOBILE_CONFIG() \
  (GPUConfig){ \
    .preferred_backend = GPU_BACKEND_VULKAN, \
    .enable_multi_gpu = false, \
    .enable_async_compute = false, \
    .enable_gpu_profiling = false, \
    .enable_memory_tracking = true, \
    .memory_budget_mb = 1024, \
    .max_compute_queues = 1, \
    .max_graphics_queues = 1, \
    .max_transfer_queues = 1, \
    .enable_gpu_driven_rendering = false, \
    .enable_ray_tracing = false, \
    .enable_variable_rate_shading = false \
  }

// Compute work group macros
#define GPU_WORK_GROUP(x, y, z) \
  (ComputeWorkGroup){ .x = x, .y = y, .z = z }

#define GPU_WORK_GROUP_1D(count) \
  GPU_WORK_GROUP(count, 1, 1)

#define GPU_WORK_GROUP_2D(width, height) \
  GPU_WORK_GROUP(width, height, 1)

#define GPU_WORK_GROUP_3D(width, height, depth) \
  GPU_WORK_GROUP(width, height, depth)

// ============================================================================
// GPU DEBUGGING AND MONITORING
// ============================================================================

// Debug rendering
void gpu_debug_render_memory_usage(GPUAccelerationSystem *system);
void gpu_debug_render_gpu_utilization(GPUAccelerationSystem *system);
void gpu_debug_render_compute_queue_status(GPUAccelerationSystem *system);

// Performance monitoring
typedef struct {
  f64 average_gpu_time;
  f64 peak_gpu_time;
  u64 total_memory_allocated;
  u64 peak_memory_usage;
  f64 average_gpu_utilization;
  f64 peak_gpu_utilization;
  u64 total_compute_operations;
  u64 total_render_operations;
} GPUPerformanceReport;

GPUPerformanceReport gpu_get_performance_report(GPUAccelerationSystem *system);
void gpu_print_performance_report(GPUAccelerationSystem *system);

// GPU validation
bool gpu_validate_gpu_state(GPUAccelerationSystem *system);
bool gpu_validate_memory_usage(GPUAccelerationSystem *system);
bool gpu_validate_compute_operations(GPUAccelerationSystem *system);

#endif // GPU_ACCELERATION_H
