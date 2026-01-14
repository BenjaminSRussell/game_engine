/*
 * streaming_budget.h
 * Texture memory budget
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAMING_BUDGET_H
#define TEXTURE_STREAMING_BUDGET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_streaming_budget_handle {
    uint32_t id;
} texture_streaming_budget_handle_t;

typedef struct texture_streaming_budget_desc {
    uint32_t flags;
    void* user_data;
} texture_streaming_budget_desc_t;

typedef struct texture_streaming_budget_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_streaming_budget_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_streaming_budget_init(void);
void texture_streaming_budget_shutdown(void);

/* Lifecycle */
int texture_streaming_budget_create(texture_streaming_budget_handle_t* out_handle, const texture_streaming_budget_desc_t* desc);
void texture_streaming_budget_destroy(texture_streaming_budget_handle_t handle);

/* Operations */
int texture_streaming_budget_update(texture_streaming_budget_handle_t handle, const void* data, size_t size);
bool texture_streaming_budget_is_valid(texture_streaming_budget_handle_t handle);
int texture_streaming_budget_get_info(texture_streaming_budget_handle_t handle, texture_streaming_budget_info_t* out_info);
void texture_streaming_budget_mark_dirty(texture_streaming_budget_handle_t handle);
int texture_streaming_budget_process_pending(void);

/* Statistics */
uint32_t texture_streaming_budget_get_count(void);
size_t texture_streaming_budget_get_memory_usage(void);
void texture_streaming_budget_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAMING_BUDGET_H */
