/*
 * portal_culling.h
 * Portal visibility
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_PORTAL_CULLING_H
#define CULLING_PORTAL_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_portal_culling_handle {
    uint32_t id;
} culling_portal_culling_handle_t;

typedef struct culling_portal_culling_desc {
    uint32_t flags;
    void* user_data;
} culling_portal_culling_desc_t;

typedef struct culling_portal_culling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_portal_culling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_portal_culling_init(void);
void culling_portal_culling_shutdown(void);

/* Lifecycle */
int culling_portal_culling_create(culling_portal_culling_handle_t* out_handle, const culling_portal_culling_desc_t* desc);
void culling_portal_culling_destroy(culling_portal_culling_handle_t handle);

/* Operations */
int culling_portal_culling_update(culling_portal_culling_handle_t handle, const void* data, size_t size);
bool culling_portal_culling_is_valid(culling_portal_culling_handle_t handle);
int culling_portal_culling_get_info(culling_portal_culling_handle_t handle, culling_portal_culling_info_t* out_info);
void culling_portal_culling_mark_dirty(culling_portal_culling_handle_t handle);
int culling_portal_culling_process_pending(void);

/* Statistics */
uint32_t culling_portal_culling_get_count(void);
size_t culling_portal_culling_get_memory_usage(void);
void culling_portal_culling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PORTAL_CULLING_H */
