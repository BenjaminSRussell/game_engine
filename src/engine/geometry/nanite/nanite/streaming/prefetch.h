/*
 * prefetch.h
 * Cluster prefetching
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_PREFETCH_H
#define NANITE_PREFETCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_prefetch_handle {
    uint32_t id;
} nanite_prefetch_handle_t;

typedef struct nanite_prefetch_desc {
    uint32_t flags;
    void* user_data;
} nanite_prefetch_desc_t;

typedef struct nanite_prefetch_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_prefetch_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_prefetch_init(void);
void nanite_prefetch_shutdown(void);

/* Lifecycle */
int nanite_prefetch_create(nanite_prefetch_handle_t* out_handle, const nanite_prefetch_desc_t* desc);
void nanite_prefetch_destroy(nanite_prefetch_handle_t handle);

/* Operations */
int nanite_prefetch_update(nanite_prefetch_handle_t handle, const void* data, size_t size);
bool nanite_prefetch_is_valid(nanite_prefetch_handle_t handle);
int nanite_prefetch_get_info(nanite_prefetch_handle_t handle, nanite_prefetch_info_t* out_info);
void nanite_prefetch_mark_dirty(nanite_prefetch_handle_t handle);
int nanite_prefetch_process_pending(void);

/* Statistics */
uint32_t nanite_prefetch_get_count(void);
size_t nanite_prefetch_get_memory_usage(void);
void nanite_prefetch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_PREFETCH_H */
