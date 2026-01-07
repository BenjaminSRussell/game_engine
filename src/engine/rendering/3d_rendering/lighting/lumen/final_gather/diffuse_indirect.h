/*
 * diffuse_indirect.h
 * Diffuse indirect lighting
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_DIFFUSE_INDIRECT_H
#define LUMEN_DIFFUSE_INDIRECT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_diffuse_indirect_handle {
    uint32_t id;
} lumen_diffuse_indirect_handle_t;

typedef struct lumen_diffuse_indirect_desc {
    uint32_t flags;
    void* user_data;
} lumen_diffuse_indirect_desc_t;

typedef struct lumen_diffuse_indirect_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_diffuse_indirect_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_diffuse_indirect_init(void);
void lumen_diffuse_indirect_shutdown(void);

/* Lifecycle */
int lumen_diffuse_indirect_create(lumen_diffuse_indirect_handle_t* out_handle, const lumen_diffuse_indirect_desc_t* desc);
void lumen_diffuse_indirect_destroy(lumen_diffuse_indirect_handle_t handle);

/* Operations */
int lumen_diffuse_indirect_update(lumen_diffuse_indirect_handle_t handle, const void* data, size_t size);
bool lumen_diffuse_indirect_is_valid(lumen_diffuse_indirect_handle_t handle);
int lumen_diffuse_indirect_get_info(lumen_diffuse_indirect_handle_t handle, lumen_diffuse_indirect_info_t* out_info);
void lumen_diffuse_indirect_mark_dirty(lumen_diffuse_indirect_handle_t handle);
int lumen_diffuse_indirect_process_pending(void);

/* Statistics */
uint32_t lumen_diffuse_indirect_get_count(void);
size_t lumen_diffuse_indirect_get_memory_usage(void);
void lumen_diffuse_indirect_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_DIFFUSE_INDIRECT_H */
