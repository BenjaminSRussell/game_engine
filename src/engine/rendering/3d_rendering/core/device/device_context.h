/*
 * device_context.h
 * GPU device context management
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DEVICE_CONTEXT_H
#define CORE_DEVICE_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "device_capabilities.h"
#include "../memory/gpu_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct render_device {
    void* backend_handle;  // VkDevice, id<MTLDevice>, ID3D12Device*
    render_device_caps_t caps;
    render_memory_allocator_t* allocator;
    uint32_t queue_family_indices[RENDER_QUEUE_TYPE_COUNT];
} render_device_t;

typedef struct core_device_context_handle {
    uint32_t id;
} core_device_context_handle_t;

typedef struct core_device_context_desc {
    uint32_t flags;
    void* user_data;
} core_device_context_desc_t;

typedef struct core_device_context_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_device_context_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_device_context_init(void);
void core_device_context_shutdown(void);

/* Lifecycle */
int core_device_context_create(core_device_context_handle_t* out_handle, const core_device_context_desc_t* desc);
void core_device_context_destroy(core_device_context_handle_t handle);

/* Operations */
int core_device_context_update(core_device_context_handle_t handle, const void* data, size_t size);
bool core_device_context_is_valid(core_device_context_handle_t handle);
int core_device_context_get_info(core_device_context_handle_t handle, core_device_context_info_t* out_info);
void core_device_context_mark_dirty(core_device_context_handle_t handle);
int core_device_context_process_pending(void);

/* Statistics */
uint32_t core_device_context_get_count(void);
size_t core_device_context_get_memory_usage(void);
void core_device_context_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEVICE_CONTEXT_H */
