/*
 * indirect_lighting.h
 * Indirect light accumulation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_INDIRECT_LIGHTING_H
#define LIGHTING_INDIRECT_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_indirect_lighting_handle {
    uint32_t id;
} lighting_indirect_lighting_handle_t;

typedef struct lighting_indirect_lighting_desc {
    uint32_t flags;
    void* user_data;
} lighting_indirect_lighting_desc_t;

typedef struct lighting_indirect_lighting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_indirect_lighting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_indirect_lighting_init(void);
void lighting_indirect_lighting_shutdown(void);

/* Lifecycle */
int lighting_indirect_lighting_create(lighting_indirect_lighting_handle_t* out_handle, const lighting_indirect_lighting_desc_t* desc);
void lighting_indirect_lighting_destroy(lighting_indirect_lighting_handle_t handle);

/* Operations */
int lighting_indirect_lighting_update(lighting_indirect_lighting_handle_t handle, const void* data, size_t size);
bool lighting_indirect_lighting_is_valid(lighting_indirect_lighting_handle_t handle);
int lighting_indirect_lighting_get_info(lighting_indirect_lighting_handle_t handle, lighting_indirect_lighting_info_t* out_info);
void lighting_indirect_lighting_mark_dirty(lighting_indirect_lighting_handle_t handle);
int lighting_indirect_lighting_process_pending(void);

/* Statistics */
uint32_t lighting_indirect_lighting_get_count(void);
size_t lighting_indirect_lighting_get_memory_usage(void);
void lighting_indirect_lighting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_INDIRECT_LIGHTING_H */
