/*
 * dead_list.h
 * Dead particle recycling
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_DEAD_LIST_H
#define EFFECTS_DEAD_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_dead_list_handle {
    uint32_t id;
} effects_dead_list_handle_t;

typedef struct effects_dead_list_desc {
    uint32_t flags;
    void* user_data;
} effects_dead_list_desc_t;

typedef struct effects_dead_list_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_dead_list_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_dead_list_init(void);
void effects_dead_list_shutdown(void);

/* Lifecycle */
int effects_dead_list_create(effects_dead_list_handle_t* out_handle, const effects_dead_list_desc_t* desc);
void effects_dead_list_destroy(effects_dead_list_handle_t handle);

/* Operations */
int effects_dead_list_update(effects_dead_list_handle_t handle, const void* data, size_t size);
bool effects_dead_list_is_valid(effects_dead_list_handle_t handle);
int effects_dead_list_get_info(effects_dead_list_handle_t handle, effects_dead_list_info_t* out_info);
void effects_dead_list_mark_dirty(effects_dead_list_handle_t handle);
int effects_dead_list_process_pending(void);

/* Statistics */
uint32_t effects_dead_list_get_count(void);
size_t effects_dead_list_get_memory_usage(void);
void effects_dead_list_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_DEAD_LIST_H */
