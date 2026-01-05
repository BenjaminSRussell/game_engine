/*
 * barrier_batch.h
 * Resource barrier batching
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_BARRIER_BATCH_H
#define CORE_BARRIER_BATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_barrier_batch_handle {
    uint32_t id;
} core_barrier_batch_handle_t;

typedef struct core_barrier_batch_desc {
    uint32_t flags;
    void* user_data;
} core_barrier_batch_desc_t;

typedef struct core_barrier_batch_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_barrier_batch_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_barrier_batch_init(void);
void core_barrier_batch_shutdown(void);

/* Lifecycle */
int core_barrier_batch_create(core_barrier_batch_handle_t* out_handle, const core_barrier_batch_desc_t* desc);
void core_barrier_batch_destroy(core_barrier_batch_handle_t handle);

/* Operations */
int core_barrier_batch_update(core_barrier_batch_handle_t handle, const void* data, size_t size);
bool core_barrier_batch_is_valid(core_barrier_batch_handle_t handle);
int core_barrier_batch_get_info(core_barrier_batch_handle_t handle, core_barrier_batch_info_t* out_info);
void core_barrier_batch_mark_dirty(core_barrier_batch_handle_t handle);
int core_barrier_batch_process_pending(void);

/* Statistics */
uint32_t core_barrier_batch_get_count(void);
size_t core_barrier_batch_get_memory_usage(void);
void core_barrier_batch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BARRIER_BATCH_H */
