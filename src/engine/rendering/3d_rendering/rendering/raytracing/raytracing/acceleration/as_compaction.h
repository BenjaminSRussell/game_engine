/*
 * as_compaction.h
 * AS memory compaction
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_AS_COMPACTION_H
#define RAYTRACING_AS_COMPACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_as_compaction_handle {
    uint32_t id;
} raytracing_as_compaction_handle_t;

typedef struct raytracing_as_compaction_desc {
    uint32_t flags;
    void* user_data;
} raytracing_as_compaction_desc_t;

typedef struct raytracing_as_compaction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_as_compaction_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_as_compaction_init(void);
void raytracing_as_compaction_shutdown(void);

/* Lifecycle */
int raytracing_as_compaction_create(raytracing_as_compaction_handle_t* out_handle, const raytracing_as_compaction_desc_t* desc);
void raytracing_as_compaction_destroy(raytracing_as_compaction_handle_t handle);

/* Operations */
int raytracing_as_compaction_update(raytracing_as_compaction_handle_t handle, const void* data, size_t size);
bool raytracing_as_compaction_is_valid(raytracing_as_compaction_handle_t handle);
int raytracing_as_compaction_get_info(raytracing_as_compaction_handle_t handle, raytracing_as_compaction_info_t* out_info);
void raytracing_as_compaction_mark_dirty(raytracing_as_compaction_handle_t handle);
int raytracing_as_compaction_process_pending(void);

/* Statistics */
uint32_t raytracing_as_compaction_get_count(void);
size_t raytracing_as_compaction_get_memory_usage(void);
void raytracing_as_compaction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_AS_COMPACTION_H */
