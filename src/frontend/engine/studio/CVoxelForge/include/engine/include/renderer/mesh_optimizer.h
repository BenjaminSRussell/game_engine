// include/rendering/mesh_optimizer.h
//
// Purpose: Provides comprehensive mesh optimization capabilities including LOD
// (Level of Detail) generation, mesh compression, culling systems, and vertex
// format optimization. This module significantly reduces memory usage and
// improves rendering performance through intelligent mesh processing and
// GPU-friendly data structures.
//
// Public APIs:
// - `MeshLOD`: Structure representing a single LOD level with distance
// threshold
// - `MeshLODSet`: Container for multiple LOD levels of the same mesh
// - `CullingResult`: Results from frustum/occlusion culling operations
// - `CompressedMesh`: Mesh with compressed vertex data
// - `mesh_lod_generate`: Generate LOD levels from a source mesh
// - `mesh_compress`: Compress mesh vertex data
// - `mesh_decompress`: Decompress mesh for rendering
// - `mesh_frustum_cull`: Perform frustum culling on meshes
// - `mesh_occlusion_cull`: Perform occlusion culling
// - `mesh_optimize_vertex_cache`: Optimize vertex ordering for GPU cache
//
// Ownership: LOD sets and compressed meshes own their internal data and must be
// freed with appropriate cleanup functions.
//
// Invariants:
// - LOD levels must be ordered from highest to lowest detail
// - Compressed meshes must be decompressed before rendering
// - Culling results are valid only for the frame they were generated
//

#ifndef MESH_OPTIMIZER_H
#define MESH_OPTIMIZER_H

#include <common.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <renderer/mesh.h>

// Forward declarations
struct Camera;
typedef struct Camera Camera;

// ============================================================================
// LOD (Level of Detail) System
// ============================================================================

typedef enum {
  LOD_LEVEL_HIGH = 0,   // Full detail (0-32 units)
  LOD_LEVEL_MEDIUM = 1, // Medium detail (32-64 units)
  LOD_LEVEL_LOW = 2,    // Low detail (64+ units)
  LOD_LEVEL_COUNT = 3
} LODLevel;

typedef struct {
  Mesh mesh;
  f32 distance_threshold;   // Distance at which this LOD becomes active
  f32 simplification_ratio; // 1.0 = full detail, 0.5 = half vertices, etc.
} MeshLOD;

typedef struct {
  MeshLOD levels[LOD_LEVEL_COUNT];
  u32 level_count;
  Vec3 center; // Center point for distance calculation
} MeshLODSet;

// Generate LOD levels from a source mesh
void mesh_lod_generate(MeshLODSet *lod_set, const Mesh *source_mesh,
                       Vec3 center);

// Get appropriate LOD level based on camera distance
LODLevel mesh_lod_select(const MeshLODSet *lod_set, Vec3 camera_pos);

// Get mesh for specific LOD level
Mesh *mesh_lod_get(MeshLODSet *lod_set, LODLevel level);

// Free LOD set resources
void mesh_lod_free(MeshLODSet *lod_set);

// ============================================================================
// Mesh Compression
// ============================================================================

typedef enum {
  VERTEX_COMPRESSION_NONE = 0,
  VERTEX_COMPRESSION_HALF_FLOAT, // 16-bit floats for positions/normals
  VERTEX_COMPRESSION_QUANTIZED,  // Quantized to 16-bit integers
  VERTEX_COMPRESSION_OCTAHEDRAL  // Octahedral encoding for normals
} VertexCompressionType;

typedef struct {
  u8 *compressed_data;
  u32 compressed_size;
  u32 original_vertex_count;
  u32 original_index_count;
  VertexCompressionType compression_type;
  Vec3 bounds_min; // For decompression
  Vec3 bounds_max;
} CompressedMesh;

// Compress mesh data
bool mesh_compress(CompressedMesh *out, const Mesh *source,
                   VertexCompressionType type);

// Decompress mesh data
bool mesh_decompress(Mesh *out, const CompressedMesh *compressed);

// Get compression ratio
f32 mesh_get_compression_ratio(const CompressedMesh *compressed);

// Free compressed mesh
void mesh_compressed_free(CompressedMesh *compressed);

// ============================================================================
// Culling Systems
// ============================================================================

#include <math/vec4.h>

typedef struct {
  Vec4 planes[6]; // Frustum planes (left, right, bottom, top, near, far)
} Frustum;

typedef struct {
  bool *visible;     // Array of visibility flags (one per mesh)
  u32 visible_count; // Number of visible meshes
  u32 total_count;   // Total number of meshes tested
} CullingResult;

// Extract frustum from view-projection matrix
void frustum_from_matrix(Frustum *frustum, const Mat4 *view_proj);

// Test if AABB is inside frustum
bool frustum_test_aabb(const Frustum *frustum, Vec3 min, Vec3 max);

// Test if sphere is inside frustum
bool frustum_test_sphere(const Frustum *frustum, Vec3 center, f32 radius);

// Perform frustum culling on array of meshes
void mesh_frustum_cull(CullingResult *result, const Mesh *meshes,
                       u32 mesh_count, const Frustum *frustum,
                       const Vec3 *mesh_positions);

// Perform occlusion culling (simple distance-based for now)
void mesh_occlusion_cull(CullingResult *result, const Mesh *meshes,
                         u32 mesh_count, const Vec3 *mesh_positions,
                         Vec3 camera_pos, f32 max_distance);

// Free culling result
void culling_result_free(CullingResult *result);

// ============================================================================
// Vertex Cache Optimization
// ============================================================================

// Optimize vertex order for GPU post-transform cache
void mesh_optimize_vertex_cache(Mesh *mesh);

// Optimize vertex order for pre-transform cache (vertex fetch)
void mesh_optimize_vertex_fetch(Mesh *mesh);

// Calculate ACMR (Average Cache Miss Ratio)
f32 mesh_calculate_acmr(const Mesh *mesh, u32 cache_size);

// ============================================================================
// Mesh Statistics
// ============================================================================

typedef struct {
  u32 vertex_count;
  u32 index_count;
  u32 triangle_count;
  f32 memory_usage_mb;
  f32 acmr; // Average Cache Miss Ratio
  Vec3 bounds_min;
  Vec3 bounds_max;
  Vec3 center;
  f32 radius;
} MeshStats;

// Calculate mesh statistics
void mesh_calculate_stats(MeshStats *stats, const Mesh *mesh);

// Print mesh statistics to console
void mesh_print_stats(const MeshStats *stats, const char *mesh_name);

// ============================================================================
// Batch Optimization
// ============================================================================

typedef struct {
  Mesh **meshes;
  u32 mesh_count;
  u32 total_vertex_count;
  u32 total_index_count;
} MeshBatch;

// Create mesh batch from array of meshes
void mesh_batch_create(MeshBatch *batch, Mesh **meshes, u32 count);

// Merge batch into single mesh (for static geometry)
bool mesh_batch_merge(Mesh *out, const MeshBatch *batch);

// Free mesh batch
void mesh_batch_free(MeshBatch *batch);

#endif // MESH_OPTIMIZER_H
