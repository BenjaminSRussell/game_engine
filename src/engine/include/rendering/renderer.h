// include/render/renderer.h
//
// Purpose: Defines the abstract Renderer interface that decouples rendering
// logic from specific rendering backends (Vulkan, OpenGL, 2.5D sprite
// renderer). This allows the game logic to work with any renderer
// implementation while keeping entity/world logic intact.
//
// Public APIs:
// - `IRenderer`: Abstract renderer interface with function pointers for all
//   rendering operations
// - `RendererType`: Enumeration for different renderer implementations
// - Concrete implementations: VoxelRenderer (wraps VulkanRenderer) and
//   Sprite3DRenderer (for 2.5D rendering)
//
// Ownership: Renderer implementations manage their own resources. The IRenderer
// interface provides a vtable-like structure for polymorphic rendering.
//
// Invariants:
// - All function pointers in IRenderer must be non-NULL after initialization
// - Renderer implementations must properly initialize all function pointers
// - The renderer must be initialized before use and cleaned up afterwards
//
#ifndef RENDERER_H
#define RENDERER_H

#include <common.h>
#include <core/gpu_acceleration.h>
#include <ecs/ecs.h>
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/quat.h>

// Forward declarations
struct Chunk;
struct Chunk;
typedef struct Chunk Chunk;
struct Mesh;
typedef struct Mesh Mesh;
struct PlayerSystem;
struct Camera;

// Renderer type enumeration
typedef enum {
  RENDERER_TYPE_VOXEL,     // 3D voxel rendering (Vulkan)
  RENDERER_TYPE_SPRITE_3D, // 2.5D sprite rendering
  RENDERER_TYPE_GRAPH,     // Glass-Box Render Graph
  RENDERER_TYPE_COUNT
} RendererType;

// Renderer initialization parameters
typedef struct {
  void *window; // Platform window handle
  u32 width;
  u32 height;
  RendererType type;
  GPUBackend backend;
  struct GameConfig *config;
} RendererInitParams;

// Abstract renderer interface (vtable-like structure)
typedef struct IRenderer {
  // Renderer type
  RendererType type;

  // Lifecycle
  bool (*init)(struct IRenderer *self, RendererInitParams *params);
  void (*cleanup)(struct IRenderer *self);
  void (*resize)(struct IRenderer *self, u32 width, u32 height);

  // Frame management
  bool (*begin_frame)(struct IRenderer *self, u32 *image_index);
  void (*end_frame)(struct IRenderer *self, u32 image_index);

  // Camera management
  void (*update_camera)(struct IRenderer *self, struct Camera *camera,
                        f32 aspect);
  void (*update_camera_uniforms)(struct IRenderer *self, struct Camera *camera,
                                 f32 aspect);

  // Chunk/World rendering
  void (*render_chunk)(struct IRenderer *self, Chunk *chunk, Mat4 view,
                       Mat4 proj);
  void (*render_chunk_mesh)(struct IRenderer *self, Chunk *chunk, Mat4 view,
                            Mat4 proj);
  void (*render_dynamic_mesh)(struct IRenderer *self, Mesh *mesh, Mat4 view,
                              Mat4 proj);

  // Entity/Sprite rendering (for 2.5D)
  void (*render_sprite)(struct IRenderer *self, Vec3 position, Vec2 size,
                        u32 texture_id, f32 rotation);
  void (*render_entity_sprite)(struct IRenderer *self, Entity entity,
                               Vec3 position, Vec2 size, u32 texture_id);

  // UI rendering
  void (*render_ui_quad)(struct IRenderer *self, Vec2 pos, Vec2 size,
                         u32 texture_id);
  void (*render_text)(struct IRenderer *self, const char *text, Vec2 pos,
                      f32 scale, Vec3 color);

  // Special effects
  void (*render_block_highlight)(struct IRenderer *self,
                                 struct PlayerSystem *player_system);
  void (*render_physics_debug)(struct IRenderer *self, Mat4 view, Mat4 proj);

  // Debug rendering
  void (*render_debug_line)(struct IRenderer *self, Vec3 start, Vec3 end, Vec3 color);
  void (*render_debug_box)(struct IRenderer *self, Vec3 center, Vec3 size, Quat rotation, Vec3 color);
  void (*render_debug_sphere)(struct IRenderer *self, Vec3 center, f32 radius, Vec3 color);

  // Lighting
  void (*set_ambient_light)(struct IRenderer *self, f32 ambient_light);

  // Buffer management (for mesh updates)
  bool (*create_chunk_buffers)(struct IRenderer *self, Mesh *mesh,
                               void **vertex_buffer, void **index_buffer);
  bool (*update_chunk_buffers)(struct IRenderer *self, Mesh *mesh,
                               void *vertex_buffer, void *index_buffer);

  // Internal implementation data (opaque pointer)
  void *impl_data;

  // Backend Info
  const char *(*get_backend_name)(struct IRenderer *self);
} IRenderer;

// Factory function to create renderer
IRenderer *renderer_create(RendererType type);
IRenderer *renderer_create_with_backend(RendererType type, GPUBackend backend);
void renderer_destroy(IRenderer *renderer);

// Convenience macros for calling renderer functions
#define RENDERER_INIT(renderer, params) ((renderer)->init((renderer), (params)))
#define RENDERER_CLEANUP(renderer) ((renderer)->cleanup((renderer)))
#define RENDERER_RESIZE(renderer, w, h)                                        \
  ((renderer)->resize((renderer), (w), (h)))
#define RENDERER_BEGIN_FRAME(renderer, idx)                                    \
  ((renderer)->begin_frame((renderer), (idx)))
#define RENDERER_END_FRAME(renderer, idx)                                      \
  ((renderer)->end_frame((renderer), (idx)))
#define RENDERER_UPDATE_CAMERA(renderer, cam, aspect)                          \
  ((renderer)->update_camera((renderer), (cam), (aspect)))
#define RENDERER_RENDER_CHUNK(renderer, chunk, view, proj)                     \
  ((renderer)->render_chunk((renderer), (chunk), (view), (proj)))
#define RENDERER_RENDER_SPRITE(renderer, pos, size, tex, rot)                  \
  ((renderer)->render_sprite((renderer), (pos), (size), (tex), (rot)))
#define RENDERER_DRAW_LINE(renderer, start, end, color)                        \
  ((renderer)->render_debug_line((renderer), (start), (end), (color)))
#define RENDERER_DRAW_BOX(renderer, center, size, rot, color)                  \
  ((renderer)->render_debug_box((renderer), (center), (size), (rot), (color)))
#define RENDERER_DRAW_SPHERE(renderer, center, radius, color)                  \
  ((renderer)->render_debug_sphere((renderer), (center), (radius), (color)))

#endif // RENDERER_H
