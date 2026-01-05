/*
 * color_grading_tod.h
 * TOD color grading
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_COLOR_GRADING_TOD_H
#define ATMOSPHERE_COLOR_GRADING_TOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_color_grading_tod_handle {
    uint32_t id;
} atmosphere_color_grading_tod_handle_t;

typedef struct atmosphere_color_grading_tod_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_color_grading_tod_desc_t;

typedef struct atmosphere_color_grading_tod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_color_grading_tod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_color_grading_tod_init(void);
void atmosphere_color_grading_tod_shutdown(void);

/* Lifecycle */
int atmosphere_color_grading_tod_create(atmosphere_color_grading_tod_handle_t* out_handle, const atmosphere_color_grading_tod_desc_t* desc);
void atmosphere_color_grading_tod_destroy(atmosphere_color_grading_tod_handle_t handle);

/* Operations */
int atmosphere_color_grading_tod_update(atmosphere_color_grading_tod_handle_t handle, const void* data, size_t size);
bool atmosphere_color_grading_tod_is_valid(atmosphere_color_grading_tod_handle_t handle);
int atmosphere_color_grading_tod_get_info(atmosphere_color_grading_tod_handle_t handle, atmosphere_color_grading_tod_info_t* out_info);
void atmosphere_color_grading_tod_mark_dirty(atmosphere_color_grading_tod_handle_t handle);
int atmosphere_color_grading_tod_process_pending(void);

/* Statistics */
uint32_t atmosphere_color_grading_tod_get_count(void);
size_t atmosphere_color_grading_tod_get_memory_usage(void);
void atmosphere_color_grading_tod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_COLOR_GRADING_TOD_H */
