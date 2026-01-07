/*
 * runtime_fracture.h
 * Runtime fracturing
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_RUNTIME_FRACTURE_H
#define DESTRUCTION_RUNTIME_FRACTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_runtime_fracture_handle {
    uint32_t id;
} destruction_runtime_fracture_handle_t;

typedef struct destruction_runtime_fracture_desc {
    uint32_t flags;
    void* user_data;
} destruction_runtime_fracture_desc_t;

typedef struct destruction_runtime_fracture_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_runtime_fracture_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_runtime_fracture_init(void);
void destruction_runtime_fracture_shutdown(void);

/* Lifecycle */
int destruction_runtime_fracture_create(destruction_runtime_fracture_handle_t* out_handle, const destruction_runtime_fracture_desc_t* desc);
void destruction_runtime_fracture_destroy(destruction_runtime_fracture_handle_t handle);

/* Operations */
int destruction_runtime_fracture_update(destruction_runtime_fracture_handle_t handle, const void* data, size_t size);
bool destruction_runtime_fracture_is_valid(destruction_runtime_fracture_handle_t handle);
int destruction_runtime_fracture_get_info(destruction_runtime_fracture_handle_t handle, destruction_runtime_fracture_info_t* out_info);
void destruction_runtime_fracture_mark_dirty(destruction_runtime_fracture_handle_t handle);
int destruction_runtime_fracture_process_pending(void);

/* Statistics */
uint32_t destruction_runtime_fracture_get_count(void);
size_t destruction_runtime_fracture_get_memory_usage(void);
void destruction_runtime_fracture_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_RUNTIME_FRACTURE_H */
