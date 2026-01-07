/*
 * fabric_patterns.h
 * Fabric pattern detail
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_FABRIC_PATTERNS_H
#define SHADING_FABRIC_PATTERNS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_fabric_patterns_handle {
    uint32_t id;
} shading_fabric_patterns_handle_t;

typedef struct shading_fabric_patterns_desc {
    uint32_t flags;
    void* user_data;
} shading_fabric_patterns_desc_t;

typedef struct shading_fabric_patterns_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_fabric_patterns_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_fabric_patterns_init(void);
void shading_fabric_patterns_shutdown(void);

/* Lifecycle */
int shading_fabric_patterns_create(shading_fabric_patterns_handle_t* out_handle, const shading_fabric_patterns_desc_t* desc);
void shading_fabric_patterns_destroy(shading_fabric_patterns_handle_t handle);

/* Operations */
int shading_fabric_patterns_update(shading_fabric_patterns_handle_t handle, const void* data, size_t size);
bool shading_fabric_patterns_is_valid(shading_fabric_patterns_handle_t handle);
int shading_fabric_patterns_get_info(shading_fabric_patterns_handle_t handle, shading_fabric_patterns_info_t* out_info);
void shading_fabric_patterns_mark_dirty(shading_fabric_patterns_handle_t handle);
int shading_fabric_patterns_process_pending(void);

/* Statistics */
uint32_t shading_fabric_patterns_get_count(void);
size_t shading_fabric_patterns_get_memory_usage(void);
void shading_fabric_patterns_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_FABRIC_PATTERNS_H */
