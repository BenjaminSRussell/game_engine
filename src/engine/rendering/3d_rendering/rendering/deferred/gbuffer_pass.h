/*
 * gbuffer_pass.h
 * G-buffer geometry pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GBUFFER_PASS_H
#define RENDERING_GBUFFER_PASS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_gbuffer_pass_handle {
    uint32_t id;
} rendering_gbuffer_pass_handle_t;

typedef struct rendering_gbuffer_pass_desc {
    uint32_t flags;
    void* user_data;
} rendering_gbuffer_pass_desc_t;

typedef struct rendering_gbuffer_pass_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_gbuffer_pass_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_gbuffer_pass_init(void);
void rendering_gbuffer_pass_shutdown(void);

/* Lifecycle */
int rendering_gbuffer_pass_create(rendering_gbuffer_pass_handle_t* out_handle, const rendering_gbuffer_pass_desc_t* desc);
void rendering_gbuffer_pass_destroy(rendering_gbuffer_pass_handle_t handle);

/* Operations */
int rendering_gbuffer_pass_update(rendering_gbuffer_pass_handle_t handle, const void* data, size_t size);
bool rendering_gbuffer_pass_is_valid(rendering_gbuffer_pass_handle_t handle);
int rendering_gbuffer_pass_get_info(rendering_gbuffer_pass_handle_t handle, rendering_gbuffer_pass_info_t* out_info);
void rendering_gbuffer_pass_mark_dirty(rendering_gbuffer_pass_handle_t handle);
int rendering_gbuffer_pass_process_pending(void);

/* Statistics */
uint32_t rendering_gbuffer_pass_get_count(void);
size_t rendering_gbuffer_pass_get_memory_usage(void);
void rendering_gbuffer_pass_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GBUFFER_PASS_H */
