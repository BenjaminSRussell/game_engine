/*
 * hzb_first_person.h
 * First-person HZB
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef OCCLUSION_HZB_FIRST_PERSON_H
#define OCCLUSION_HZB_FIRST_PERSON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct occlusion_hzb_first_person_handle {
    uint32_t id;
} occlusion_hzb_first_person_handle_t;

typedef struct occlusion_hzb_first_person_desc {
    uint32_t flags;
    void* user_data;
} occlusion_hzb_first_person_desc_t;

typedef struct occlusion_hzb_first_person_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} occlusion_hzb_first_person_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int occlusion_hzb_first_person_init(void);
void occlusion_hzb_first_person_shutdown(void);

/* Lifecycle */
int occlusion_hzb_first_person_create(occlusion_hzb_first_person_handle_t* out_handle, const occlusion_hzb_first_person_desc_t* desc);
void occlusion_hzb_first_person_destroy(occlusion_hzb_first_person_handle_t handle);

/* Operations */
int occlusion_hzb_first_person_update(occlusion_hzb_first_person_handle_t handle, const void* data, size_t size);
bool occlusion_hzb_first_person_is_valid(occlusion_hzb_first_person_handle_t handle);
int occlusion_hzb_first_person_get_info(occlusion_hzb_first_person_handle_t handle, occlusion_hzb_first_person_info_t* out_info);
void occlusion_hzb_first_person_mark_dirty(occlusion_hzb_first_person_handle_t handle);
int occlusion_hzb_first_person_process_pending(void);

/* Statistics */
uint32_t occlusion_hzb_first_person_get_count(void);
size_t occlusion_hzb_first_person_get_memory_usage(void);
void occlusion_hzb_first_person_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSION_HZB_FIRST_PERSON_H */
