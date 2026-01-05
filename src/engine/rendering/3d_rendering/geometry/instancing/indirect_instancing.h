/*
 * indirect_instancing.h
 * Indirect draw instancing
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INDIRECT_INSTANCING_H
#define GEOMETRY_INDIRECT_INSTANCING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_indirect_instancing_handle {
    uint32_t id;
} geometry_indirect_instancing_handle_t;

typedef struct geometry_indirect_instancing_desc {
    uint32_t flags;
    void* user_data;
} geometry_indirect_instancing_desc_t;

typedef struct geometry_indirect_instancing_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_indirect_instancing_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_indirect_instancing_init(void);
void geometry_indirect_instancing_shutdown(void);

/* Lifecycle */
int geometry_indirect_instancing_create(geometry_indirect_instancing_handle_t* out_handle, const geometry_indirect_instancing_desc_t* desc);
void geometry_indirect_instancing_destroy(geometry_indirect_instancing_handle_t handle);

/* Operations */
int geometry_indirect_instancing_update(geometry_indirect_instancing_handle_t handle, const void* data, size_t size);
bool geometry_indirect_instancing_is_valid(geometry_indirect_instancing_handle_t handle);
int geometry_indirect_instancing_get_info(geometry_indirect_instancing_handle_t handle, geometry_indirect_instancing_info_t* out_info);
void geometry_indirect_instancing_mark_dirty(geometry_indirect_instancing_handle_t handle);
int geometry_indirect_instancing_process_pending(void);

/* Statistics */
uint32_t geometry_indirect_instancing_get_count(void);
size_t geometry_indirect_instancing_get_memory_usage(void);
void geometry_indirect_instancing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INDIRECT_INSTANCING_H */
