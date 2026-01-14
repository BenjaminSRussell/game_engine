// include/render/normal_mapping.h
//
// Purpose: Normal mapping system for adding surface detail to blocks.
// Enables realistic surface variations (bumps, scratches, etc.) without
// increasing geometry complexity. Includes parallax mapping for depth perception.
//
// Public APIs:
// - `NormalMapConfig`: Configuration for normal mapping
// - `normal_map_perturb_normal`: Apply normal map to surface normal
// - `normal_map_get_world_normal`: Convert tangent-space to world-space normal
// - `parallax_map_uv`: Offset UV coordinates for parallax effect
// - `normal_map_blend`: Blend multiple normal maps
// - `normal_map_generate_tangents`: Generate tangent/bitangent for mesh
//

#ifndef NORMAL_MAPPING_H
#define NORMAL_MAPPING_H

#include <common.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

// ============================================================================
// Configuration
// ============================================================================

// Normal mapping quality levels
typedef enum {
    NORMAL_QUALITY_FAST = 1,     // Simple normal perturbation
    NORMAL_QUALITY_MEDIUM = 2,   // With parallax offset
    NORMAL_QUALITY_HIGH = 3,     // With relief mapping
    NORMAL_QUALITY_VERY_HIGH = 4 // With steep parallax mapping
} NormalQuality;

// Normal mapping configuration
typedef struct {
    NormalQuality quality;
    f32 normal_strength;          // How much to apply normal maps (0.0-2.0)
    f32 parallax_height;          // Height/displacement scale (0.0-0.1)
    bool use_relief_mapping;      // More expensive but more accurate
    bool use_parallax_occlusion;  // Prevent light leaks through parallax
    bool invert_green_channel;    // For different normal map formats
} NormalMapConfig;

// Surface tangent frame (TBN matrix)
typedef struct {
    Vec3 tangent;                 // Right vector in local space
    Vec3 bitangent;               // Up vector in local space
    Vec3 normal;                  // Forward vector (already provided)
} TangentFrame;

// ============================================================================
// Tangent Frame Generation
// ============================================================================

/// Generate tangent and bitangent vectors for a triangle
/// Required for converting normal maps to world space
/// Args:
///   position0, position1, position2: Triangle vertex positions
///   uv0, uv1, uv2: Triangle vertex UV coordinates
///   normal: Triangle normal (or face normal)
///   out_tangent: Output tangent vector (normalized)
///   out_bitangent: Output bitangent vector (normalized)
void normal_map_generate_tangent_frame(Vec3 position0, Vec3 position1, Vec3 position2,
                                       Vec2 uv0, Vec2 uv1, Vec2 uv2,
                                       Vec3 normal, Vec3* out_tangent,
                                       Vec3* out_bitangent);

/// Generate tangent frame for entire mesh
/// Args:
///   positions: Vertex position array
///   normals: Vertex normal array
///   uvs: Vertex UV array
///   num_vertices: Number of vertices
///   out_tangent_frames: Output tangent frames (must be pre-allocated)
void normal_map_generate_tangent_frames(const Vec3* positions, const Vec3* normals,
                                        const Vec2* uvs, u32 num_vertices,
                                        TangentFrame* out_tangent_frames);

// ============================================================================
// Normal Map Application
// ============================================================================

/// Decode normal from normal map texture (sRGB or BC5 format)
/// Args:
///   color: Sampled normal map color/data
///   invert_green: Whether to flip green channel (different map formats)
/// Returns: Decoded normal in tangent space (normalized)
Vec3 normal_map_decode(Vec4 color, bool invert_green);

/// Apply normal map perturbation to surface normal
/// Combines surface normal with perturbation from normal map
/// Args:
///   surface_normal: Original surface normal (world space)
///   mapped_normal: Normal from normal map (tangent space)
///   tangent_frame: Tangent/bitangent/normal basis
///   strength: How strong to apply the perturbation (0.0-2.0)
/// Returns: Perturbed normal (normalized, world space)
Vec3 normal_map_perturb_normal(Vec3 surface_normal, Vec3 mapped_normal,
                              const TangentFrame* tangent_frame, f32 strength);

/// Convert tangent-space normal to world space
/// Uses TBN matrix transformation
/// Args:
///   tangent_normal: Normal in tangent space
///   tangent_frame: TBN matrix for conversion
/// Returns: Normal in world space (normalized)
Vec3 normal_map_to_world_space(Vec3 tangent_normal, const TangentFrame* tangent_frame);

/// Convert world-space normal to tangent space
/// Inverse operation for analysis/debugging
/// Args:
///   world_normal: Normal in world space
///   tangent_frame: TBN matrix for conversion
/// Returns: Normal in tangent space (normalized)
Vec3 normal_map_to_tangent_space(Vec3 world_normal, const TangentFrame* tangent_frame);

// ============================================================================
// Parallax Mapping
// ============================================================================

/// Parallax mapping: offset UV based on view direction
/// Creates illusion of surface depth without additional geometry
/// Args:
///   uv: Original UV coordinates
///   view_direction: View direction in tangent space (normalized)
///   height_sample: Height value from heightmap (0.0-1.0)
///   height_scale: Parallax strength (0.0-0.1)
/// Returns: Offset UV coordinates
Vec2 parallax_map_uv(Vec2 uv, Vec3 view_direction, f32 height_sample, f32 height_scale);

