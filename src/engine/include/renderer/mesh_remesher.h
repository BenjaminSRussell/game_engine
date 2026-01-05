#ifndef MESH_REMESHER_H
#define MESH_REMESHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct Mesh Mesh;

// Remeshing method
typedef enum {
  REMESH_UNIFORM,           // Uniform triangle size
  REMESH_ADAPTIVE,          // Adaptive based on curvature
  REMESH_ISOTROPIC,         // Isotropic remeshing
  REMESH_FEATURE_PRESERVING // Preserve sharp features
} RemeshMethod;

// Mesh analysis results
typedef struct {
  bool has_non_manifold;
  bool has_holes;
  bool has_self_intersections;
  bool has_degenerate_triangles;
  bool has_flipped_normals;
  uint32_t non_manifold_edges;
  uint32_t hole_count;
  uint32_t degenerate_count;
  float min_triangle_quality;
  float avg_triangle_quality;
} MeshAnalysis;

// Remeshing options
typedef struct {
  RemeshMethod method;
  float target_edge_length; // Target edge length (0 = auto)
  uint32_t iterations;      // Number of remeshing iterations
  bool preserve_boundaries;
  bool preserve_sharp_edges;
  float sharp_edge_angle; // Angle threshold for sharp edges (degrees)
  bool fix_topology;      // Attempt to fix topology issues
  bool generate_uvs;      // Auto-generate UVs if missing
} RemeshOptions;

// UV unwrapping options
typedef enum {
  UV_UNWRAP_SMART,       // Smart projection
  UV_UNWRAP_CONFORMAL,   // Conformal (angle-preserving)
  UV_UNWRAP_EQUIAREAL,   // Equal-area
  UV_UNWRAP_CYLINDRICAL, // Cylindrical projection
  UV_UNWRAP_SPHERICAL    // Spherical projection
} UVUnwrapMethod;

typedef struct {
  UVUnwrapMethod method;
  float chart_padding;   // Padding between UV charts
  uint32_t texture_size; // Target texture resolution
  bool pack_charts;      // Pack UV charts efficiently
} UVUnwrapOptions;

// Public API - Analysis
bool mesh_analyze(const Mesh *mesh, MeshAnalysis *analysis);
bool mesh_validate_topology(const Mesh *mesh);
float mesh_calculate_triangle_quality(const Mesh *mesh,
                                      uint32_t triangle_index);
float mesh_calculate_avg_edge_length(const Mesh *mesh);

// Public API - Remeshing
bool mesh_remesh(Mesh *mesh, const RemeshOptions *options);
Mesh *mesh_create_remeshed(const Mesh *mesh, const RemeshOptions *options);

// Public API - Topology Repair
bool mesh_fix_non_manifold(Mesh *mesh);
bool mesh_fill_holes(Mesh *mesh);
bool mesh_fix_self_intersections(Mesh *mesh);
bool mesh_fix_flipped_normals(Mesh *mesh);
bool mesh_remove_duplicate_vertices(Mesh *mesh, float threshold);

// Public API - UV Operations
bool mesh_generate_uvs(Mesh *mesh, const UVUnwrapOptions *options);
bool mesh_pack_uvs(Mesh *mesh, float padding);
bool mesh_validate_uvs(const Mesh *mesh);

// Public API - Normal Operations
bool mesh_recalculate_normals(Mesh *mesh, float angle_threshold);
bool mesh_smooth_normals(Mesh *mesh, uint32_t iterations);
bool mesh_fix_normal_orientation(Mesh *mesh);

// Default options
RemeshOptions mesh_remesh_get_default_options(void);
UVUnwrapOptions mesh_uv_get_default_options(void);

#endif // MESH_REMESHER_H
