/*
 * pipeline_cache.h
 * Pipeline state object caching
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_PIPELINE_CACHE_H
#define CORE_PIPELINE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_pipeline_cache_handle {
    uint32_t id;
} core_pipeline_cache_handle_t;

typedef struct core_pipeline_cache_desc {
    uint32_t flags;
    void* user_data;
} core_pipeline_cache_desc_t;

typedef struct core_pipeline_cache_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_pipeline_cache_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_pipeline_cache_init(void);
void core_pipeline_cache_shutdown(void);

/* Lifecycle */
int core_pipeline_cache_create(core_pipeline_cache_handle_t* out_handle, const core_pipeline_cache_desc_t* desc);
void core_pipeline_cache_destroy(core_pipeline_cache_handle_t handle);

/* Operations */
int core_pipeline_cache_update(core_pipeline_cache_handle_t handle, const void* data, size_t size);
bool core_pipeline_cache_is_valid(core_pipeline_cache_handle_t handle);
int core_pipeline_cache_get_info(core_pipeline_cache_handle_t handle, core_pipeline_cache_info_t* out_info);
void core_pipeline_cache_mark_dirty(core_pipeline_cache_handle_t handle);
int core_pipeline_cache_process_pending(void);

/* Statistics */
uint32_t core_pipeline_cache_get_count(void);
size_t core_pipeline_cache_get_memory_usage(void);
void core_pipeline_cache_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PIPELINE_CACHE_H */
