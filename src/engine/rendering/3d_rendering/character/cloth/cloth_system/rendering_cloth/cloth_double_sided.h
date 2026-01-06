/*
 * cloth_double_sided.h
 * Double-sided rendering
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SYSTEM_CLOTH_DOUBLE_SIDED_H
#define CLOTH_SYSTEM_CLOTH_DOUBLE_SIDED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_system_cloth_double_sided_handle {
    uint32_t id;
} cloth_system_cloth_double_sided_handle_t;

typedef struct cloth_system_cloth_double_sided_desc {
    uint32_t flags;
    void* user_data;
} cloth_system_cloth_double_sided_desc_t;

typedef struct cloth_system_cloth_double_sided_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} cloth_system_cloth_double_sided_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int cloth_system_cloth_double_sided_init(void);
void cloth_system_cloth_double_sided_shutdown(void);

/* Lifecycle */
int cloth_system_cloth_double_sided_create(cloth_system_cloth_double_sided_handle_t* out_handle, const cloth_system_cloth_double_sided_desc_t* desc);
void cloth_system_cloth_double_sided_destroy(cloth_system_cloth_double_sided_handle_t handle);

/* Operations */
int cloth_system_cloth_double_sided_update(cloth_system_cloth_double_sided_handle_t handle, const void* data, size_t size);
bool cloth_system_cloth_double_sided_is_valid(cloth_system_cloth_double_sided_handle_t handle);
int cloth_system_cloth_double_sided_get_info(cloth_system_cloth_double_sided_handle_t handle, cloth_system_cloth_double_sided_info_t* out_info);
void cloth_system_cloth_double_sided_mark_dirty(cloth_system_cloth_double_sided_handle_t handle);
int cloth_system_cloth_double_sided_process_pending(void);

/* Statistics */
uint32_t cloth_system_cloth_double_sided_get_count(void);
size_t cloth_system_cloth_double_sided_get_memory_usage(void);
void cloth_system_cloth_double_sided_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SYSTEM_CLOTH_DOUBLE_SIDED_H */
