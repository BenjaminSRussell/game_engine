#ifndef LANDSCAPE_TERRAIN_RENDERER_H
#define LANDSCAPE_TERRAIN_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h>
#include <math/mat4.h>
// #include <renderer/vulkan.h> // Avoid circular dependency if possible, use forward decls

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_renderer_handle {
    uint32_t id;
} landscape_terrain_renderer_handle_t;

typedef struct landscape_terrain_renderer_desc {
    uint32_t flags;
    void* user_data;
    // rendering configuration
    uint32_t max_lod_levels;
    float base_grid_scale;
    uint32_t grid_size;
} landscape_terrain_renderer_desc_t;

typedef struct landscape_terrain_renderer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t lod_levels_active;
} landscape_terrain_renderer_info_t;

// Forward declarations for renderer types
typedef struct VkBuffer_T* VkBuffer;
typedef struct VkPipeline_T* VkPipeline;
typedef struct VkDescriptorSet_T* VkDescriptorSet;
typedef struct VkCommandBuffer_T* VkCommandBuffer;
typedef uint64_t texture_handle_t; // Placeholder, adjust to actual type

// Forward declaration for camera
typedef struct Camera Camera;

typedef struct terrain_clipmap {
    texture_handle_t heightmap;
    texture_handle_t normalmap;
    texture_handle_t splatmap;
    uint32_t levels;              // Number of LOD levels (4-8)
    float base_scale;             // Base grid scale
    Vec3 center;                  // Camera-centered position (using Vec3 for consistency)
    uint32_t grid_size;           // Grid resolution per level (e.g., 128x128)
} terrain_clipmap_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_terrain_renderer_init(void);
void landscape_terrain_renderer_shutdown(void);

/* Lifecycle */
int landscape_terrain_renderer_create(landscape_terrain_renderer_handle_t* out_handle, const landscape_terrain_renderer_desc_t* desc);
void landscape_terrain_renderer_destroy(landscape_terrain_renderer_handle_t handle);

/* Operations */
int landscape_terrain_renderer_update(landscape_terrain_renderer_handle_t handle, const void* data, size_t size);
bool landscape_terrain_renderer_is_valid(landscape_terrain_renderer_handle_t handle);
int landscape_terrain_renderer_get_info(landscape_terrain_renderer_handle_t handle, landscape_terrain_renderer_info_t* out_info);
void landscape_terrain_renderer_mark_dirty(landscape_terrain_renderer_handle_t handle);
int landscape_terrain_renderer_process_pending(void);

/* Rendering */
void landscape_terrain_renderer_render(landscape_terrain_renderer_handle_t handle, Camera* camera, VkCommandBuffer cmd);
void landscape_terrain_update_clipmap_center(landscape_terrain_renderer_handle_t handle, Vec3 camera_pos);

/* Statistics */
uint32_t landscape_terrain_renderer_get_count(void);
size_t landscape_terrain_renderer_get_memory_usage(void);
void landscape_terrain_renderer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_RENDERER_H */
