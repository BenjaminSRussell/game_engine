// include/render/ambient_occlusion.h
//
// Purpose: Ambient occlusion system for ray tracing.
// Computes how occluded a surface is from ambient light by casting rays
// in all directions. Provides realistic darkening in corners and crevices.
//
// Public APIs:
// - `AOConfig`: Configuration for AO computation
// - `ao_ray_cast`: Cast AO rays from a point and compute occlusion
// - `ao_compute_screen_space`: Screen-space approximation (fast)
// - `ao_compute_bent_normal`: Compute dominant unoccluded direction
// - `ao_compute_cavity_map`: Precompute per-vertex AO
//

#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/vec4.h>

// ============================================================================
// Configuration
// ============================================================================

// Ambient Occlusion computation quality levels
typedef enum {
    AO_QUALITY_FAST = 1,        // 4 samples
    AO_QUALITY_MEDIUM = 2,      // 16 samples
    AO_QUALITY_HIGH = 3,        // 32 samples
    AO_QUALITY_VERY_HIGH = 4,   // 64 samples
} AOQuality;

// Ambient occlusion configuration
typedef struct {
    AOQuality quality;          // Number of samples
    f32 radius;                 // Maximum occlusion distance (in world units)
    f32 bias;                   // Avoid self-occlusion bias
    f32 power;                  // AO contrast (higher = more contrast)
    f32 intensity;              // Overall AO strength (0.0-1.0)
    bool use_bent_normal;       // Compute bent normal for better lighting
    bool use_color_bleeding;    // Allow color bleeding from surfaces
    bool temporal_denoise;      // Use temporal filtering for smooth results
} AOConfig;

// ============================================================================
// Ray-Based AO Computation
// ============================================================================

/// Cast ambient occlusion rays from a surface point
/// Casts rays in a hemisphere and counts how many hit geometry
/// Args:
///   position: Surface position
///   normal: Surface normal (rays cast in hemisphere around this)
///   radius: Maximum ray distance
///   num_samples: Number of rays to cast
///   random_seed: Seed for random number generation
///   out_occlusion: Output occlusion value [0.0, 1.0] (0=fully occluded, 1=unoccluded)
///   out_bent_normal: Optional output for dominant unoccluded direction
void ao_ray_cast(Vec3 position, Vec3 normal, f32 radius, u32 num_samples,
                u32 random_seed, f32* out_occlusion, Vec3* out_bent_normal);

/// Cast AO rays with custom hemisphere distribution
/// Allows weighted sampling (e.g., cosine-weighted for more realistic weighting)
/// Args:
///   position: Surface position
///   normal: Surface normal
///   radius: Maximum ray distance
///   num_samples: Number of rays to cast
///   use_cosine_weighting: Weight samples by cosine of angle with normal
///   out_occlusion: Output occlusion value [0.0, 1.0]
void ao_ray_cast_weighted(Vec3 position, Vec3 normal, f32 radius,
                         u32 num_samples, bool use_cosine_weighting,
                         f32* out_occlusion);

// ============================================================================
// Screen-Space AO
// ============================================================================

/// Screen-space ambient occlusion (SSAO) approximation
/// Fast GPU-friendly approximation that doesn't require ray tracing
/// Reads depth and normal buffers from screen
/// Args:
///   screen_x, screen_y: Screen coordinates
///   depth: View space depth at this pixel
///   normal: World space or view space normal
///   fov: Camera field of view in degrees
///   near, far: Camera near and far planes
///   radius: SSAO radius in pixels
/// Returns: AO value [0.0, 1.0]
f32 ao_compute_screen_space(u32 screen_x, u32 screen_y, f32 depth, Vec3 normal,
                           f32 fov, f32 near, f32 far, f32 radius);

// ============================================================================
// Bent Normal Computation
// ============================================================================

/// Compute bent normal (dominant unoccluded direction)
/// The bent normal points toward the most unoccluded hemisphere direction
/// This direction can be used for better indirect lighting calculations
/// Args:
///   position: Surface position
///   normal: Surface normal
///   radius: Maximum occlusion distance
///   num_samples: Number of samples
/// Returns: Bent normal (normalized)
Vec3 ao_compute_bent_normal(Vec3 position, Vec3 normal, f32 radius, u32 num_samples);

