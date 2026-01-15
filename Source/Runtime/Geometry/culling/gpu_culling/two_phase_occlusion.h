/*
 * two_phase_occlusion.h
 * Two-phase occlusion
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_TWO_PHASE_OCCLUSION_H
#define CULLING_TWO_PHASE_OCCLUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_two_phase_occlusion_handle {
    uint32_t id;
} culling_two_phase_occlusion_handle_t;

typedef struct culling_two_phase_occlusion_desc {
    uint32_t flags;
    void* user_data;
} culling_two_phase_occlusion_desc_t;

typedef struct culling_two_phase_occlusion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_two_phase_occlusion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_two_phase_occlusion_init(void);
void culling_two_phase_occlusion_shutdown(void);

/* Lifecycle */
int culling_two_phase_occlusion_create(culling_two_phase_occlusion_handle_t* out_handle, const culling_two_phase_occlusion_desc_t* desc);
void culling_two_phase_occlusion_destroy(culling_two_phase_occlusion_handle_t handle);

/* Operations */
int culling_two_phase_occlusion_update(culling_two_phase_occlusion_handle_t handle, const void* data, size_t size);
bool culling_two_phase_occlusion_is_valid(culling_two_phase_occlusion_handle_t handle);
int culling_two_phase_occlusion_get_info(culling_two_phase_occlusion_handle_t handle, culling_two_phase_occlusion_info_t* out_info);
void culling_two_phase_occlusion_mark_dirty(culling_two_phase_occlusion_handle_t handle);
int culling_two_phase_occlusion_process_pending(void);

/* Statistics */
uint32_t culling_two_phase_occlusion_get_count(void);
size_t culling_two_phase_occlusion_get_memory_usage(void);
void culling_two_phase_occlusion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_TWO_PHASE_OCCLUSION_H */
