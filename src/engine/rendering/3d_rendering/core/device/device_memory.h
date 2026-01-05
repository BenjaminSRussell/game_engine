/*
 * device_memory.h
 * Device memory allocation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DEVICE_MEMORY_H
#define CORE_DEVICE_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_device_memory_handle {
    uint32_t id;
} core_device_memory_handle_t;

typedef struct core_device_memory_desc {
    uint32_t flags;
    void* user_data;
} core_device_memory_desc_t;

typedef struct core_device_memory_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_device_memory_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_device_memory_init(void);
void core_device_memory_shutdown(void);

/* Lifecycle */
int core_device_memory_create(core_device_memory_handle_t* out_handle, const core_device_memory_desc_t* desc);
void core_device_memory_destroy(core_device_memory_handle_t handle);

/* Operations */
int core_device_memory_update(core_device_memory_handle_t handle, const void* data, size_t size);
bool core_device_memory_is_valid(core_device_memory_handle_t handle);
int core_device_memory_get_info(core_device_memory_handle_t handle, core_device_memory_info_t* out_info);
void core_device_memory_mark_dirty(core_device_memory_handle_t handle);
int core_device_memory_process_pending(void);

/* Statistics */
uint32_t core_device_memory_get_count(void);
size_t core_device_memory_get_memory_usage(void);
void core_device_memory_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEVICE_MEMORY_H */
