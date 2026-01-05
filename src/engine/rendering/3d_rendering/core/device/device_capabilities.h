/*
 * device_capabilities.h
 * Hardware capability detection
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DEVICE_CAPABILITIES_H
#define CORE_DEVICE_CAPABILITIES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_device_capabilities_handle {
    uint32_t id;
} core_device_capabilities_handle_t;

typedef struct core_device_capabilities_desc {
    uint32_t flags;
    void* user_data;
} core_device_capabilities_desc_t;

typedef struct core_device_capabilities_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_device_capabilities_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_device_capabilities_init(void);
void core_device_capabilities_shutdown(void);

/* Lifecycle */
int core_device_capabilities_create(core_device_capabilities_handle_t* out_handle, const core_device_capabilities_desc_t* desc);
void core_device_capabilities_destroy(core_device_capabilities_handle_t handle);

/* Operations */
int core_device_capabilities_update(core_device_capabilities_handle_t handle, const void* data, size_t size);
bool core_device_capabilities_is_valid(core_device_capabilities_handle_t handle);
int core_device_capabilities_get_info(core_device_capabilities_handle_t handle, core_device_capabilities_info_t* out_info);
void core_device_capabilities_mark_dirty(core_device_capabilities_handle_t handle);
int core_device_capabilities_process_pending(void);

/* Statistics */
uint32_t core_device_capabilities_get_count(void);
size_t core_device_capabilities_get_memory_usage(void);
void core_device_capabilities_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEVICE_CAPABILITIES_H */
