#ifndef CORE_GPU_ACCELERATION_H
#define CORE_GPU_ACCELERATION_H

#include <stdbool.h>
#include "core/types.h"

// GPU Backend type definition
typedef enum GPUBackendType { 
    GPU_BACKEND_VULKAN = 0,
    GPU_BACKEND_DIRECT3D12,
    GPU_BACKEND_METAL,
    GPU_BACKEND_OPENGL,
    GPU_BACKEND_WEBGPU,
    GPU_BACKEND_CUSTOM
} GPUBackendType;

// Pipeline stages for barriers
typedef enum PipelineStage {
    PIPELINE_STAGE_TOP_OF_PIPE = BIT(0),
    PIPELINE_STAGE_DRAW_INDIRECT = BIT(1),
    PIPELINE_STAGE_VERTEX_INPUT = BIT(2),
    PIPELINE_STAGE_VERTEX_SHADER = BIT(3),
    PIPELINE_STAGE_FRAGMENT_SHADER = BIT(4),
    PIPELINE_STAGE_EARLY_FRAGMENT_TESTS = BIT(5),
    PIPELINE_STAGE_LATE_FRAGMENT_TESTS = BIT(6),
    PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT = BIT(7),
    PIPELINE_STAGE_COMPUTE_SHADER = BIT(8),
    PIPELINE_STAGE_TRANSFER = BIT(9),
    PIPELINE_STAGE_BOTTOM_OF_PIPE = BIT(10),
    PIPELINE_STAGE_HOST = BIT(11),
    PIPELINE_STAGE_ALL_GRAPHICS = BIT(12),
    PIPELINE_STAGE_ALL_COMMANDS = BIT(13),
} PipelineStage;

// Typedef for compatibility with older code that might use GPUBackend as the enum name
// OR checks if existing code uses GPUBackend as a struct.
// From previous error "unknown type name 'GPUBackend'", it seems it's used as a type.
// But wait, the file I viewed had "typedef enum { ... } GPUBackend;".
// So let's define it as an enum to match that pattern if possible, or alias it.

typedef GPUBackendType GPUBackend;

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

// Forward declarations for other types
typedef struct GPUAccelerationSystem GPUAccelerationSystem;
typedef struct GPUConfig GPUConfig;

#endif // CORE_GPU_ACCELERATION_H
