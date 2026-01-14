/**
 * @file renderer_types.h
 * @brief Renderer subsystem type definitions
 */

#ifndef VOXELFORGE_RENDERER_TYPES_H
#define VOXELFORGE_RENDERER_TYPES_H

#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/Math/Vector/vec4.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct Renderer Renderer;
typedef struct RenderContext RenderContext;
typedef struct RenderView RenderView;
typedef struct RenderScene RenderScene;
typedef struct MeshInstance MeshInstance;
typedef struct Material Material;
typedef struct MaterialInstance MaterialInstance;
typedef struct Light Light;
typedef struct Camera Camera;

// ============================================================================
// Mesh Types
// ============================================================================

typedef struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec4 tangent; // xyz = tangent, w = handedness
  f32 uv[2];
} Vertex;

typedef struct MeshData {
  Vertex *vertices;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;
} MeshData;

typedef struct Mesh {
  ResourceHandle vertex_buffer;
  ResourceHandle index_buffer;
  u32 vertex_count;
  u32 index_count;
  AABB bounds;
} Mesh;

typedef struct MeshInstance {
  Mesh *mesh;
  Material *material;
  Mat4 transform;
  AABB world_bounds;
  u32 flags;
} MeshInstance;

// ============================================================================
// Material Types
// ============================================================================

typedef enum MaterialType {
  MATERIAL_TYPE_OPAQUE = 0,
  MATERIAL_TYPE_MASKED,
  MATERIAL_TYPE_TRANSPARENT,
  MATERIAL_TYPE_SUBSURFACE,
  MATERIAL_TYPE_CUSTOM,
} MaterialType;

typedef enum MaterialFlags {
  MATERIAL_FLAG_TWO_SIDED = (1 << 0),
  MATERIAL_FLAG_EMISSIVE = (1 << 1),
  MATERIAL_FLAG_VIRTUAL_TEXTURE = (1 << 2),
} MaterialFlags;

typedef struct MaterialParams {
  Vec4 base_color;
  f32 metallic;
  f32 roughness;
  f32 ao;
  f32 emissive_intensity;
  Vec3 emissive_color;
} MaterialParams;

typedef struct Material {
  MaterialType type;
  u32 flags;
  MaterialParams params;
  ResourceHandle albedo_texture;
  ResourceHandle normal_texture;
  ResourceHandle metallic_roughness_texture;
  ResourceHandle emissive_texture;
  ResourceHandle ao_texture;
} Material;

// ============================================================================
// Light Types
// ============================================================================

typedef enum LightType {
  LIGHT_TYPE_DIRECTIONAL = 0,
  LIGHT_TYPE_POINT,
  LIGHT_TYPE_SPOT,
  LIGHT_TYPE_AREA,
} LightType;

typedef struct Light {
  LightType type;
  Vec3 position;  // Point/Spot
  Vec3 direction; // Directional/Spot
  Vec3 color;
  f32 intensity;
  f32 range;       // Point/Spot
  f32 inner_angle; // Spot
  f32 outer_angle; // Spot
  Vec2 area_size;  // Area
  b8 cast_shadows;
  u32 shadow_map_index;
} Light;

// ============================================================================
// Camera
// ============================================================================

typedef enum ProjectionType {
  PROJECTION_PERSPECTIVE = 0,
  PROJECTION_ORTHOGRAPHIC,
} ProjectionType;

typedef struct Camera {
  ProjectionType type;
  Vec3 position;
  Quat rotation;
  f32 fov; // Perspective
  f32 near_plane;
  f32 far_plane;
  f32 ortho_size; // Orthographic
  f32 aspect_ratio;
  Mat4 view_matrix;
  Mat4 proj_matrix;
  Mat4 view_proj_matrix;
  Frustum frustum;
} Camera;

// ============================================================================
// Render View
// ============================================================================

typedef struct RenderView {
  Camera *camera;
  u32 viewport_x;
  u32 viewport_y;
  u32 viewport_width;
  u32 viewport_height;
  ResourceHandle render_target;
  ResourceHandle depth_target;
} RenderView;

// ============================================================================
// Render Statistics
// ============================================================================

typedef struct RenderStats {
  u64 draw_calls;
  u64 triangles_rendered;
  u64 instances_rendered;
  u64 instances_culled;
  u64 shadow_passes;
  f32 gpu_time_ms;
  f32 cpu_time_ms;
} RenderStats;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_RENDERER_TYPES_H
