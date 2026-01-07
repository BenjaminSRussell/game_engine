// include/tools/asset_compiler/mesh_optimizer.h
//
// Purpose: Asset compiler mesh optimization tools for preprocessing meshes to improve
// GPU efficiency through vertex cache optimization, overdraw reduction, quantization,
// simplification, and stripification.
//
// Public APIs:
// - `MeshOptimizer`: Main mesh optimization context
// - `OptimizationSettings`: Configuration for optimization algorithms
// - `mesh_opt_create`: Create mesh optimizer instance
// - `mesh_opt_destroy`: Destroy mesh optimizer instance
// - `mesh_opt_optimize_vertex_cache`: Optimize vertex ordering for GPU cache
// - `mesh_opt_reduce_overdraw`: Reorder triangles to reduce overdraw
// - `mesh_opt_quantize_vertices`: Quantize vertex positions for memory savings
// - `mesh_opt_simplify_mesh`: Generate simplified LOD meshes
// - `mesh_opt_stripify`: Convert triangle lists to triangle strips
// - `mesh_opt_benchmark`: Run performance benchmarks
//
// Ownership: MeshOptimizer owns its internal data and must be freed with mesh_opt_destroy.
//
// Invariants:
// - All optimization functions work in-place on mesh data
// - Optimization settings must be valid before use
// - Benchmark results are valid until next benchmark run

#ifndef MESH_OPTIMIZER_ASSET_COMPILER_H
#define MESH_OPTIMIZER_ASSET_COMPILER_H

#include "include/common.h"
#include <math/vec3.h>
#include <math/vec2.h>
#include <include/rendering/mesh.h>

// ============================================================================
// Optimization Configuration
// ============================================================================

typedef enum {
    VERTEX_CACHE_OPTIMIZATION_TIPSY = 0,  // Tom Forsyth's algorithm
    VERTEX_CACHE_OPTIMIZATION_LFORSYTH,   // Linear-speed algorithm
    VERTEX_CACHE_OPTIMIZATION_KCACHE      // k-cache optimizer
} VertexCacheAlgorithm;

typedef enum {
    SIMPLIFICATION_PRESERVE_NORMALS = 1 << 0,
    SIMPLIFICATION_PRESERVE_UVS = 1 << 1,
    SIMPLIFICATION_PRESERVE_BOUNDARIES = 1 << 2,
    SIMPLIFICATION_AGGRESSIVE = 1 << 3
} SimplificationFlags;

typedef enum {
    QUANTIZATION_POSITION_16BIT = 0,
    QUANTIZATION_POSITION_12BIT,
    QUANTIZATION_POSITION_8BIT,
    QUANTIZATION_NORMAL_16BIT,
    QUANTIZATION_NORMAL_8BIT,
    QUANTIZATION_UV_16BIT,
    QUANTIZATION_UV_12BIT
} QuantizationType;

typedef struct {
    // Vertex cache optimization settings
    VertexCacheAlgorithm cache_algorithm;
    u32 cache_size;              // GPU vertex cache size (typically 16-32)
    
    // Overdraw reduction settings
    bool enable_overdraw_reduction;
    f32 overdraw_threshold;      // Threshold for overdraw detection
    
    // Vertex quantization settings
    bool enable_quantization;
    QuantizationType quant_types[8];
    u32 quant_type_count;
    
    // Mesh simplification settings
    bool enable_simplification;
    SimplificationFlags simplify_flags;
    f32 simplification_ratio;   // Target vertex count ratio (0.1-1.0)
    u32 max_error;              // Maximum simplification error
    
    // Stripification settings
    bool enable_stripification;
    u32 min_strip_length;        // Minimum strip length to keep
    
    // General settings
    bool validate_results;       // Validate optimization results
    bool generate_stats;         // Generate optimization statistics
} OptimizationSettings;

// ============================================================================
// Optimization Statistics
// ============================================================================

typedef struct {
    // Original mesh stats
    u32 original_vertex_count;
    u32 original_index_count;
    u32 original_triangle_count;
    f32 original_memory_mb;
    
    // Optimized mesh stats
    u32 optimized_vertex_count;
    u32 optimized_index_count;
    u32 optimized_triangle_count;
    f32 optimized_memory_mb;
    
    // Performance metrics
    f32 acmr_before;             // Average Cache Miss Ratio before
    f32 acmr_after;              // Average Cache Miss Ratio after
    f32 overdraw_before;         // Estimated overdraw before
    f32 overdraw_after;          // Estimated overdraw after
    
    // Compression ratios
    f32 vertex_compression_ratio;
    f32 index_compression_ratio;
    f32 overall_compression_ratio;
    
    // Timing information
    f64 optimization_time_ms;   // Total optimization time
    f64 cache_opt_time_ms;       // Vertex cache optimization time
    f64 overdraw_opt_time_ms;    // Overdraw reduction time
    f64 quantization_time_ms;    // Quantization time
    f64 simplification_time_ms;   // Simplification time
    f64 stripification_time_ms;  // Stripification time
} OptimizationStats;