/// Relief mapping: improved parallax with multiple samples
/// More accurate than simple parallax, but more expensive
/// Args:
///   uv: Original UV coordinates
///   view_direction: View direction in tangent space
///   height_scale: Parallax strength
///   step_count: Number of ray marching steps (8-32)
///   out_parallax_occlusion: Output parallax shadow factor
/// Returns: Offset UV coordinates with better accuracy
Vec2 relief_map_uv(Vec2 uv, Vec3 view_direction, f32 height_scale,
                  u32 step_count, f32* out_parallax_occlusion);

/// Steep parallax mapping: advanced parallax with occlusion handling
/// Args:
///   uv: Original UV coordinates
///   view_direction: View direction in tangent space
///   height_scale: Parallax strength
///   step_count: Number of coarse steps
///   refine_steps: Number of fine refinement steps
/// Returns: Offset UV coordinates with occlusion handling
Vec2 steep_parallax_map_uv(Vec2 uv, Vec3 view_direction, f32 height_scale,
                          u32 step_count, u32 refine_steps);

// ============================================================================
// Normal Map Blending
// ============================================================================

/// Blend two normal maps
/// Args:
///   normal1: First normal (world or tangent space)
///   normal2: Second normal (tangent space)
///   blend_factor: Blend amount (0.0 = 100% normal1, 1.0 = 100% normal2)
/// Returns: Blended normal
Vec3 normal_map_blend(Vec3 normal1, Vec3 normal2, f32 blend_factor);

/// Whiteout blend: blend using "whiteout" algorithm
/// Better for mixing multiple normal maps
/// Args:
///   normal1: First normal (normalized)
///   normal2: Second normal (normalized)
///   blend_amount: How much to blend (0.0-1.0)
/// Returns: Blended normal
Vec3 normal_map_blend_whiteout(Vec3 normal1, Vec3 normal2, f32 blend_amount);

/// Partial derivatives blend: mathematical blend for detail maps
/// Args:
///   normal1: Base normal
///   normal2: Detail normal
/// Returns: Combined normal
Vec3 normal_map_blend_partial_derivatives(Vec3 normal1, Vec3 normal2);

/// Layer blending with multiple normals
/// Args:
///   normals: Array of normal vectors to blend
///   weights: Blend weight for each normal (should sum to 1.0)
///   num_layers: Number of normal maps to blend
/// Returns: Final blended normal
Vec3 normal_map_blend_layers(const Vec3* normals, const f32* weights, u32 num_layers);

// ============================================================================
// Detail Normal Maps
// ============================================================================

/// Apply detail normal map for micro-surface variations
/// Adds fine detail to base normal
/// Args:
///   base_normal: Base surface normal
///   detail_normal: Detail normal from detail map
///   detail_scale: How much to apply detail (0.0-1.0)
/// Returns: Combined normal with detail
Vec3 normal_map_apply_detail(Vec3 base_normal, Vec3 detail_normal, f32 detail_scale);

/// Generate detail normal map from heightfield
/// Creates synthetic detail normals for procedural surfaces
/// Args:
///   position: Surface position
///   frequency: Detail frequency (how much detail)
///   amplitude: Detail amplitude (how large)
/// Returns: Detail normal
Vec3 normal_map_generate_detail(Vec3 position, f32 frequency, f32 amplitude);

// ============================================================================
// Normal Map Utilities
// ============================================================================

/// Calculate surface roughness from normal variance
/// Rougher normal maps = higher roughness value
/// Args:
///   normal_samples: Array of sampled normals
///   num_samples: Number of samples
/// Returns: Estimated roughness (0.0 = smooth, 1.0 = very rough)
f32 normal_map_estimate_roughness(const Vec3* normal_samples, u32 num_samples);

/// Reconstruct Z component of normal map
/// Some formats only store X and Y (DXT5, BC5)
/// Args:
///   normal_xy: XY components of normal
/// Returns: Reconstructed normal with Z component
Vec3 normal_map_reconstruct_z(Vec2 normal_xy);

/// Mipmap normal map for LOD
/// Downsamples normal map while preserving quality
/// Args:
///   normals: Input normal array (must be square, power of 2)
///   width: Input width
///   out_mip: Output mipmap (half resolution, must be pre-allocated)
void normal_map_create_mip(const Vec3* normals, u32 width, Vec3* out_mip);

// ============================================================================
// Advanced Techniques
// ============================================================================

/// Compute surface curvature from normal variation
/// Used for cavity mapping and weathering effects
/// Args:
///   positions: Vertex positions
///   normals: Vertex normals
///   num_vertices: Number of vertices
///   radius: Neighborhood radius
///   out_curvature: Output curvature values (must be pre-allocated)
void normal_map_compute_curvature(const Vec3* positions, const Vec3* normals,
                                 u32 num_vertices, f32 radius, f32* out_curvature);

/// Apply triplanar projection for seamless tiling
/// Useful for tiling blocks without visible seams
/// Args:
///   position: World position
///   normal: Surface normal
///   normal_x, normal_y, normal_z: Normal maps for each axis
///   scale: Tiling scale
/// Returns: Blended normal from triplanar projection
Vec3 normal_map_triplanar(Vec3 position, Vec3 normal,
                         Vec3 normal_x, Vec3 normal_y, Vec3 normal_z, f32 scale);

#endif // NORMAL_MAPPING_H
