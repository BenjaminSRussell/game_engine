/*
 * hair_strand.h
 * Hair strand representation
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef HAIR_SYSTEM_HAIR_STRAND_H
#define HAIR_SYSTEM_HAIR_STRAND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hair_system_hair_strand_handle {
    uint32_t id;
} hair_system_hair_strand_handle_t;

typedef struct hair_system_hair_strand_desc {
    uint32_t flags;
    void* user_data;
} hair_system_hair_strand_desc_t;

typedef struct hair_system_hair_strand_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} hair_system_hair_strand_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int hair_system_hair_strand_init(void);
void hair_system_hair_strand_shutdown(void);

/* Lifecycle */
int hair_system_hair_strand_create(hair_system_hair_strand_handle_t* out_handle, const hair_system_hair_strand_desc_t* desc);
void hair_system_hair_strand_destroy(hair_system_hair_strand_handle_t handle);

/* Operations */
int hair_system_hair_strand_update(hair_system_hair_strand_handle_t handle, const void* data, size_t size);
bool hair_system_hair_strand_is_valid(hair_system_hair_strand_handle_t handle);
int hair_system_hair_strand_get_info(hair_system_hair_strand_handle_t handle, hair_system_hair_strand_info_t* out_info);
void hair_system_hair_strand_mark_dirty(hair_system_hair_strand_handle_t handle);
int hair_system_hair_strand_process_pending(void);

/* Statistics */
uint32_t hair_system_hair_strand_get_count(void);
size_t hair_system_hair_strand_get_memory_usage(void);
void hair_system_hair_strand_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* HAIR_SYSTEM_HAIR_STRAND_H */
