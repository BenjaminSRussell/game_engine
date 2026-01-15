/*
 * prefetch_system.h
 * Predictive mesh loading
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_PREFETCH_SYSTEM_H
#define GEOMETRY_PREFETCH_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_prefetch_system_handle {
    uint32_t id;
} geometry_prefetch_system_handle_t;

typedef struct geometry_prefetch_system_desc {
    uint32_t flags;
    void* user_data;
} geometry_prefetch_system_desc_t;

typedef struct geometry_prefetch_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_prefetch_system_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_prefetch_system_init(void);
void geometry_prefetch_system_shutdown(void);

/* Lifecycle */
int geometry_prefetch_system_create(geometry_prefetch_system_handle_t* out_handle, const geometry_prefetch_system_desc_t* desc);
void geometry_prefetch_system_destroy(geometry_prefetch_system_handle_t handle);

/* Operations */
int geometry_prefetch_system_update(geometry_prefetch_system_handle_t handle, const void* data, size_t size);
bool geometry_prefetch_system_is_valid(geometry_prefetch_system_handle_t handle);
int geometry_prefetch_system_get_info(geometry_prefetch_system_handle_t handle, geometry_prefetch_system_info_t* out_info);
void geometry_prefetch_system_mark_dirty(geometry_prefetch_system_handle_t handle);
int geometry_prefetch_system_process_pending(void);

/* Statistics */
uint32_t geometry_prefetch_system_get_count(void);
size_t geometry_prefetch_system_get_memory_usage(void);
void geometry_prefetch_system_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_PREFETCH_SYSTEM_H */
