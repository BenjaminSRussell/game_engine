// include/mesh/mesh.h
//
// Purpose: Game-specific mesh data structures for voxel rendering
//
#ifndef GAME_MESH_H
#define GAME_MESH_H

#include "../game_common.h"
#include <math/vec2.h>
#include <math/vec3.h>

// Vertex structure for voxel mesh generation
typedef struct ChunkVertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  u8 ao;          // Ambient occlusion
  u8 light;       // Light level
  u8 texture_id;  // Texture atlas ID
  f32 wave_phase; // Wave animation phase (for water)
} ChunkVertex;

// Mesh structure for chunk rendering
typedef struct ChunkMesh {
  ChunkVertex *vertices;
  u32 vertex_count;
  u32 vertex_capacity;

  u32 *indices;
  u32 index_count;
  u32 index_capacity;

  bool dirty;
} ChunkMesh;

// Mesh quality levels
#ifndef MESH_QUALITY_DEFINED
#define MESH_QUALITY_DEFINED
typedef enum {
  MESH_QUALITY_LOW,
  MESH_QUALITY_MEDIUM,
  MESH_QUALITY_HIGH,
  MESH_QUALITY_ULTRA
} MeshQuality;
#endif

// Mesh generation options
#ifndef MESH_OPTIONS_DEFINED
#define MESH_OPTIONS_DEFINED
typedef struct {
  MeshQuality quality;
  bool greedy_meshing;
  bool ambient_occlusion;
  bool smooth_lighting;
  bool face_culling;
  bool enable_caching;
  bool enable_compression;
  bool enable_cancellation;
} MeshOptions;
#endif

// Mesh lifecycle
// Mesh lifecycle
void chunk_mesh_init(ChunkMesh *mesh, u32 vertex_capacity, u32 index_capacity);
void chunk_mesh_free(ChunkMesh *mesh);
void chunk_mesh_clear(ChunkMesh *mesh);

// Mesh generation
void chunk_mesh_generate_chunk(ChunkMesh *mesh, struct Chunk *chunk,
                               const struct BlockRegistry *registry,
                               MeshOptions options);

// Mesh validation
bool mesh_should_render_face(struct Chunk *chunk, i32 x, i32 y, i32 z,
                             u8 face_mask,
                             const struct BlockRegistry *registry);

#endif // GAME_MESH_H
