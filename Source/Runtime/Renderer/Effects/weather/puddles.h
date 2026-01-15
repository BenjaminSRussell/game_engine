/*
 * puddles.h
 * Dynamic puddle rendering
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PUDDLES_H
#define EFFECTS_PUDDLES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_puddles_handle {
    uint32_t id;
} effects_puddles_handle_t;

typedef struct effects_puddles_desc {
    uint32_t flags;
    void* user_data;
} effects_puddles_desc_t;

typedef struct effects_puddles_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_puddles_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_puddles_init(void);
void effects_puddles_shutdown(void);

/* Lifecycle */
int effects_puddles_create(effects_puddles_handle_t* out_handle, const effects_puddles_desc_t* desc);
void effects_puddles_destroy(effects_puddles_handle_t handle);

/* Operations */
int effects_puddles_update(effects_puddles_handle_t handle, const void* data, size_t size);
bool effects_puddles_is_valid(effects_puddles_handle_t handle);
int effects_puddles_get_info(effects_puddles_handle_t handle, effects_puddles_info_t* out_info);
void effects_puddles_mark_dirty(effects_puddles_handle_t handle);
int effects_puddles_process_pending(void);

/* Statistics */
uint32_t effects_puddles_get_count(void);
size_t effects_puddles_get_memory_usage(void);
void effects_puddles_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PUDDLES_H */
