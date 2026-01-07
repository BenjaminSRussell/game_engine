/*
 * indirect_args.h
 * Indirect draw arguments
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_INDIRECT_ARGS_H
#define EFFECTS_INDIRECT_ARGS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_indirect_args_handle {
    uint32_t id;
} effects_indirect_args_handle_t;

typedef struct effects_indirect_args_desc {
    uint32_t flags;
    void* user_data;
} effects_indirect_args_desc_t;

typedef struct effects_indirect_args_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_indirect_args_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_indirect_args_init(void);
void effects_indirect_args_shutdown(void);

/* Lifecycle */
int effects_indirect_args_create(effects_indirect_args_handle_t* out_handle, const effects_indirect_args_desc_t* desc);
void effects_indirect_args_destroy(effects_indirect_args_handle_t handle);

/* Operations */
int effects_indirect_args_update(effects_indirect_args_handle_t handle, const void* data, size_t size);
bool effects_indirect_args_is_valid(effects_indirect_args_handle_t handle);
int effects_indirect_args_get_info(effects_indirect_args_handle_t handle, effects_indirect_args_info_t* out_info);
void effects_indirect_args_mark_dirty(effects_indirect_args_handle_t handle);
int effects_indirect_args_process_pending(void);

/* Statistics */
uint32_t effects_indirect_args_get_count(void);
size_t effects_indirect_args_get_memory_usage(void);
void effects_indirect_args_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_INDIRECT_ARGS_H */
