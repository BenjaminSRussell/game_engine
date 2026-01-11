#ifndef GEOMETRY_TYPES_H
#define GEOMETRY_TYPES_H

#include "../include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

// Forward declarations
struct material_t;

// ----------------------------------------------------------------------------
// Vertex Definitions
// ----------------------------------------------------------------------------

/**
 * Standard vertex structure for general purpose meshes
 *
 * Layout: Interleaved
 * Size: 12 + 12 + 8 + 16 = 48 bytes
 * Alignment: 16 bytes (simd friendly)
 */
typedef struct vertex_t {
  Vec3 position; // 12 bytes
  Vec3 normal;   // 12 bytes
  Vec2 uv;       // 8 bytes
  Vec4 tangent;  // 16 bytes (w component stores handedness for bitangent)
} vertex_t;

/**
 * Skinned vertex structure for animated meshes
 * Adds bone weights and indices
 */
typedef struct vertex_skinned_t {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  Vec4 tangent;
  Vec4 weights;     // Bone weights
  u32 indices[4];   // Bone indices
} vertex_skinned_t; // Size: 48 + 16 + 16 = 80 bytes

// ----------------------------------------------------------------------------
// Mesh Bounds
// ----------------------------------------------------------------------------

typedef struct mesh_bounds_t {
  Vec3 min; // AABB min
  Vec3 max; // AABB max
  Vec3 sphere_center;
  f32 sphere_radius;
} mesh_bounds_t;

// ----------------------------------------------------------------------------
// Submesh
// ----------------------------------------------------------------------------

/**
 * A submesh represents a subset of the mesh that uses a specific material.
 * This maps to a draw call.
 */
typedef struct submesh_t {
  u32 index_start;    // Starting index in the global index buffer
  u32 index_count;    // Number of indices to draw
  u32 vertex_start;   // Base vertex offset (for relative indexing)
  u32 vertex_count;   // Number of vertices used by this submesh
  u32 material_index; // Index into the mesh material table
  mesh_bounds_t bounds;
} submesh_t;

// ----------------------------------------------------------------------------
// LODs
// ----------------------------------------------------------------------------

typedef struct mesh_lod_t {
  u32 index_start;
  u32 index_count;
  f32 screen_size_threshold; // Switching threshold (0.0-1.0)
} mesh_lod_t;

// ----------------------------------------------------------------------------
// Mesh Flags
// ----------------------------------------------------------------------------

typedef enum mesh_flags_e {
  MESH_FLAG_NONE = 0,
  MESH_FLAG_DYNAMIC = 1 << 0,     // CPU-side updates frequent
  MESH_FLAG_SKINNED = 1 << 1,     // Has bone data
  MESH_FLAG_KEEP_CPU = 1 << 2,    // Keep CPU copy after upload
  MESH_FLAG_TRANSPARENT = 1 << 3, // Hint for sorting
  MESH_FLAG_SHADOW_CASTER = 1 << 4,
  MESH_FLAG_SHADOW_RECEIVER = 1 << 5,
} mesh_flags_e;

// ----------------------------------------------------------------------------
// Mesh
// ----------------------------------------------------------------------------

#define MESH_MAX_LODS 8
#define MESH_MAX_MATERIALS 16

typedef struct mesh_t {
  char name[64];
  u32 id;    // Unique mesh ID
  u32 flags; // mesh_flags_e combination

  // CPU Data
  vertex_t *vertices;
  u32 vertex_count;
  u32 vertex_capacity;

  u32 *indices;
  u32 index_count;
  u32 index_capacity;

  // Submeshes
  submesh_t *submeshes;
  u32 submesh_count;

  // LODs
  mesh_lod_t lods[MESH_MAX_LODS];
  u32 lod_count;

  // Materials (Referenced by ID/Handle)
  // Actual material pointers usually resolved at render time or scene graph
  u32 material_ids[MESH_MAX_MATERIALS];
  u32 material_count;

  mesh_bounds_t bounds;

  // GPU Handles (Platform agnostic ID)
  u32 vertex_buffer_handle;
  u32 index_buffer_handle;

  // Blend Shape Data
  blend_shape_t* blend_shapes;
  u32 blend_shape_count;
  vertex_t* base_vertices;  // Original vertices for blend shape evaluation

  // Skeletal Animation Data
  skeleton_t* skeleton;
  vertex_skinned_t* skinned_vertices;  // Skinned vertex data

  // Internal state
  u32 ref_count;
  u64 last_accessed_frame;

} mesh_t;

// ----------------------------------------------------------------------------
// Blend Shapes
// ----------------------------------------------------------------------------

#define MAX_BLEND_SHAPES 32
#define MAX_BLEND_SHAPE_NAME 64

typedef struct blend_shape_t {
  char name[MAX_BLEND_SHAPE_NAME];
  u32 vertex_count;
  Vec3* delta_positions;  // Position deltas from base mesh
  Vec3* delta_normals;    // Normal deltas from base mesh
  Vec3* delta_tangents;   // Tangent deltas from base mesh (optional)
  f32 weight;             // Current weight (0.0 - 1.0)
} blend_shape_t;

// ----------------------------------------------------------------------------
// Skeletal Animation
// ----------------------------------------------------------------------------

#define MAX_BONES_PER_VERTEX 4
#define MAX_BONES 256

typedef struct bone_weight_t {
  u32 bone_index;
  f32 weight;
} bone_weight_t;

typedef struct skeleton_t {
  char name[64];
  u32 bone_count;
  // TODO: Add bone hierarchy data when skeleton system is implemented
} skeleton_t;

#endif // GEOMETRY_TYPES_H