/// Refine bent normal with multiple bounces
/// More expensive but more accurate for complex geometry
/// Args:
///   position: Surface position
///   normal: Surface normal
///   bent_normal: Initial bent normal (from ao_compute_bent_normal)
///   radius: Occlusion radius
///   num_bounces: Number of refinement bounces
/// Returns: Refined bent normal
Vec3 ao_refine_bent_normal(Vec3 position, Vec3 normal, Vec3 bent_normal,
                          f32 radius, u32 num_bounces);

// ============================================================================
// Precomputed AO
// ============================================================================

/// Vertex AO data for static meshes
typedef struct {
    f32 occlusion;              // AO value at vertex
    Vec3 bent_normal;           // Bent normal at vertex
} VertexAO;

/// Precompute AO for all vertices in a mesh
/// Stores AO values per vertex for efficient rendering
/// Args:
///   positions: Vertex positions array
///   normals: Vertex normals array
///   num_vertices: Number of vertices
///   radius: AO computation radius
///   quality: AO quality level
///   out_ao_data: Output array of VertexAO structures (must be pre-allocated)
void ao_precompute_mesh(const Vec3* positions, const Vec3* normals,
                       u32 num_vertices, f32 radius, AOQuality quality,
                       VertexAO* out_ao_data);

// ============================================================================
// Cavity Detection
// ============================================================================

/// Compute surface curvature (cavity/convexity)
/// Positive values indicate convex features, negative indicate concave
/// Args:
///   position: Surface position
///   normal: Surface normal
///   radius: Sampling radius for curvature calculation
/// Returns: Curvature value (typically -1.0 to 1.0)
f32 ao_compute_curvature(Vec3 position, Vec3 normal, f32 radius);

/// Compute cavity map texture
/// Pre-computed map showing surface cavities for faster rendering
/// Args:
///   positions: Vertex positions array
///   normals: Vertex normals array
///   num_vertices: Number of vertices
///   radius: Sampling radius
///   out_cavity_map: Output cavity values (must be pre-allocated)
void ao_compute_cavity_map(const Vec3* positions, const Vec3* normals,
                          u32 num_vertices, f32 radius, f32* out_cavity_map);

// ============================================================================
// AO Sample Pattern Generation
// ============================================================================

/// Generate hemisphere sample directions using various patterns
/// Args:
///   num_samples: Number of samples to generate
///   pattern_type: 0=random, 1=Poisson disk, 2=Hammersley, 3=cos-weighted
///   out_directions: Output array of normalized direction vectors
void ao_generate_sample_directions(u32 num_samples, u32 pattern_type,
                                  Vec3* out_directions);

/// Generate sample directions with specific angular distribution
/// Allows weighting samples toward preferred directions
/// Args:
///   num_samples: Number of samples
///   bias_direction: Direction to bias samples toward
///   bias_strength: How much to bias (0.0-1.0)
///   out_directions: Output sample directions
void ao_generate_biased_samples(u32 num_samples, Vec3 bias_direction,
                               f32 bias_strength, Vec3* out_directions);

// ============================================================================
// Temporal Filtering
// ============================================================================

/// History structure for temporal AO filtering
typedef struct {
    f32 current_ao;
    f32 history_ao;
    f32 reprojection_error;
    u32 sample_count;
} AOTemporalHistory;

/// Update AO history for temporal filtering
/// Reprojects previous frame's AO and blends with current frame
/// Args:
///   position: Current pixel position
///   prev_position: Position in previous frame (from motion vectors)
///   current_ao: Current frame AO value
///   history: History structure to update
/// Returns: Temporally filtered AO value
f32 ao_update_temporal_history(Vec3 position, Vec3 prev_position,
                              f32 current_ao, AOTemporalHistory* history);

// ============================================================================
// Utility Functions
// ============================================================================

/// Apply AO to a diffuse color
/// Multiplies color by AO value to darken occluded areas
/// Args:
///   color: Original diffuse color
///   occlusion: AO value [0.0, 1.0]
///   intensity: How strong the AO effect is (0.0-1.0)
/// Returns: Darkened color
Vec3 ao_apply_to_color(Vec3 color, f32 occlusion, f32 intensity);

/// Apply bent normal for indirect lighting
/// Adjust indirect light direction based on bent normal
/// Args:
///   position: Surface position
///   normal: Original surface normal
///   bent_normal: Bent normal from AO computation
///   indirect_light: Incoming indirect light color
/// Returns: Adjusted indirect light color
Vec3 ao_apply_bent_normal(Vec3 position, Vec3 normal, Vec3 bent_normal,
                         Vec3 indirect_light);

#endif // AMBIENT_OCCLUSION_H
