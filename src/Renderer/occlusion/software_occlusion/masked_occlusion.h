/*
 * masked_occlusion.h
 * Masked occlusion culling
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef OCCLUSION_MASKED_OCCLUSION_H
#define OCCLUSION_MASKED_OCCLUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct occlusion_masked_occlusion_handle {
    uint32_t id;
} occlusion_masked_occlusion_handle_t;

typedef struct occlusion_masked_occlusion_desc {
    uint32_t flags;
    void* user_data;
} occlusion_masked_occlusion_desc_t;

typedef struct occlusion_masked_occlusion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} occlusion_masked_occlusion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int occlusion_masked_occlusion_init(void);
void occlusion_masked_occlusion_shutdown(void);

/* Lifecycle */
int occlusion_masked_occlusion_create(occlusion_masked_occlusion_handle_t* out_handle, const occlusion_masked_occlusion_desc_t* desc);
void occlusion_masked_occlusion_destroy(occlusion_masked_occlusion_handle_t handle);

/* Operations */
int occlusion_masked_occlusion_update(occlusion_masked_occlusion_handle_t handle, const void* data, size_t size);
bool occlusion_masked_occlusion_is_valid(occlusion_masked_occlusion_handle_t handle);
int occlusion_masked_occlusion_get_info(occlusion_masked_occlusion_handle_t handle, occlusion_masked_occlusion_info_t* out_info);
void occlusion_masked_occlusion_mark_dirty(occlusion_masked_occlusion_handle_t handle);
int occlusion_masked_occlusion_process_pending(void);

/* Statistics */
uint32_t occlusion_masked_occlusion_get_count(void);
size_t occlusion_masked_occlusion_get_memory_usage(void);
void occlusion_masked_occlusion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSION_MASKED_OCCLUSION_H */
