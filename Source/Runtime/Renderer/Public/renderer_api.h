/**
 * @file renderer_api.h
 * @brief Main Renderer API
 */

#ifndef VOXELFORGE_RENDERER_API_H
#define VOXELFORGE_RENDERER_API_H

#include "Core/Public/Math/Geometry/aabb.h"
#include "Core/Public/Math/Geometry/frustum.h"
#include "Renderer/Public/renderer_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Initialization
// ============================================================================

typedef struct RendererConfig {
  u32 render_width;
  u32 render_height;
  u32 shadow_map_resolution;
  u32 max_lights;
  u32 max_mesh_instances;
  b8 enable_raytracing;
  b8 enable_vsync;
  void *window_handle;
} RendererConfig;

VF_API VF_Result renderer_init(const RendererConfig *config);
VF_API void renderer_shutdown(void);
VF_API Renderer *renderer_get(void);

// ============================================================================
// Frame Management
// ============================================================================

VF_API void renderer_begin_frame(void);
VF_API void renderer_end_frame(void);
VF_API void renderer_present(void);
VF_API void renderer_resize(u32 width, u32 height);

// ============================================================================
// Camera
// ============================================================================

VF_API Camera *camera_create(void);
VF_API void camera_destroy(Camera *camera);
VF_API void camera_set_perspective(Camera *camera, f32 fov, f32 aspect,
                                   f32 near, f32 far);
VF_API void camera_set_orthographic(Camera *camera, f32 size, f32 aspect,
                                    f32 near, f32 far);
VF_API void camera_set_transform(Camera *camera, Vec3 position, Quat rotation);
VF_API void camera_look_at(Camera *camera, Vec3 position, Vec3 target, Vec3 up);
VF_API void camera_update_matrices(Camera *camera);

// ============================================================================
// Mesh Management
// ============================================================================

VF_API Mesh *mesh_create(const MeshData *data);
VF_API void mesh_destroy(Mesh *mesh);
VF_API Mesh *mesh_create_cube(f32 size);
VF_API Mesh *mesh_create_sphere(f32 radius, u32 segments);
VF_API Mesh *mesh_create_plane(f32 width, f32 height);

// ============================================================================
// Material Management
// ============================================================================

VF_API Material *material_create(MaterialType type);
VF_API void material_destroy(Material *material);
VF_API void material_set_params(Material *material,
                                const MaterialParams *params);
VF_API void material_set_texture(Material *material, const char *slot,
                                 ResourceHandle texture);

// ============================================================================
// Light Management
// ============================================================================

VF_API Light *light_create_directional(Vec3 direction, Vec3 color,
                                       f32 intensity);
VF_API Light *light_create_point(Vec3 position, Vec3 color, f32 intensity,
                                 f32 range);
VF_API Light *light_create_spot(Vec3 position, Vec3 direction, Vec3 color,
                                f32 intensity, f32 range, f32 inner_angle,
                                f32 outer_angle);
VF_API void light_destroy(Light *light);
VF_API void light_set_shadow_casting(Light *light, b8 cast_shadows);

// ============================================================================
// Scene Submission
// ============================================================================

VF_API void renderer_submit_mesh(Mesh *mesh, Material *material,
                                 Mat4 transform);
VF_API void renderer_submit_light(Light *light);
VF_API void renderer_set_camera(Camera *camera);
VF_API void renderer_set_environment_map(ResourceHandle cubemap);
VF_API void renderer_set_sky_light(Vec3 color, f32 intensity);

// ============================================================================
// Render Passes
// ============================================================================

VF_API void renderer_execute_shadow_pass(void);
VF_API void renderer_execute_gbuffer_pass(void);
VF_API void renderer_execute_lighting_pass(void);
VF_API void renderer_execute_transparent_pass(void);
VF_API void renderer_execute_post_process(void);
VF_API void renderer_execute_ui_pass(void);

// ============================================================================
// Debug Rendering
// ============================================================================

VF_API void debug_draw_line(Vec3 start, Vec3 end, Vec4 color);
VF_API void debug_draw_box(Vec3 center, Vec3 extents, Vec4 color);
VF_API void debug_draw_sphere(Vec3 center, f32 radius, Vec4 color);
VF_API void debug_draw_frustum(Frustum frustum, Vec4 color);
VF_API void debug_draw_text(Vec3 position, const char *text, Vec4 color);

// ============================================================================
// Statistics
// ============================================================================

VF_API void renderer_get_stats(RenderStats *out_stats);
VF_API void renderer_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_RENDERER_API_H
