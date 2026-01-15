/*
 * compute_dispatch.h
 * Centralized GPU compute shader dispatch framework
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_COMPUTE_DISPATCH_H
#define RENDERING_COMPUTE_DISPATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;
typedef struct metal_buffer metal_buffer_t;
typedef struct metal_compute_pipeline metal_compute_pipeline_t;

#ifdef __OBJC__
@class MTLCommandBuffer;
@class MTLComputeCommandEncoder;
#else
typedef void MTLCommandBuffer;
typedef void MTLComputeCommandEncoder;
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Compute dispatch descriptor */
typedef struct compute_dispatch_desc {
    metal_device_t* device;           // Metal device
    const char* shader_path;          // Path to Metal shader file
    const char* kernel_name;          // Kernel function name
    uint32_t threadgroup_size_x;      // Threadgroup size X
    uint32_t threadgroup_size_y;      // Threadgroup size Y (default 1)
    uint32_t threadgroup_size_z;      // Threadgroup size Z (default 1)
} compute_dispatch_desc_t;

/* Compute dispatch statistics */
typedef struct compute_dispatch_stats {
    uint32_t total_dispatches;        // Total dispatch calls
    uint64_t total_gpu_time_ns;       // Total GPU time in nanoseconds
    uint32_t threads_per_call;        // Threads in last dispatch
    uint32_t peak_threadgroups;       // Peak threadgroups in one dispatch
} compute_dispatch_stats_t;

/* Compute dispatcher handle */
typedef struct compute_dispatcher_handle {
    uint32_t id;
} compute_dispatcher_handle_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

/**
 * Initialize compute dispatch system
 * @return 0 on success, negative on error
 */
int compute_dispatch_init(void);

/**
 * Shutdown compute dispatch system
 */
void compute_dispatch_shutdown(void);

/* ============================================================================
 * API - DISPATCHER LIFECYCLE
 * ============================================================================ */

/**
 * Create a compute dispatcher
 * @param out_handle Output dispatcher handle
 * @param desc Dispatcher descriptor
 * @return 0 on success, negative on error
 */
int compute_dispatch_create(compute_dispatcher_handle_t* out_handle,
                           const compute_dispatch_desc_t* desc);

/**
 * Destroy a compute dispatcher
 */
void compute_dispatch_destroy(compute_dispatcher_handle_t handle);

/**
 * Check if dispatcher is valid
 */
bool compute_dispatch_is_valid(compute_dispatcher_handle_t handle);

/* ============================================================================
 * API - BUFFER BINDING
 * ============================================================================ */

/**
 * Bind buffer to compute shader
 * @param handle Dispatcher handle
 * @param buffer_index Buffer binding index
 * @param buffer Metal buffer to bind
 * @param offset Offset within buffer
 * @return 0 on success, negative on error
 */
int compute_dispatch_bind_buffer(compute_dispatcher_handle_t handle,
                                uint32_t buffer_index,
                                metal_buffer_t* buffer,
                                size_t offset);

/**
 * Bind multiple buffers at once
 * @param handle Dispatcher handle
 * @param buffers Array of metal buffers
 * @param buffer_count Number of buffers
 * @return 0 on success, negative on error
 */
int compute_dispatch_bind_buffers(compute_dispatcher_handle_t handle,
                                 metal_buffer_t** buffers,
                                 uint32_t buffer_count);

/**
 * Clear all bound buffers
 */
void compute_dispatch_clear_buffers(compute_dispatcher_handle_t handle);

/* ============================================================================
 * API - DISPATCH
 * ============================================================================ */

/**
 * Dispatch compute kernel
 * @param handle Dispatcher handle
 * @param command_buffer Metal command buffer
 * @param thread_count Total number of threads to dispatch
 * @return 0 on success, negative on error
 */
int compute_dispatch_execute(compute_dispatcher_handle_t handle,
                            MTLCommandBuffer* command_buffer,
                            uint32_t thread_count);

/**
 * Dispatch with explicit threadgroup count
 * @param handle Dispatcher handle
 * @param command_buffer Metal command buffer
 * @param threadgroup_count Number of threadgroups
 * @return 0 on success, negative on error
 */
int compute_dispatch_execute_threadgroups(compute_dispatcher_handle_t handle,
                                         MTLCommandBuffer* command_buffer,
                                         uint32_t threadgroup_count);

/**
 * Dispatch and measure GPU time
 * @param handle Dispatcher handle
 * @param command_buffer Metal command buffer
 * @param thread_count Total number of threads
 * @param out_gpu_time_ns Output GPU time in nanoseconds (optional)
 * @return 0 on success, negative on error
 */
int compute_dispatch_execute_timed(compute_dispatcher_handle_t handle,
                                  MTLCommandBuffer* command_buffer,
                                  uint32_t thread_count,
                                  uint64_t* out_gpu_time_ns);

/* ============================================================================
 * API - STATISTICS
 * ============================================================================ */

/**
 * Get dispatcher statistics
 * @param handle Dispatcher handle
 * @param out_stats Output statistics
 * @return 0 on success, negative on error
 */
int compute_dispatch_get_stats(compute_dispatcher_handle_t handle,
                              compute_dispatch_stats_t* out_stats);

/**
 * Reset dispatcher statistics
 */
void compute_dispatch_reset_stats(compute_dispatcher_handle_t handle);

/**
 * Get total memory usage of all dispatchers
 */
size_t compute_dispatch_get_total_memory(void);

/**
 * Print debug information for all dispatchers
 */
void compute_dispatch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_COMPUTE_DISPATCH_H */
