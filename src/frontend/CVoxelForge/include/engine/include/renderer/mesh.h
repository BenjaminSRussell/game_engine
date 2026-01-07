// include/renderer/mesh.h
//
// Purpose: Mesh data structure and rendering
//
#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <chunk/chunk.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Forward declaration
struct Material;
struct Chunk;
struct BlockRegistry;

typedef struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  u32 ao;
  u32 light;
  u32 texture_id;
  f32 wave_phase;
} Vertex;

// Forward declaration for Vulkan streaming (defined in vulkan_streaming.h)
#ifdef VULKAN_BUILD
#include <renderer/vulkan_streaming.h>
#endif

typedef struct Mesh {
  // Vertex data
  Vertex *vertices;
  u32 vertex_count;
  u32 vertex_capacity;

  // Index data
  u32 *indices;
  u32 index_count;
  u32 index_capacity;

  // OpenGL buffers
  u32 vao; // Vertex Array Object
  u32 vbo; // Vertex Buffer Object
  u32 ebo; // Element Buffer Object

  // Material
  struct Material *material;

  // State
  bool is_uploaded;

#ifdef VULKAN_BUILD
  // Vulkan streaming allocations
  MeshAllocation vertex_alloc;
  MeshAllocation index_alloc;
#endif
} Mesh;

// Mesh lifecycle
bool mesh_create(Mesh *mesh, Vertex *vertices, u32 vertex_count, u32 *indices,
                 u32 index_count);
void mesh_destroy(Mesh *mesh);

// Mesh operations
void mesh_upload_to_gpu(Mesh *mesh);
void mesh_render(const Mesh *mesh);

// Mesh generators
Mesh *mesh_create_cube(f32 size);
Mesh *mesh_create_plane(f32 width, f32 height);
Mesh *mesh_create_sphere(f32 radius, u32 segments);

// MeshQuality and MeshOptions - use include guards to avoid redefinition
#ifndef MESH_QUALITY_DEFINED
#define MESH_QUALITY_DEFINED
typedef enum {
  MESH_QUALITY_LOW,
  MESH_QUALITY_MEDIUM,
  MESH_QUALITY_HIGH,
  MESH_QUALITY_ULTRA
} MeshQuality;
#endif

#ifndef MESH_OPTIONS_DEFINED
#define MESH_OPTIONS_DEFINED
typedef struct {
  bool enable_caching;
  bool enable_cancellation;
  bool enable_compression;
  bool ambient_occlusion;
  bool smooth_lighting;
  bool greedy_meshing;
  bool face_culling;
  MeshQuality quality;
} MeshOptions;
#endif

void mesh_init(Mesh *mesh, u32 vertex_capacity, u32 index_capacity);
void mesh_free(Mesh *mesh);
void mesh_clear(Mesh *mesh);
bool mesh_should_render_face(struct Chunk *chunk, i32 x, i32 y, i32 z, u8 face,
                             const struct BlockRegistry *registry);

#endif // ENGINE_MESH_H
