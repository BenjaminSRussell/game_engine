#ifndef LANDSCAPE_TREE_RENDERER_H
#define LANDSCAPE_TREE_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_tree_renderer_handle {
    uint32_t id;
} landscape_tree_renderer_handle_t;

typedef struct landscape_tree_renderer_desc {
    uint32_t flags;
    void* user_data;
    const char* mesh_path; // Only one mesh per renderer instance for now
    const char* material_path;
} landscape_tree_renderer_desc_t;

typedef struct landscape_tree_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t instance_count;
    uint32_t visible_count;
} landscape_tree_renderer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_tree_renderer_init(void);
void landscape_tree_renderer_shutdown(void);

/* Lifecycle */
int landscape_tree_renderer_create(landscape_tree_renderer_handle_t* out_handle, const landscape_tree_renderer_desc_t* desc);
void landscape_tree_renderer_destroy(landscape_tree_renderer_handle_t handle);

/* Instance Management */
int landscape_tree_renderer_add_instances(
    landscape_tree_renderer_handle_t handle,
    const Vec3* positions,
    const float* scales,
    const float* rotations,
    uint32_t count
);

int landscape_tree_renderer_clear_instances(landscape_tree_renderer_handle_t handle);

/* Operations */

// Update for culling
int landscape_tree_renderer_update(landscape_tree_renderer_handle_t handle, const Vec3* camera_pos, const Vec3* camera_dir);
void landscape_tree_renderer_render(landscape_tree_renderer_handle_t handle, const void* render_context);

bool landscape_tree_renderer_is_valid(landscape_tree_renderer_handle_t handle);
int landscape_tree_renderer_get_info(landscape_tree_renderer_handle_t handle, landscape_tree_renderer_info_t* out_info);
void landscape_tree_renderer_mark_dirty(landscape_tree_renderer_handle_t handle);
int landscape_tree_renderer_process_pending(void);

/* Statistics */
uint32_t landscape_tree_renderer_get_count(void);
size_t landscape_tree_renderer_get_memory_usage(void);
void landscape_tree_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TREE_RENDERER_H */
