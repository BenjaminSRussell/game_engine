/*
 * lod_generator.h
 * Automatic LOD mesh generation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_GENERATOR_H
#define GEOMETRY_LOD_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/geometry_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Forward declaration for GPU mesh data (placeholder - adapt to actual GPU backend)
typedef struct mesh_gpu_data {
    void* vertex_buffer;
    void* index_buffer;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t vertex_stride;
    uint32_t index_size;
} mesh_gpu_data_t;

// LOD Generation Configuration
typedef struct lod_generation_config {
    float reduction_ratios[4];    // Target reduction for LODs 1-4 (e.g., 0.75, 0.5, 0.25, 0.1)
    float quality_weights[3];     // Weights for position, normal, UV error [0-1]
    bool preserve_boundaries;      // Preserve mesh boundaries
    bool preserve_seams;          // Preserve UV/Material seams
    bool preserve_features;       // Preserve sharp features
    float feature_angle;          // Angle threshold for sharp features (degrees)
    float weld_distance;          // Distance to weld vertices
    float normal_smoothing;       // Smoothing factor for normals [0-1]
} lod_generation_config_t;

// LOD Generation Statistics
typedef struct lod_stats {
    uint32_t original_triangles;
    uint32_t lod_triangles[8];
    float geometric_error[8];     // Max geometric error per LOD
    double generation_time_ms;
    uint32_t collapsed_edges;
} lod_stats_t;

// LOD chain structure
typedef struct lod_chain {
    mesh_t* lods[8];                  // Mesh data for each LOD
    float switch_distances[8];        // Screen coverage thresholds
    float geometric_errors[8];        // Geometric error for each LOD
    uint32_t lod_count;               // Number of LODs in chain
    lod_generation_config_t config;   // Config used to generate
} lod_chain_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* LOD Generation */
// Generate LOD chain from source mesh with default config
lod_chain_t* lod_generate_chain(const mesh_t* source, const float* ratios, uint32_t count);

// Generate LOD chain with advanced configuration
lod_chain_t* lod_generate_chain_advanced(const mesh_t* source, 
                                        const lod_generation_config_t* config);

// Get default configuration
lod_generation_config_t lod_get_default_config(void);

// Destroy LOD chain and free resources
void lod_chain_destroy(lod_chain_t* chain);

/* Mesh Simplification */
// Simplify mesh using Quadric Error Metrics
mesh_t* qem_simplify(const mesh_t* source, float target_ratio);

// Simplify with advanced options
mesh_t* qem_simplify_advanced(const mesh_t* source, 
                                  float target_ratio,
                                  const lod_generation_config_t* config,
                                  float* out_error);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_GENERATOR_H */
