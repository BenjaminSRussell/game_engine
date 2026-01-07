/*
 * wetness_mask.h
 * Surface wetness mask
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_WETNESS_MASK_H
#define EFFECTS_WETNESS_MASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_wetness_mask_handle {
    uint32_t id;
} effects_wetness_mask_handle_t;

typedef struct effects_wetness_mask_desc {
    uint32_t flags;
    void* user_data;
} effects_wetness_mask_desc_t;

typedef struct effects_wetness_mask_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_wetness_mask_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_wetness_mask_init(void);
void effects_wetness_mask_shutdown(void);

/* Lifecycle */
int effects_wetness_mask_create(effects_wetness_mask_handle_t* out_handle, const effects_wetness_mask_desc_t* desc);
void effects_wetness_mask_destroy(effects_wetness_mask_handle_t handle);

/* Operations */
int effects_wetness_mask_update(effects_wetness_mask_handle_t handle, const void* data, size_t size);
bool effects_wetness_mask_is_valid(effects_wetness_mask_handle_t handle);
int effects_wetness_mask_get_info(effects_wetness_mask_handle_t handle, effects_wetness_mask_info_t* out_info);
void effects_wetness_mask_mark_dirty(effects_wetness_mask_handle_t handle);
int effects_wetness_mask_process_pending(void);

/* Statistics */
uint32_t effects_wetness_mask_get_count(void);
size_t effects_wetness_mask_get_memory_usage(void);
void effects_wetness_mask_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_WETNESS_MASK_H */
