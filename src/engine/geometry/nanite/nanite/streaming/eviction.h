/*
 * eviction.h
 * Page eviction policy
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_EVICTION_H
#define NANITE_EVICTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_eviction_handle {
    uint32_t id;
} nanite_eviction_handle_t;

typedef struct nanite_eviction_desc {
    uint32_t flags;
    void* user_data;
} nanite_eviction_desc_t;

typedef struct nanite_eviction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_eviction_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_eviction_init(void);
void nanite_eviction_shutdown(void);

/* Lifecycle */
int nanite_eviction_create(nanite_eviction_handle_t* out_handle, const nanite_eviction_desc_t* desc);
void nanite_eviction_destroy(nanite_eviction_handle_t handle);

/* Operations */
int nanite_eviction_update(nanite_eviction_handle_t handle, const void* data, size_t size);
bool nanite_eviction_is_valid(nanite_eviction_handle_t handle);
int nanite_eviction_get_info(nanite_eviction_handle_t handle, nanite_eviction_info_t* out_info);
void nanite_eviction_mark_dirty(nanite_eviction_handle_t handle);
int nanite_eviction_process_pending(void);

/* Statistics */
uint32_t nanite_eviction_get_count(void);
size_t nanite_eviction_get_memory_usage(void);
void nanite_eviction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_EVICTION_H */
