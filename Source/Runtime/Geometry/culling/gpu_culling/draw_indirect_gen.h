/*
 * draw_indirect_gen.h
 * Indirect draw generation
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_DRAW_INDIRECT_GEN_H
#define CULLING_DRAW_INDIRECT_GEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_draw_indirect_gen_handle {
    uint32_t id;
} culling_draw_indirect_gen_handle_t;

typedef struct culling_draw_indirect_gen_desc {
    uint32_t flags;
    void* user_data;
} culling_draw_indirect_gen_desc_t;

typedef struct culling_draw_indirect_gen_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_draw_indirect_gen_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_draw_indirect_gen_init(void);
void culling_draw_indirect_gen_shutdown(void);

/* Lifecycle */
int culling_draw_indirect_gen_create(culling_draw_indirect_gen_handle_t* out_handle, const culling_draw_indirect_gen_desc_t* desc);
void culling_draw_indirect_gen_destroy(culling_draw_indirect_gen_handle_t handle);

/* Operations */
int culling_draw_indirect_gen_update(culling_draw_indirect_gen_handle_t handle, const void* data, size_t size);
bool culling_draw_indirect_gen_is_valid(culling_draw_indirect_gen_handle_t handle);
int culling_draw_indirect_gen_get_info(culling_draw_indirect_gen_handle_t handle, culling_draw_indirect_gen_info_t* out_info);
void culling_draw_indirect_gen_mark_dirty(culling_draw_indirect_gen_handle_t handle);
int culling_draw_indirect_gen_process_pending(void);

/* Statistics */
uint32_t culling_draw_indirect_gen_get_count(void);
size_t culling_draw_indirect_gen_get_memory_usage(void);
void culling_draw_indirect_gen_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_DRAW_INDIRECT_GEN_H */
