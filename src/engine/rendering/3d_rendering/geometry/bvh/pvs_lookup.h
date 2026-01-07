/*
 * pvs_lookup.h
 * Precomputed visibility sets
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_PVS_LOOKUP_H
#define CULLING_PVS_LOOKUP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_pvs_lookup_handle {
    uint32_t id;
} culling_pvs_lookup_handle_t;

typedef struct culling_pvs_lookup_desc {
    uint32_t flags;
    void* user_data;
} culling_pvs_lookup_desc_t;

typedef struct culling_pvs_lookup_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_pvs_lookup_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_pvs_lookup_init(void);
void culling_pvs_lookup_shutdown(void);

/* Lifecycle */
int culling_pvs_lookup_create(culling_pvs_lookup_handle_t* out_handle, const culling_pvs_lookup_desc_t* desc);
void culling_pvs_lookup_destroy(culling_pvs_lookup_handle_t handle);

/* Operations */
int culling_pvs_lookup_update(culling_pvs_lookup_handle_t handle, const void* data, size_t size);
bool culling_pvs_lookup_is_valid(culling_pvs_lookup_handle_t handle);
int culling_pvs_lookup_get_info(culling_pvs_lookup_handle_t handle, culling_pvs_lookup_info_t* out_info);
void culling_pvs_lookup_mark_dirty(culling_pvs_lookup_handle_t handle);
int culling_pvs_lookup_process_pending(void);

/* Statistics */
uint32_t culling_pvs_lookup_get_count(void);
size_t culling_pvs_lookup_get_memory_usage(void);
void culling_pvs_lookup_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PVS_LOOKUP_H */
