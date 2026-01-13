/*
 * cloth_bones.h
 * Cloth bone simulation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_CLOTH_BONES_H
#define ANIMATION_CLOTH_BONES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_cloth_bones_handle {
    uint32_t id;
} animation_cloth_bones_handle_t;

typedef struct animation_cloth_bones_desc {
    uint32_t flags;
    void* user_data;
} animation_cloth_bones_desc_t;

typedef struct animation_cloth_bones_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_cloth_bones_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_cloth_bones_init(void);
void animation_cloth_bones_shutdown(void);

/* Lifecycle */
int animation_cloth_bones_create(animation_cloth_bones_handle_t* out_handle, const animation_cloth_bones_desc_t* desc);
void animation_cloth_bones_destroy(animation_cloth_bones_handle_t handle);

/* Operations */
int animation_cloth_bones_update(animation_cloth_bones_handle_t handle, const void* data, size_t size);
bool animation_cloth_bones_is_valid(animation_cloth_bones_handle_t handle);
int animation_cloth_bones_get_info(animation_cloth_bones_handle_t handle, animation_cloth_bones_info_t* out_info);
void animation_cloth_bones_mark_dirty(animation_cloth_bones_handle_t handle);
int animation_cloth_bones_process_pending(void);

/* Render Graph Integration */
/* Returns a handle to the render pass node created for this cloth simulation */
/* The return type is uint32_t to avoid including render_pass_node.h here, caller should cast to rendering_render_pass_node_handle_t */
uint32_t animation_cloth_bones_create_render_node(animation_cloth_bones_handle_t handle);

/* Statistics */
uint32_t animation_cloth_bones_get_count(void);
size_t animation_cloth_bones_get_memory_usage(void);
void animation_cloth_bones_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_CLOTH_BONES_H */
