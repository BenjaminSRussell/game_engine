#ifndef GEOMETRY_MESH_OPTIMIZE_H
#define GEOMETRY_MESH_OPTIMIZE_H

#include "geometry/geometry_types.h"

/**
 * Mesh Optimization Functions
 * 
 * These functions optimize mesh data for better performance:
 * - Vertex cache optimization
 * - Vertex deduplication
 * - Attribute packing/compression
 * - Normal calculation and optimization
 */

// ============================================================================
// VERTEX NORMAL CALCULATION
// ============================================================================

// Normal calculation options
typedef struct normal_calculation_options_t {
    bool smooth_normals;        // Generate smooth normals across triangle boundaries
    float angle_threshold;      // Angle threshold for smooth normals (degrees)
    bool preserve_existing;     // Keep existing normals where possible
    bool normalize_normals;     // Ensure all normals are unit length
    bool calculate_tangents;    // Also calculate tangents
} normal_calculation_options_t;

// Default options for normal calculation
extern const normal_calculation_options_t NORMAL_CALCULATION_DEFAULT;

// Calculate face normals (per-triangle)
void mesh_calculate_face_normals(mesh_t* mesh);

// Calculate smooth vertex normals with angle-based weighting
void mesh_calculate_smooth_normals(mesh_t* mesh, float angle_threshold);

// Calculate normals with advanced options
void mesh_calculate_normals_advanced(mesh_t* mesh, const normal_calculation_options_t* options);

// Optimize existing normals (normalize, smooth, etc.)
void mesh_optimize_normals(mesh_t* mesh, float angle_threshold);

// Calculate tangents from normals and UVs
void mesh_calculate_tangents(mesh_t* mesh);

// ============================================================================
// VERTEX DEDUPLICATION
// ============================================================================

// Merges identical vertices within epsilon tolerance
void mesh_deduplicate_vertices(mesh_t* mesh, float epsilon);

// Advanced vertex deduplication with position/normal/UV weighting
typedef struct vertex_dedup_options_t {
    float position_weight;     // Weight for position comparison
    float normal_weight;        // Weight for normal comparison
    float uv_weight;           // Weight for UV comparison
    float epsilon;             // Overall tolerance
    bool preserve_seams;       // Preserve UV seams
    bool preserve_sharp_edges;  // Preserve sharp normal edges
} vertex_dedup_options_t;

void mesh_deduplicate_vertices_advanced(mesh_t* mesh, const vertex_dedup_options_t* options);

// ============================================================================
// INDEX OPTIMIZATION
// ============================================================================

// Reorders indices for better GPU vertex cache performance
// Uses Forsyth/Tipsify algorithm
void mesh_optimize_indices(mesh_t* mesh);
void mesh_optimize_vertex_cache(mesh_t* mesh);

// Advanced vertex cache optimization with cache size parameter
void mesh_optimize_vertex_cache_advanced(mesh_t* mesh, uint32_t cache_size);

// ============================================================================
// ATTRIBUTE COMPRESSION
// ============================================================================

// Pack normals using octahedral encoding (8 or 16 bit)
void mesh_pack_normals_oct8(mesh_t* mesh);
void mesh_pack_normals_oct16(mesh_t* mesh);

// Quantize UVs to 16-bit integers
void mesh_quantize_uvs(mesh_t* mesh);

// Compress vertex positions to 16-bit integers with scale/offset
void mesh_quantize_positions(mesh_t* mesh);

// ============================================================================
// MESH VALIDATION AND REPAIR
// ============================================================================

// Validate mesh integrity
typedef struct mesh_validation_result_t {
    bool valid;
    uint32_t degenerate_triangles;
    uint32_t invalid_normals;
    uint32_t duplicate_vertices;
    uint32_t unused_vertices;
    bool has_nan_or_inf;
} mesh_validation_result_t;

mesh_validation_result_t mesh_validate(const mesh_t* mesh);

// Repair common mesh issues
void mesh_repair_degenerate_triangles(mesh_t* mesh);
void mesh_fix_invalid_normals(mesh_t* mesh);
void mesh_remove_unused_vertices(mesh_t* mesh);

// ============================================================================
// ADVANCED OPTIMIZATION
// ============================================================================

// Complete mesh optimization pipeline
typedef struct mesh_optimization_pipeline_t {
    bool calculate_normals;
    bool deduplicate_vertices;
    bool optimize_vertex_cache;
    bool quantize_attributes;
    bool repair_mesh;
    normal_calculation_options_t normal_options;
    vertex_dedup_options_t dedup_options;
    float vertex_cache_size;
} mesh_optimization_pipeline_t;

void mesh_optimize_complete(mesh_t* mesh, const mesh_optimization_pipeline_t* pipeline);

// Get default optimization pipeline
mesh_optimization_pipeline_t mesh_get_default_optimization_pipeline(void);

#endif // GEOMETRY_MESH_OPTIMIZE_H
