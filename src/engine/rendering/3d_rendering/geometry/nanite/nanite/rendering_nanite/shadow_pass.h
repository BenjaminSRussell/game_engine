/*
 * shadow_pass.h
 * Shadow rendering pass
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_SHADOW_PASS_H
#define NANITE_SHADOW_PASS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_shadow_pass_handle {
    uint32_t id;
} nanite_shadow_pass_handle_t;

typedef struct nanite_shadow_pass_desc {
    uint32_t flags;
    void* user_data;
} nanite_shadow_pass_desc_t;

typedef struct nanite_shadow_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_shadow_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_shadow_pass_init(void);
void nanite_shadow_pass_shutdown(void);

/* Lifecycle */
int nanite_shadow_pass_create(nanite_shadow_pass_handle_t* out_handle, const nanite_shadow_pass_desc_t* desc);
void nanite_shadow_pass_destroy(nanite_shadow_pass_handle_t handle);

/* Operations */
int nanite_shadow_pass_update(nanite_shadow_pass_handle_t handle, const void* data, size_t size);
bool nanite_shadow_pass_is_valid(nanite_shadow_pass_handle_t handle);
int nanite_shadow_pass_get_info(nanite_shadow_pass_handle_t handle, nanite_shadow_pass_info_t* out_info);
void nanite_shadow_pass_mark_dirty(nanite_shadow_pass_handle_t handle);
int nanite_shadow_pass_process_pending(void);

/* Statistics */
uint32_t nanite_shadow_pass_get_count(void);
size_t nanite_shadow_pass_get_memory_usage(void);
void nanite_shadow_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_SHADOW_PASS_H */
