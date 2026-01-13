// include/engine/renderer.h
//
// Purpose: Abstract renderer interface that decouples games from specific
// rendering implementations (Vulkan, OpenGL, DirectX, etc.).
//
// RENDER-API-001: Add support for multiple rendering backends
// RENDER-API-002: Implement render graph system
// RENDER-API-003: Add material system abstraction
// RENDER-API-004: Implement shader hot-reloading
// RENDER-API-005: Add render target management
// RENDER-API-006: Implement GPU memory management
// RENDER-API-007: Add debugging and profiling tools
// RENDER-API-008: Implement async rendering
// RENDER-API-009: Add VR/AR support
//
#ifndef ENGINE_RENDERER_H
#define ENGINE_RENDERER_H

#include <common.h>
#include "engine/include/math/math.h"

// Forward declarations
typedef struct Renderer Renderer;
typedef struct RenderTarget RenderTarget;
typedef struct Material Material;
typedef struct Mesh Mesh;
typedef struct Texture Texture;
typedef struct Shader Shader;
typedef struct Camera Camera;

// Rendering configuration
typedef struct {
  u32 max_frames_in_flight;
  bool enable_validation;
  bool enable_ray_tracing;
  bool enable_vsync;
  u32 max_texture_size;
  u32 max_mesh_vertices;
  u32 max_mesh_indices;
} RendererConfig;

// Render statistics
typedef struct {
  u32 draw_calls;
  u32 triangles;
  u32 vertices;
  f32 frame_time_ms;
  f32 gpu_time_ms;
  u64 memory_used;
  u64 memory_allocated;
} RenderStats;

// Color format
typedef enum {
  COLOR_FORMAT_R8G8B8A8_UNORM,
  COLOR_FORMAT_R8G8B8A8_SRGB,
  COLOR_FORMAT_R32G32B32A32_FLOAT,
  COLOR_FORMAT_R16G16B16A16_FLOAT,
  COLOR_FORMAT_D32_FLOAT
} ColorFormat;

// Primitive topology
typedef enum {
  PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
  PRIMITIVE_TOPOLOGY_LINE_LIST,
  PRIMITIVE_TOPOLOGY_POINT_LIST
} PrimitiveTopology;

// Blend mode
typedef enum {
  BLEND_MODE_OPAQUE,
  BLEND_MODE_TRANSPARENT,
  BLEND_MODE_ADDITIVE,
  BLEND_MODE_MULTIPLY
} BlendMode;

// Cull mode
typedef enum { CULL_MODE_NONE, CULL_MODE_FRONT, CULL_MODE_BACK } CullMode;

// Render target descriptor
typedef struct {
  u32 width;
  u32 height;
  ColorFormat format;
  bool enable_depth;
  bool enable_stencil;
  u32 sample_count;
} RenderTargetDesc;

// Texture descriptor
typedef struct {
  u32 width;
  u32 height;
  u32 depth;
  u32 mip_levels;
  ColorFormat format;
  bool generate_mips;
  void *data;
  size_t data_size;
} TextureDesc;

// Mesh descriptor
typedef struct {
  void *vertices;
  size_t vertex_count;
  size_t vertex_stride;
  void *indices;
  size_t index_count;
  size_t index_stride;
  PrimitiveTopology topology;
  u32 material_index; // Index into ImportedModel.materials or similar
} MeshDesc;

// Material properties
typedef struct {
  Vec3 albedo;
  Vec3 emission;
  f32 metallic;
  f32 roughness;
  f32 ao;
  f32 alpha;
  BlendMode blend_mode;
  CullMode cull_mode;
  bool double_sided;
} MaterialProperties;

// Renderer interface functions
typedef struct Renderer {
  // Lifecycle
  bool (*init)(Renderer *renderer, const RendererConfig *config);
  void (*shutdown)(Renderer *renderer);
  void (*begin_frame)(Renderer *renderer);
  void (*end_frame)(Renderer *renderer);

  // Resource management
  Texture *(*create_texture)(Renderer *renderer, const TextureDesc *desc);
  void (*destroy_texture)(Renderer *renderer, Texture *texture);
  Mesh *(*create_mesh)(Renderer *renderer, const MeshDesc *desc);
  void (*destroy_mesh)(Renderer *renderer, Mesh *mesh);
  Material *(*create_material)(Renderer *renderer,
                               const MaterialProperties *props);
  void (*destroy_material)(Renderer *renderer, Material *material);
  RenderTarget *(*create_render_target)(Renderer *renderer,
                                        const RenderTargetDesc *desc);
  void (*destroy_render_target)(Renderer *renderer, RenderTarget *target);

  // Rendering commands
  void (*set_render_target)(Renderer *renderer, RenderTarget *target);
  void (*clear)(Renderer *renderer, Vec4 color, float depth, u32 stencil);
  void (*set_viewport)(Renderer *renderer, u32 x, u32 y, u32 width, u32 height);
  void (*set_scissor)(Renderer *renderer, u32 x, u32 y, u32 width, u32 height);
  void (*bind_material)(Renderer *renderer, Material *material);
  void (*bind_mesh)(Renderer *renderer, Mesh *mesh);
  void (*draw)(Renderer *renderer, u32 vertex_count, u32 start_vertex);
  void (*draw_indexed)(Renderer *renderer, u32 index_count, u32 start_index);
  void (*draw_instanced)(Renderer *renderer, u32 vertex_count,
                         u32 instance_count, u32 start_vertex);

  // Camera and transforms
  void (*set_camera)(Renderer *renderer, const Camera *camera);
  void (*set_model_matrix)(Renderer *renderer, Mat4 matrix);
  void (*set_view_matrix)(Renderer *renderer, Mat4 matrix);
  void (*set_projection_matrix)(Renderer *renderer, Mat4 matrix);

  // Lighting
  void (*set_ambient_light)(Renderer *renderer, Vec3 color, f32 intensity);
  void (*set_directional_light)(Renderer *renderer, Vec3 direction, Vec3 color,
                                f32 intensity);
  void (*set_point_light)(Renderer *renderer, u32 index, Vec3 position,
                          Vec3 color, f32 intensity, f32 radius);
  void (*set_spot_light)(Renderer *renderer, u32 index, Vec3 position,
                         Vec3 direction, Vec3 color, f32 intensity, f32 radius,
                         f32 cone_angle);

  // Debug and utilities
  void (*draw_debug_line)(Renderer *renderer, Vec3 start, Vec3 end, Vec3 color);
  void (*draw_debug_box)(Renderer *renderer, Vec3 min, Vec3 max, Vec3 color);
  void (*draw_debug_sphere)(Renderer *renderer, Vec3 center, f32 radius,
                            Vec3 color);
  RenderStats (*get_stats)(Renderer *renderer);
  void (*reset_stats)(Renderer *renderer);

  // Platform-specific data
  void *platform_data;
} Renderer;

// Renderer factory functions
Renderer *create_vulkan_renderer(void);
Renderer *create_opengl_renderer(void);
Renderer *create_directx11_renderer(void);
Renderer *create_directx12_renderer(void);

// Utility functions
RendererConfig renderer_create_default_config(void);
bool renderer_validate_config(const RendererConfig *config);
const char *renderer_get_backend_name(Renderer *renderer);

#endif // ENGINE_RENDERER_H
