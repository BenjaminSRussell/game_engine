#ifndef LANDSCAPE_SPLAT_MATERIALS_H
#define LANDSCAPE_SPLAT_MATERIALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

#define LANDSCAPE_SPLAT_MAX_LAYERS 16

typedef struct landscape_splat_materials_handle {
    uint32_t id;
} landscape_splat_materials_handle_t;

typedef struct landscape_splat_materials_desc {
    uint32_t flags;
    void* user_data;
} landscape_splat_materials_desc_t;

typedef struct landscape_splat_materials_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t active_layers;
} landscape_splat_materials_info_t;

typedef struct splat_material_layer {
    char* albedo_path;
    char* normal_path;
    char* roughness_path;
    char* height_path; // For displacement/parallax
    float tiling_scale;
    float displacement_scale;
    bool active;
} splat_material_layer_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_splat_materials_init(void);
void landscape_splat_materials_shutdown(void);

/* Lifecycle */
int landscape_splat_materials_create(landscape_splat_materials_handle_t* out_handle, const landscape_splat_materials_desc_t* desc);
void landscape_splat_materials_destroy(landscape_splat_materials_handle_t handle);

/* Material Management */
int landscape_splat_materials_set_layer(
    landscape_splat_materials_handle_t handle,
    uint32_t layer_index,
    const splat_material_layer_t* layer
);

int landscape_splat_materials_get_layer(
    landscape_splat_materials_handle_t handle,
    uint32_t layer_index,
    splat_material_layer_t* out_layer
);

/* Operations */

// Upload textures to GPU (placeholder)
int landscape_splat_materials_upload(landscape_splat_materials_handle_t handle);

bool landscape_splat_materials_is_valid(landscape_splat_materials_handle_t handle);
int landscape_splat_materials_get_info(landscape_splat_materials_handle_t handle, landscape_splat_materials_info_t* out_info);
void landscape_splat_materials_mark_dirty(landscape_splat_materials_handle_t handle);
int landscape_splat_materials_process_pending(void);

/* Statistics */
uint32_t landscape_splat_materials_get_count(void);
size_t landscape_splat_materials_get_memory_usage(void);
void landscape_splat_materials_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_SPLAT_MATERIALS_H */
