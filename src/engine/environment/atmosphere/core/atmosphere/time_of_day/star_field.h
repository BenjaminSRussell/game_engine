/*
 * star_field.h
 * Night sky stars
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_STAR_FIELD_H
#define ATMOSPHERE_STAR_FIELD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_star_field_handle {
    uint32_t id;
} atmosphere_star_field_handle_t;

typedef struct atmosphere_star_field_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_star_field_desc_t;

typedef struct atmosphere_star_field_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_star_field_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_star_field_init(void);
void atmosphere_star_field_shutdown(void);

/* Lifecycle */
int atmosphere_star_field_create(atmosphere_star_field_handle_t* out_handle, const atmosphere_star_field_desc_t* desc);
void atmosphere_star_field_destroy(atmosphere_star_field_handle_t handle);

/* Operations */
int atmosphere_star_field_update(atmosphere_star_field_handle_t handle, const void* data, size_t size);
bool atmosphere_star_field_is_valid(atmosphere_star_field_handle_t handle);
int atmosphere_star_field_get_info(atmosphere_star_field_handle_t handle, atmosphere_star_field_info_t* out_info);
void atmosphere_star_field_mark_dirty(atmosphere_star_field_handle_t handle);
int atmosphere_star_field_process_pending(void);

/* Statistics */
uint32_t atmosphere_star_field_get_count(void);
size_t atmosphere_star_field_get_memory_usage(void);
void atmosphere_star_field_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_STAR_FIELD_H */
