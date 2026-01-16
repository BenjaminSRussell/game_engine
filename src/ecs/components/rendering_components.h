#ifndef RENDERING_COMPONENTS_H
#define RENDERING_COMPONENTS_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * Rendering Components for ECS
 *
 * These components enable entity-based rendering:
 * - MeshRendererComponent: Renderable mesh with material
 * - CameraComponent: Viewpoint for rendering
 * - LightComponent: Light source
 */

// ============================================================================
// MESH RENDERER COMPONENT
// ============================================================================

typedef struct {
  u32 mesh_id;         // Reference to mesh resource
  u32 material_id;     // Reference to material resource
  bool visible;        // Visibility flag
  bool cast_shadow;    // Shadow casting
  bool receive_shadow; // Shadow receiving
  u8 render_layer;     // Render layer (0-255)
  f32 lod_bias;        // LOD bias
} MeshRendererComponent;

// ============================================================================
// CAMERA COMPONENT
// ============================================================================

typedef enum {
  CAMERA_PROJECTION_PERSPECTIVE,
  CAMERA_PROJECTION_ORTHOGRAPHIC
} CameraProjection;

typedef struct {
  CameraProjection projection;

  // Perspective parameters
  f32 fov;          // Field of view (degrees)
  f32 aspect_ratio; // Width / height

  // Orthographic parameters
  f32 ortho_size; // Orthographic size

  // Common parameters
  f32 near_plane; // Near clipping plane
  f32 far_plane;  // Far clipping plane

  // Render target
  u32 render_target_id; // 0 = screen

  // Viewport
  f32 viewport_x; // Normalized [0, 1]
  f32 viewport_y;
  f32 viewport_width;
  f32 viewport_height;

  // Priority (higher = rendered later)
  u32 priority;

  // Flags
  bool enabled;
  bool clear_color;
  bool clear_depth;
  Vec4 clear_color_value;
} CameraComponent;

// ============================================================================
// LIGHT COMPONENT
// ============================================================================

typedef enum {
  LIGHT_TYPE_DIRECTIONAL,
  LIGHT_TYPE_POINT,
  LIGHT_TYPE_SPOT
} LightType;

typedef struct {
  LightType type;

  Vec3 color;    // RGB color
  f32 intensity; // Light intensity

  // Point/Spot light parameters
  f32 range;       // Maximum range
  f32 attenuation; // Attenuation factor

  // Spot light parameters
  f32 inner_cone_angle; // Inner cone (degrees)
  f32 outer_cone_angle; // Outer cone (degrees)

  // Shadow parameters
  bool cast_shadow;
  u32 shadow_map_size; // Shadow map resolution
  f32 shadow_bias;

  // Flags
  bool enabled;
} LightComponent;

// ============================================================================
// SKYBOX COMPONENT
// ============================================================================

typedef struct {
  u32 cubemap_id; // Skybox cubemap texture
  f32 exposure;   // HDR exposure
  f32 rotation;   // Rotation (degrees)
  bool enabled;
} SkyboxComponent;

// ============================================================================
// COMPONENT IDs (Global)
// ============================================================================

extern u32 g_mesh_renderer_component_id;
extern u32 g_camera_component_id;
extern u32 g_light_component_id;
extern u32 g_skybox_component_id;

// ============================================================================
// COMPONENT REGISTRATION
// ============================================================================

/**
 * Register all rendering components with ECS world
 * Call once at startup after ECS initialization
 */
void register_rendering_components(void *world);

/**
 * Helper functions for component creation
 */
MeshRendererComponent mesh_renderer_create(u32 mesh_id, u32 material_id);
CameraComponent camera_create_perspective(f32 fov, f32 aspect, f32 near,
                                          f32 far);
CameraComponent camera_create_orthographic(f32 size, f32 aspect, f32 near,
                                           f32 far);
LightComponent light_create_directional(Vec3 color, f32 intensity);
LightComponent light_create_point(Vec3 color, f32 intensity, f32 range);
LightComponent light_create_spot(Vec3 color, f32 intensity, f32 range,
                                 f32 inner_angle, f32 outer_angle);

#endif // RENDERING_COMPONENTS_H
