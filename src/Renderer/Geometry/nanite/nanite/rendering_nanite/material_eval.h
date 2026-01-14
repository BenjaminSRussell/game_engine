/*
 * material_eval.h
 * Material evaluation
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_MATERIAL_EVAL_H
#define NANITE_MATERIAL_EVAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_material_eval_handle {
    uint32_t id;
} nanite_material_eval_handle_t;

typedef struct nanite_material_eval_desc {
    uint32_t flags;
    void* user_data;
} nanite_material_eval_desc_t;

typedef struct nanite_material_eval_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_material_eval_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_material_eval_init(void);
void nanite_material_eval_shutdown(void);

/* Lifecycle */
int nanite_material_eval_create(nanite_material_eval_handle_t* out_handle, const nanite_material_eval_desc_t* desc);
void nanite_material_eval_destroy(nanite_material_eval_handle_t handle);

/* Operations */
int nanite_material_eval_update(nanite_material_eval_handle_t handle, const void* data, size_t size);
bool nanite_material_eval_is_valid(nanite_material_eval_handle_t handle);
int nanite_material_eval_get_info(nanite_material_eval_handle_t handle, nanite_material_eval_info_t* out_info);
void nanite_material_eval_mark_dirty(nanite_material_eval_handle_t handle);
int nanite_material_eval_process_pending(void);

/* Statistics */
uint32_t nanite_material_eval_get_count(void);
size_t nanite_material_eval_get_memory_usage(void);
void nanite_material_eval_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_MATERIAL_EVAL_H */
