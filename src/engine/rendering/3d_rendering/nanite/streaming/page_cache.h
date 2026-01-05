/*
 * page_cache.h
 * Cluster page caching
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_PAGE_CACHE_H
#define NANITE_PAGE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_page_cache_handle {
    uint32_t id;
} nanite_page_cache_handle_t;

typedef struct nanite_page_cache_desc {
    uint32_t flags;
    void* user_data;
} nanite_page_cache_desc_t;

typedef struct nanite_page_cache_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_page_cache_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_page_cache_init(void);
void nanite_page_cache_shutdown(void);

/* Lifecycle */
int nanite_page_cache_create(nanite_page_cache_handle_t* out_handle, const nanite_page_cache_desc_t* desc);
void nanite_page_cache_destroy(nanite_page_cache_handle_t handle);

/* Operations */
int nanite_page_cache_update(nanite_page_cache_handle_t handle, const void* data, size_t size);
bool nanite_page_cache_is_valid(nanite_page_cache_handle_t handle);
int nanite_page_cache_get_info(nanite_page_cache_handle_t handle, nanite_page_cache_info_t* out_info);
void nanite_page_cache_mark_dirty(nanite_page_cache_handle_t handle);
int nanite_page_cache_process_pending(void);

/* Statistics */
uint32_t nanite_page_cache_get_count(void);
size_t nanite_page_cache_get_memory_usage(void);
void nanite_page_cache_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_PAGE_CACHE_H */
