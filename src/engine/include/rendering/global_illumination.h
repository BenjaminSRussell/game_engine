// include/render/global_illumination.h
//
// Purpose: Global illumination system using path tracing and photon mapping.
// Provides realistic indirect lighting, light transport, and color bleeding.
//
// Public APIs:
// - `GIConfig`: Configuration for global illumination
// - `gi_path_trace`: Main path tracing function
// - `gi_compute_irradiance`: Cache irradiance for fast lookup
// - `gi_photon_map_init`: Initialize photon mapping data structure
// - `gi_trace_photons`: Emit and trace photons
// - `gi_estimate_irradiance`: Estimate irradiance at position
//

#ifndef GLOBAL_ILLUMINATION_H
#define GLOBAL_ILLUMINATION_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/vec4.h>

// ============================================================================
// Configuration
// ============================================================================

// Global illumination quality levels
typedef enum {
    GI_QUALITY_VERY_FAST = 1,   // 2-4 bounces, sparse sampling
    GI_QUALITY_FAST = 2,         // 4-6 bounces, moderate sampling
    GI_QUALITY_MEDIUM = 3,       // 6-8 bounces, good sampling
    GI_QUALITY_HIGH = 4,         // 8+ bounces, dense sampling
    GI_QUALITY_VERY_HIGH = 5,    // 16+ bounces, very dense sampling
} GIQuality;

// GI algorithm selector
typedef enum {
    GI_METHOD_PATH_TRACING,      // Unbiased but noisy
    GI_METHOD_PHOTON_MAPPING,    // Biased but fast
    GI_METHOD_IRRADIANCE_CACHE,  // Very fast with caching
    GI_METHOD_LIGHT_PROBES,      // Real-time with probes
} GIMethod;

// Path tracing configuration
typedef struct {
    u32 max_bounces;            // Maximum ray bounces (4-16)
    u32 samples_per_pixel;      // Samples for convergence (1-256)
    f32 importance_threshold;   // Minimum ray contribution before stopping
    bool use_russian_roulette;  // Use probabilistic termination
    bool use_next_event_estimation;  // Direct light sampling
    bool use_multiple_importance_sampling;  // MIS for better convergence
} PathTracingConfig;

// Photon mapping configuration
typedef struct {
    u32 num_photons;            // Number of photons to trace (10k-1M)
    u32 photon_bounces;         // Max bounces per photon
    u32 gather_samples;         // Photons to gather per lookup
    f32 gather_radius;          // Radius for photon gathering
    bool use_caustics;          // Include caustic photons
    bool use_volumes;           // Include volumetric effects
} PhotonMappingConfig;

// Irradiance cache configuration
typedef struct {
    u32 cache_size;             // Max cached irradiance values
    f32 interpolation_radius;   // Radius for interpolation
    f32 min_spacing;            // Minimum distance between samples
    bool use_octree;            // Use octree for spatial acceleration
} IrradianceCacheConfig;



// Main GI configuration
typedef struct {
    GIMethod method;
    GIQuality quality;
    f32 overall_intensity;      // Global GI brightness scale
    bool enable_color_bleeding; // Allow indirect color transfer
    bool enable_indirect_shadows; // AO from GI
    bool temporal_coherence;    // Reuse previous frame data

    union {
        PathTracingConfig path_tracing;
        PhotonMappingConfig photon_mapping;
        IrradianceCacheConfig irradiance_cache;
        LightProbeConfig light_probes;
    } config;
} GIConfig;

// ============================================================================
// Path Tracing
// ============================================================================

/// Trace a single path for global illumination
/// Follows ray through scene, accumulating light from all bounces
/// Args:
///   ray_origin: Starting position
///   ray_direction: Ray direction (normalized)
///   config: Path tracing configuration
///   random_seed: Seed for random number generation
///   out_radiance: Output radiance value (accumulated light)
///   out_sample_count: Output for statistical information
void gi_path_trace(Vec3 ray_origin, Vec3 ray_direction,
                  const PathTracingConfig* config, u32* random_seed,
                  Vec3* out_radiance, u32* out_sample_count);

/// Path trace with next event estimation (direct lighting)
/// More efficient than standard path tracing for direct light sampling
/// Args:
///   ray_origin: Ray start point
///   ray_direction: Ray direction
///   config: Configuration
///   random_seed: Random seed
///   out_direct: Output direct light contribution
///   out_indirect: Output indirect light contribution
void gi_path_trace_nee(Vec3 ray_origin, Vec3 ray_direction,
                      const PathTracingConfig* config, u32* random_seed,
                      Vec3* out_direct, Vec3* out_indirect);

/// Multiple importance sampling for path tracing
/// Combines BSDF and light sampling for better convergence
/// Args:
///   position: Surface position
///   normal: Surface normal
///   incoming_direction: Direction ray came from
///   config: Configuration
///   random_seed: Random seed
///   out_radiance: Estimated radiance
void gi_path_trace_mis(Vec3 position, Vec3 normal, Vec3 incoming_direction,
                      const PathTracingConfig* config, u32* random_seed,
                      Vec3* out_radiance);

// ============================================================================
// Photon Mapping
// ============================================================================

// Photon structure for photon mapping
typedef struct {
    Vec3 position;              // Photon position
    Vec3 direction;             // Photon travel direction
    Vec3 power;                 // Photon energy (color)
    u8 bounces;                 // Number of bounces so far
} Photon;

