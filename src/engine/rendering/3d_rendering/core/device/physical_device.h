/*
 * physical_device.h
 * Physical device selection
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_PHYSICAL_DEVICE_H
#define CORE_PHYSICAL_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_physical_device_handle {
    uint32_t id;
} core_physical_device_handle_t;

typedef struct core_physical_device_desc {
    uint32_t flags;
    void* user_data;
} core_physical_device_desc_t;

typedef struct core_physical_device_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_physical_device_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_physical_device_init(void);
void core_physical_device_shutdown(void);

/* Lifecycle */
int core_physical_device_create(core_physical_device_handle_t* out_handle, const core_physical_device_desc_t* desc);
void core_physical_device_destroy(core_physical_device_handle_t handle);

/* Operations */
int core_physical_device_update(core_physical_device_handle_t handle, const void* data, size_t size);
bool core_physical_device_is_valid(core_physical_device_handle_t handle);
int core_physical_device_get_info(core_physical_device_handle_t handle, core_physical_device_info_t* out_info);
void core_physical_device_mark_dirty(core_physical_device_handle_t handle);
int core_physical_device_process_pending(void);

/* Statistics */
uint32_t core_physical_device_get_count(void);
size_t core_physical_device_get_memory_usage(void);
void core_physical_device_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PHYSICAL_DEVICE_H */
