#ifndef LOD_GENERATOR_H
#define LOD_GENERATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct Mesh Mesh;
typedef struct Model Model;

// LOD generation method
typedef enum {
  LOD_METHOD_QUADRIC,       // Quadric error metrics (highest quality)
  LOD_METHOD_EDGE_COLLAPSE, // Edge collapse (balanced)
  LOD_METHOD_CLUSTER,       // Cluster-based (fastest)
  LOD_METHOD_SCREEN_SPACE   // Screen-space error driven
} LODMethod;

// LOD level configuration
typedef struct {
  float distance;        // Distance from camera
  float target_ratio;    // Vertex count ratio (0.0-1.0)
  uint32_t target_count; // Absolute vertex count (0 = use ratio)
  float max_error;       // Maximum allowed error
} LODLevelConfig;

// LOD generation options
typedef struct {
  LODMethod method;
  uint32_t level_count;   // Number of LOD levels to generate
  LODLevelConfig *levels; // Per-level configuration (NULL = auto)
  bool preserve_borders;
  bool preserve_uvs;
  bool preserve_normals;
  float screen_size; // Reference screen size for error calculation
} LODGenOptions;

// LOD chain structure
typedef struct {
  Mesh **levels; // Array of LOD meshes
  uint32_t level_count;
  float *distances; // Switch distances for each level
  float *errors;    // Error metrics for each level
} LODChain;

// Public API - Generation
LODChain *lod_generate_chain(const Mesh *mesh, const LODGenOptions *options);
bool lod_generate_for_model(Model *model, const LODGenOptions *options);
void lod_chain_free(LODChain *chain);

// Public API - Utilities
uint32_t lod_select_level(const LODChain *chain, float distance);
float lod_calculate_screen_error(const Mesh *mesh, float distance,
                                 float screen_size);
bool lod_validate_chain(const LODChain *chain);

// Public API - Auto-configuration
LODGenOptions lod_get_default_options(void);
LODGenOptions lod_get_options_for_quality(
    const char *quality); // "low", "medium", "high", "ultra"
void lod_auto_configure_distances(LODChain *chain, float base_distance);

// Public API - Analysis
void lod_print_stats(const LODChain *chain);
float lod_get_total_memory_size(const LODChain *chain);
float lod_get_reduction_ratio(const LODChain *chain, uint32_t level);

#endif // LOD_GENERATOR_H