// Photon map structure
typedef struct {
    Photon* photons;
    u32 photon_count;
    u32 max_photons;

    // Spatial structure for fast lookup
    struct {
        u32* indices;           // Leaf indices
        u32 depth;              // Octree depth
    } octree;

    Vec3 grid_min, grid_max;    // Bounding box
} PhotonMap;

/// Initialize photon map
/// Args:
///   photon_map: Photon map to initialize
///   max_photons: Maximum photons to store
void gi_photon_map_init(PhotonMap* photon_map, u32 max_photons);

/// Cleanup photon map
void gi_photon_map_shutdown(PhotonMap* photon_map);

/// Trace photons through the scene
/// Emits photons from light sources and traces them
/// Args:
///   photon_map: Photon map to fill
///   config: Photon mapping configuration
///   light_power: Total power of light sources
void gi_trace_photons(PhotonMap* photon_map, const PhotonMappingConfig* config,
                     Vec3 light_power);

/// Estimate irradiance using photon map
/// Gathers nearby photons to estimate indirect lighting
/// Args:
///   position: Position to estimate irradiance at
///   normal: Surface normal
///   photon_map: Photon map to query
///   gather_count: Number of photons to gather
///   out_radiance: Output irradiance estimate
void gi_estimate_irradiance_photons(Vec3 position, Vec3 normal,
                                   const PhotonMap* photon_map,
                                   u32 gather_count, Vec3* out_radiance);

// ============================================================================
// Irradiance Caching
// ============================================================================

// Cached irradiance sample
typedef struct {
    Vec3 position;              // Sample position
    Vec3 normal;                // Surface normal
    Vec3 irradiance;            // Cached irradiance value
    f32 harmonic_mean_distance; // For interpolation weighting
    u32 num_photons;            // Photons used in estimate
    bool is_valid;              // Whether this sample is still valid
} IrradianceSample;

// Irradiance cache structure
typedef struct {
    IrradianceSample* samples;
    u32 sample_count;
    u32 max_samples;

    // Spatial acceleration
    struct {
        u32* node_indices;
        u32 depth;
    } octree;
} IrradianceCache;

/// Initialize irradiance cache
void gi_irradiance_cache_init(IrradianceCache* cache, u32 max_samples);

/// Shutdown irradiance cache
void gi_irradiance_cache_shutdown(IrradianceCache* cache);

/// Add irradiance sample to cache
/// Args:
///   cache: Cache to add to
///   position: Sample position
///   normal: Surface normal
///   irradiance: Computed irradiance
bool gi_irradiance_cache_add(IrradianceCache* cache, Vec3 position, Vec3 normal,
                            Vec3 irradiance);

/// Interpolate irradiance from cache
/// Uses nearby cached samples for fast lookup
/// Args:
///   cache: Cache to query
///   position: Query position
///   normal: Query normal
///   config: Cache configuration
///   out_irradiance: Output interpolated irradiance
bool gi_irradiance_cache_interpolate(const IrradianceCache* cache,
                                    Vec3 position, Vec3 normal,
                                    const IrradianceCacheConfig* config,
                                    Vec3* out_irradiance);

#include <include/rendering/light_probes.h>

// ============================================================================
// Utility Functions
// ============================================================================

/// Convert spherical harmonics to irradiance
/// Evaluates SH representation in a given direction
/// Args:
///   sh_coefficients: SH coefficients (9 values)
///   direction: Direction to evaluate
/// Returns: Irradiance in that direction
Vec3 gi_evaluate_sh_irradiance(const f32* sh_coefficients, Vec3 direction);

/// Project function onto spherical harmonics
/// Args:
///   directions: Array of directions
///   values: Array of values at those directions
///   num_samples: Number of samples
///   out_coefficients: Output SH coefficients (must be 9 floats)
void gi_project_to_sh(const Vec3* directions, const Vec3* values,
                     u32 num_samples, f32* out_coefficients);

/// Russian roulette termination for path tracing
/// Probabilistically terminates paths based on their contribution
/// Args:
///   path_throughput: Current path color/contribution
///   random_value: Random number [0, 1)
///   out_weight: Weight adjustment for the path
/// Returns: true if path should continue, false if terminated
bool gi_russian_roulette(Vec3 path_throughput, f32 random_value, f32* out_weight);

/// Compute direct light contribution
/// Samples light sources directly for faster convergence
/// Args:
///   position: Surface position
///   normal: Surface normal
///   random_seed: Random seed
/// Returns: Direct light contribution
Vec3 gi_compute_direct_light(Vec3 position, Vec3 normal, u32* random_seed);

// ============================================================================
// Adaptive Sampling
// ============================================================================

/// Variance estimate for convergence detection
/// Returns estimated convergence (0.0 = unconverged, 1.0 = fully converged)
f32 gi_estimate_convergence(const Vec3* samples, u32 num_samples);

/// Compute adaptive sample distribution
/// Allocates more samples to high-variance regions
/// Args:
///   pixel_variance: Variance map of current frame
///   width, height: Image dimensions
///   total_samples: Total samples to distribute
///   out_sample_distribution: Output sample counts per pixel
void gi_compute_adaptive_distribution(const f32* pixel_variance, u32 width,
                                     u32 height, u32 total_samples,
                                     u32* out_sample_distribution);

#endif // GLOBAL_ILLUMINATION_H
