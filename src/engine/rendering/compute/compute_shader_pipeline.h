#ifndef COMPUTE_SHADER_PIPELINE_H
#define COMPUTE_SHADER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute shader pipeline backend types
typedef enum {
    COMPUTE_BACKEND_VULKAN,
    COMPUTE_BACKEND_METAL,
    COMPUTE_BACKEND_D3D12,
    COMPUTE_BACKEND_OPENGL
} compute_backend_t;

// Compute shader resource types
typedef enum {
    COMPUTE_RESOURCE_BUFFER,
    COMPUTE_RESOURCE_TEXTURE,
    COMPUTE_RESOURCE_SAMPLER,
    COMPUTE_RESOURCE_UNIFORM_BUFFER
} compute_resource_type_t;

// Compute shader resource flags
typedef enum {
    COMPUTE_RESOURCE_READ = 0x1,
    COMPUTE_RESOURCE_WRITE = 0x2,
    COMPUTE_RESOURCE_READ_WRITE = COMPUTE_RESOURCE_READ | COMPUTE_RESOURCE_WRITE,
    COMPUTE_RESOURCE_DYNAMIC = 0x4,
    COMPUTE_RESOURCE_PERSISTENT = 0x8
} compute_resource_flags_t;

// Forward declarations
typedef struct compute_pipeline_t compute_pipeline_t;
typedef struct compute_shader_t compute_shader_t;
typedef struct compute_resource_t compute_resource_t;
typedef struct compute_command_buffer_t compute_command_buffer_t;

// Compute resource descriptor
typedef struct {
    compute_resource_type_t type;
    compute_resource_flags_t flags;
    uint32_t size;
    uint32_t width, height, depth;
    uint32_t format;
    void* initial_data;
    const char* name;
} compute_resource_desc_t;

// Compute shader descriptor
typedef struct {
    const char* source_code;
    const char* entry_point;
    compute_backend_t backend;
    uint32_t thread_group_size_x;
    uint32_t thread_group_size_y;
    uint32_t thread_group_size_z;
    const char* name;
} compute_shader_desc_t;

// Compute dispatch parameters
typedef struct {
    uint32_t group_count_x;
    uint32_t group_count_y;
    uint32_t group_count_z;
} compute_dispatch_params_t;

// Compute pipeline statistics
typedef struct {
    uint32_t total_shaders_created;
    uint32_t total_resources_created;
    uint32_t total_dispatches;
    uint64_t total_gpu_time_ns;
    uint64_t total_memory_allocated;
    uint32_t active_shaders;
    uint32_t active_resources;
    uint32_t pipeline_creations;
    uint32_t pipeline_destructions;
} compute_pipeline_stats_t;

// Error codes
typedef enum {
    COMPUTE_SUCCESS = 0,
    COMPUTE_ERROR_INVALID_PARAM = -1,
    COMPUTE_ERROR_OUT_OF_MEMORY = -2,
    COMPUTE_ERROR_BACKEND_NOT_SUPPORTED = -3,
    COMPUTE_ERROR_SHADER_COMPILATION_FAILED = -4,
    COMPUTE_ERROR_RESOURCE_CREATION_FAILED = -5,
    COMPUTE_ERROR_DISPATCH_FAILED = -6,
    COMPUTE_ERROR_NOT_INITIALIZED = -7,
    COMPUTE_ERROR_BACKEND_INITIALIZATION_FAILED = -8
} compute_error_t;

// Pipeline initialization
compute_error_t compute_pipeline_init(compute_pipeline_t** pipeline, compute_backend_t backend);
void compute_pipeline_shutdown(compute_pipeline_t* pipeline);

// Shader management
compute_error_t compute_shader_create(compute_pipeline_t* pipeline, const compute_shader_desc_t* desc, compute_shader_t** shader);
void compute_shader_destroy(compute_pipeline_t* pipeline, compute_shader_t* shader);
compute_error_t compute_shader_bind(compute_pipeline_t* pipeline, compute_shader_t* shader);

// Resource management
compute_error_t compute_resource_create(compute_pipeline_t* pipeline, const compute_resource_desc_t* desc, compute_resource_t** resource);
void compute_resource_destroy(compute_pipeline_t* pipeline, compute_resource_t* resource);
compute_error_t compute_resource_bind(compute_pipeline_t* pipeline, compute_resource_t* resource, uint32_t binding);
compute_error_t compute_resource_update_data(compute_pipeline_t* pipeline, compute_resource_t* resource, const void* data, uint32_t size, uint32_t offset);

// Command buffer management
compute_error_t compute_command_buffer_create(compute_pipeline_t* pipeline, compute_command_buffer_t** cmd_buffer);
void compute_command_buffer_destroy(compute_pipeline_t* pipeline, compute_command_buffer_t* cmd_buffer);
compute_error_t compute_command_buffer_begin(compute_command_buffer_t* cmd_buffer);
compute_error_t compute_command_buffer_end(compute_command_buffer_t* cmd_buffer);
compute_error_t compute_command_buffer_dispatch(compute_command_buffer_t* cmd_buffer, const compute_dispatch_params_t* params);
compute_error_t compute_command_buffer_barrier(compute_command_buffer_t* cmd_buffer);

// Pipeline execution
compute_error_t compute_execute(compute_pipeline_t* pipeline, compute_command_buffer_t* cmd_buffer);
compute_error_t compute_execute_immediate(compute_pipeline_t* pipeline, compute_shader_t* shader, const compute_dispatch_params_t* params);

// Synchronization
compute_error_t compute_fence_create(compute_pipeline_t* pipeline, uint64_t* fence);
compute_error_t compute_fence_wait(compute_pipeline_t* pipeline, uint64_t fence, uint64_t timeout_ns);
compute_error_t compute_fence_signal(compute_pipeline_t* pipeline, uint64_t fence);

// Memory management
compute_error_t compute_memory_allocate(compute_pipeline_t* pipeline, uint64_t size, uint64_t* allocation);
compute_error_t compute_memory_deallocate(compute_pipeline_t* pipeline, uint64_t allocation);
compute_error_t compute_memory_get_usage(compute_pipeline_t* pipeline, uint64_t* used, uint64_t* total);

// Validation and debugging
compute_error_t compute_validate_shader(compute_pipeline_t* pipeline, compute_shader_t* shader);
compute_error_t compute_validate_resources(compute_pipeline_t* pipeline);
compute_error_t compute_debug_print_pipeline_state(compute_pipeline_t* pipeline);

// Statistics
compute_error_t compute_get_statistics(compute_pipeline_t* pipeline, compute_pipeline_stats_t* stats);
compute_error_t compute_reset_statistics(compute_pipeline_t* pipeline);

// Backend-specific functions
compute_error_t compute_backend_init(compute_pipeline_t* pipeline);
void compute_backend_shutdown(compute_pipeline_t* pipeline);
compute_error_t compute_backend_create_shader(compute_pipeline_t* pipeline, const compute_shader_desc_t* desc, compute_shader_t** shader);
compute_error_t compute_backend_create_resource(compute_pipeline_t* pipeline, const compute_resource_desc_t* desc, compute_resource_t** resource);
compute_error_t compute_backend_dispatch(compute_pipeline_t* pipeline, const compute_dispatch_params_t* params);

// Error handling
const char* compute_error_string(compute_error_t error);

#ifdef __cplusplus
}
#endif

#endif // COMPUTE_SHADER_PIPELINE_H
