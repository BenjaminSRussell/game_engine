#ifndef GEOMETRY_MESH_OPTIMIZE_H
#define GEOMETRY_MESH_OPTIMIZE_H

#include "geometry_types.h"

/**
 * Mesh Optimization Functions
 * 
 * These functions optimize mesh data for better performance:
 * - Vertex cache optimization
 * - Vertex deduplication
 * - Attribute packing/compression
 */

// Vertex Deduplication
// Merges identical vertices within epsilon tolerance
void mesh_deduplicate_vertices(mesh_t* mesh, f32 epsilon);

// Index Optimization
// Reorders indices for better GPU vertex cache performance
// Uses Forsyth/Tipsify algorithm
void mesh_optimize_indices(mesh_t* mesh);
void mesh_optimize_vertex_cache(mesh_t* mesh);

// Attribute Compression Helpers
// Pack normals using oct ahedral encoding (8 or 16 bit)
void mesh_pack_normals_oct8(mesh_t* mesh);
void mesh_pack_normals_oct16(mesh_t* mesh);

// Quantize UVs to 16-bit integers
void mesh_quantize_uvs(mesh_t* mesh);

#endif // GEOMETRY_MESH_OPTIMIZE_H