// ============================================================================
// Main Mesh Optimizer
// ============================================================================

typedef struct MeshOptimizer MeshOptimizer;

// Create mesh optimizer instance
MeshOptimizer* mesh_opt_create(const OptimizationSettings* settings);

// Destroy mesh optimizer instance
void mesh_opt_destroy(MeshOptimizer* optimizer);

// Get current optimization settings
const OptimizationSettings* mesh_opt_get_settings(const MeshOptimizer* optimizer);

// Update optimization settings
void mesh_opt_set_settings(MeshOptimizer* optimizer, const OptimizationSettings* settings);

// Get last optimization statistics
const OptimizationStats* mesh_opt_get_stats(const MeshOptimizer* optimizer);

// ============================================================================
// Optimization Functions
// ============================================================================

// Optimize vertex cache for GPU efficiency
bool mesh_opt_optimize_vertex_cache(MeshOptimizer* optimizer, Mesh* mesh);

// Reduce overdraw by reordering triangles
bool mesh_opt_reduce_overdraw(MeshOptimizer* optimizer, Mesh* mesh);

// Quantize vertex positions and attributes for memory savings
bool mesh_opt_quantize_vertices(MeshOptimizer* optimizer, Mesh* mesh);

// Simplify mesh for LOD generation
bool mesh_opt_simplify_mesh(MeshOptimizer* optimizer, Mesh* mesh, f32 target_ratio);

// Convert triangle lists to triangle strips
bool mesh_opt_stripify(MeshOptimizer* optimizer, Mesh* mesh);

// Apply all optimizations in sequence
bool mesh_opt_optimize_full(MeshOptimizer* optimizer, Mesh* mesh);

// ============================================================================
// LOD Generation
// ============================================================================

typedef struct {
    Mesh lod_mesh;
    f32 distance_threshold;
    f32 screen_size_threshold;
    u32 lod_level;
} LODMesh;

typedef struct {
    LODMesh* lods;
    u32 lod_count;
    Vec3 center_point;
} LODSet;

// Generate LOD levels from source mesh
bool mesh_opt_generate_lods(MeshOptimizer* optimizer, const Mesh* source_mesh,
                            LODSet* lod_set, u32 lod_count);

// Free LOD set resources
void mesh_opt_free_lods(LODSet* lod_set);

// ============================================================================
// Batch Processing
// ============================================================================

typedef struct {
    Mesh** meshes;
    u32 mesh_count;
    char** mesh_names;
} MeshBatch;

// Optimize multiple meshes in batch
bool mesh_opt_optimize_batch(MeshOptimizer* optimizer, MeshBatch* batch);

// Merge compatible meshes for batch rendering
bool mesh_opt_merge_batch(MeshOptimizer* optimizer, const MeshBatch* batch, Mesh* merged);

// ============================================================================
// Benchmarking
// ============================================================================

typedef struct {
    char algorithm_name[64];
    f64 average_time_ms;
    f64 min_time_ms;
    f64 max_time_ms;
    u32 iterations;
    f32 avg_acmr_improvement;
    f32 avg_memory_savings;
} BenchmarkResult;

typedef struct {
    BenchmarkResult* results;
    u32 result_count;
    f64 total_benchmark_time_ms;
} BenchmarkReport;

// Run comprehensive benchmarks
bool mesh_opt_benchmark(MeshOptimizer* optimizer, const Mesh* test_mesh, 
                        u32 iterations, BenchmarkReport* report);

// Run specific algorithm benchmark
bool mesh_opt_benchmark_algorithm(MeshOptimizer* optimizer, const Mesh* test_mesh,
                                  const char* algorithm_name, u32 iterations,
                                  BenchmarkResult* result);

// Free benchmark report
void mesh_opt_free_benchmark_report(BenchmarkReport* report);

// Print benchmark report to console
void mesh_opt_print_benchmark_report(const BenchmarkReport* report);

// ============================================================================
// Utility Functions
// ============================================================================

// Create default optimization settings
OptimizationSettings mesh_opt_default_settings(void);

// Create high-quality optimization settings
OptimizationSettings mesh_opt_high_quality_settings(void);

// Create fast optimization settings
OptimizationSettings mesh_opt_fast_settings(void);

// Validate mesh data before optimization
bool mesh_opt_validate_mesh(const Mesh* mesh);

// Calculate mesh quality metrics
void mesh_opt_calculate_quality_metrics(const Mesh* mesh, f32* acmr, f32* overdraw);

// Estimate memory usage of mesh
f32 mesh_opt_estimate_memory_usage(const Mesh* mesh);

// Export optimization report to file
bool mesh_opt_export_report(const MeshOptimizer* optimizer, const char* filename);

#endif // MESH_OPTIMIZER_ASSET_COMPILER_H
