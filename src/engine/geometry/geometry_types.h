#ifndef GEOMETRY_TYPES_H
#define GEOMETRY_TYPES_H

#include <common.h>
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
    Vec3 position;    // 12 bytes
    Vec3 normal;      // 12 bytes
    Vec2 uv;          // 8 bytes
    Vec4 tangent;     // 16 bytes (w component stores handedness for bitangent)
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
    u32    indices[4];  // Bone indices
} vertex_skinned_t; // Size: 48 + 16 + 16 = 80 bytes

// ----------------------------------------------------------------------------
// Mesh Bounds
// ----------------------------------------------------------------------------

typedef struct mesh_bounds_t {
    Vec3 min;         // AABB min
    Vec3 max;         // AABB max
    Vec3 sphere_center;
    f32    sphere_radius;
} mesh_bounds_t;

/**
 * Oriented Bounding Box (OBB)
 * More accurate than AABB for rotated objects
 */
typedef struct mesh_obb_t {
    Vec3 center;
    Vec3 extents;     // Half-sizes along each axis
    Vec3 axes[3];     // Orientation axes (orthonormal basis)
} mesh_obb_t;

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
// Mesh Statistics
// ----------------------------------------------------------------------------

typedef struct mesh_stats_t {
    u32 triangle_count;
    u32 vertex_count;
    u32 index_count;
    f32 surface_area;
    f32 volume;
    u64 cpu_memory_bytes;
    u64 gpu_memory_bytes;
} mesh_stats_t;

// ----------------------------------------------------------------------------
// Blend Shapes / Morph Targets
// ----------------------------------------------------------------------------

#define MESH_MAX_BLEND_SHAPES 32

/**
 * Blend shape stores per-vertex deltas for morphing
 */
typedef struct blend_shape_t {
    char name[64];
    Vec3* position_deltas;  // Per-vertex position offsets
    Vec3* normal_deltas;    // Per-vertex normal offsets (optional)
    u32 vertex_count;
    f32 weight;             // Current blend weight [0, 1]
} blend_shape_t;

typedef struct morph_target_t {
    blend_shape_t shapes[MESH_MAX_BLEND_SHAPES];
    u32 shape_count;
} morph_target_t;

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
    MESH_FLAG_INSTANCED = 1 << 6,   // Supports GPU instancing
    MESH_FLAG_HAS_BLEND_SHAPES = 1 << 7,
    MESH_FLAG_GPU_UPLOADED = 1 << 8, // Currently on GPU
} mesh_flags_e;

// ----------------------------------------------------------------------------
// Mesh Validation
// ----------------------------------------------------------------------------

typedef enum mesh_validation_error_e {
    MESH_VALIDATION_OK = 0,
    MESH_VALIDATION_NULL_PTR,
    MESH_VALIDATION_NO_VERTICES,
    MESH_VALIDATION_NO_INDICES,
    MESH_VALIDATION_INVALID_INDEX,
    MESH_VALIDATION_DEGENERATE_TRIANGLE,
    MESH_VALIDATION_INVALID_NORMAL,
    MESH_VALIDATION_INVALID_UV,
    MESH_VALIDATION_BUFFER_OVERFLOW,
} mesh_validation_error_e;

// ----------------------------------------------------------------------------
// Mesh
// ----------------------------------------------------------------------------

#define MESH_MAX_LODS 8
#define MESH_MAX_MATERIALS 16

typedef struct mesh_t {
    char name[64];
    u32 id;                     // Unique mesh ID
    u32 flags;                  // mesh_flags_e combination

    // CPU Data
    vertex_t* vertices;
    u32 vertex_count;
    u32 vertex_capacity;

    u32* indices;
    u32 index_count;
    u32 index_capacity;

    // Submeshes
    submesh_t* submeshes;
    u32 submesh_count;

    // LODs
    mesh_lod_t lods[MESH_MAX_LODS];
    u32 lod_count;

    // Materials (Referenced by ID/Handle)
    // Actual material pointers usually resolved at render time or scene graph
    u32 material_ids[MESH_MAX_MATERIALS];
    u32 material_count;

    mesh_bounds_t bounds;
    mesh_obb_t obb;
    mesh_stats_t stats;

    // GPU Handles (Platform agnostic ID)
    u32 vertex_buffer_handle;
    u32 index_buffer_handle;
    
    // Deformation
    morph_target_t* morph_targets;
    
    // Internal state
    u32 ref_count;
    u64 last_accessed_frame;
    bool bounds_dirty;
    bool stats_dirty;

} mesh_t;

#endif // GEOMETRY_TYPES_H
