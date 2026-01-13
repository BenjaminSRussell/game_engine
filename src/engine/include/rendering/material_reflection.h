// include/render/material_reflection.h
//
// Purpose: Material reflection and refraction system for ray tracing.
// Handles physically-based surface interactions including Fresnel effects,
// specular/diffuse splits, refraction indices, and roughness-based scattering.
//
// Public APIs:
// - `ReflectionProperties`: Material surface properties for light interaction
// - `RefractionProperties`: Transparency and light transmission properties
// - `reflection_compute_fresnel`: Calculate Fresnel effect (Schlick's approximation)
// - `reflection_split_ray`: Determine if ray reflects or refracts
// - `reflection_get_specular_contribution`: Calculate specular reflection strength
// - `reflection_calculate_microfacet_normal`: Perturbation for roughness
// - `refraction_calculate_direction`: Compute refracted ray direction using Snell's law
// - `refraction_get_transmission_color`: Get transmission tint for glass/water
//

#ifndef MATERIAL_REFLECTION_H
#define MATERIAL_REFLECTION_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/vec4.h>

// ============================================================================
// Reflection & Refraction Properties
// ============================================================================

// Surface reflection properties
typedef struct {
    f32 fresnel_f0;           // Fresnel at normal incidence (0.0-1.0)
    f32 roughness;            // Surface roughness (0.0 = mirror, 1.0 = diffuse)
    f32 metallic;             // Metallic value (0.0 = dielectric, 1.0 = metal)
    f32 specular_intensity;   // Overall specular strength (0.0-2.0)
    Vec3 specular_color;      // Specular reflection tint (for metals)
    bool is_anisotropic;      // Enable anisotropic reflection
    f32 anisotropy_strength;  // Anisotropy direction preference
} ReflectionProperties;

// Surface refraction properties (transmission)
typedef struct {
    f32 ior;                  // Index of refraction (1.0-2.5)
    f32 transmission;         // Light transmission amount (0.0-1.0)
    Vec4 transmission_color;  // Color filter for transmitted light
    f32 thickness;            // Material thickness for absorption
    f32 attenuation;          // Color attenuation per unit distance
    bool thin_walled;         // Treat as thin surface (no internal scattering)
} RefractionProperties;

// Combined material surface properties
typedef struct {
    ReflectionProperties reflection;
    RefractionProperties refraction;

    // Subsurface scattering (for skin, wax, etc)
    f32 subsurface_amount;
    Vec3 subsurface_color;
    f32 subsurface_radius;

    // Clearcoat layer (secondary reflection)
    f32 clearcoat_amount;
    f32 clearcoat_roughness;

    // Sheen (for fabric)
    f32 sheen_amount;
    Vec3 sheen_color;
} MaterialSurfaceProperties;

// ============================================================================
// Fresnel Calculations
// ============================================================================

/// Calculate Fresnel reflection coefficient using Schlick's approximation
/// Computes the proportion of light that reflects vs refracts at a surface
/// Args:
///   cos_theta: Cosine of angle between ray and surface normal
///   fresnel_f0: Fresnel value at normal incidence (0.0-1.0)
///   roughness: Surface roughness (affects edge brightness)
/// Returns: Reflection coefficient (0.0-1.0)
f32 reflection_compute_fresnel_schlick(f32 cos_theta, f32 fresnel_f0, f32 roughness);

/// Extended Fresnel calculation for metals and dielectrics
/// Uses wavelength-dependent calculation for more accurate colors
/// Args:
///   cos_theta: Cosine of angle between ray and surface normal
///   metallic: Metallic parameter (0.0-1.0)
///   specular_color: Specular tint for metals
///   roughness: Surface roughness
/// Returns: Fresnel reflection coefficient
f32 reflection_compute_fresnel_extended(f32 cos_theta, f32 metallic,
                                        Vec3 specular_color, f32 roughness);

/// Complex IOR-based Fresnel for precise materials
/// Uses full complex refractive index for physically accurate rendering
/// Args:
///   cos_theta: Cosine of angle between ray and surface normal
///   ior: Index of refraction (1.0-2.5)
///   extinction: Extinction coefficient (for metals)
/// Returns: Fresnel reflection coefficient
f32 reflection_compute_fresnel_ior(f32 cos_theta, f32 ior, f32 extinction);

// ============================================================================
// Ray Path Decision
// ============================================================================

/// Determine if ray should reflect or refract
/// Uses Monte Carlo sampling with Fresnel probability
/// Args:
///   surface_props: Material surface properties
///   cos_theta: Cosine of angle between incoming ray and surface normal
///   random_value: Random number [0.0, 1.0) for probabilistic decision
/// Returns: true if ray should reflect, false if ray should refract
bool reflection_should_reflect(const MaterialSurfaceProperties* surface_props,
                              f32 cos_theta, f32 random_value);

/// Get the split between specular reflection and diffuse reflection
/// Args:
///   surface_props: Material surface properties
///   cos_theta: Cosine of angle with surface normal
/// Returns: Struct with specular_weight and diffuse_weight (sum to 1.0)
typedef struct {
    f32 specular_weight;  // Proportion of ray that reflects specularly
    f32 diffuse_weight;   // Proportion of ray that reflects diffusely
} ReflectionSplit;

ReflectionSplit reflection_get_split(const MaterialSurfaceProperties* surface_props,
                                     f32 cos_theta);

/// Calculate specular reflection contribution
/// Returns intensity and color modification for specular reflections
/// Args:
///   reflection_props: Reflection properties
///   cos_theta: Cosine of angle with surface normal
///   ior_ratio: Ratio of incoming to outgoing IOR
/// Returns: Color contribution for specular reflection
Vec3 reflection_get_specular_contribution(const ReflectionProperties* reflection_props,
                                          f32 cos_theta, f32 ior_ratio);

// ============================================================================
// Microfacet Roughness
// ============================================================================

/// Generate perturbed normal for rough surface reflection
/// Simulates microfacet roughness using GGX/Trowbridge-Reitz distribution
/// Args:
///   surface_normal: Original surface normal
///   tangent: Tangent vector for surface orientation
///   bitangent: Bitangent vector for surface orientation
///   roughness: Roughness value (0.0 = smooth, 1.0 = very rough)
///   random_u, random_v: Random numbers [0.0, 1.0)
///   anisotropy_strength: Anisotropy parameter (0.0 = isotropic)
/// Returns: Perturbed normal for ray reflection
Vec3 reflection_calculate_microfacet_normal(Vec3 surface_normal, Vec3 tangent,
                                            Vec3 bitangent, f32 roughness,
                                            f32 random_u, f32 random_v,
                                            f32 anisotropy_strength);

/// Distribution function for microfacet roughness (GGX)
/// Probability density of microfacets with given half-vector
f32 reflection_ggx_distribution(f32 cos_h, f32 roughness);

/// Geometric attenuation for microfacets
/// Models shadowing/masking of microfacets
f32 reflection_smith_geometry(f32 cos_in, f32 cos_out, f32 roughness);

// ============================================================================
// Refraction Calculations
// ============================================================================

/// Calculate refracted ray direction using Snell's law
/// Computes the direction of a ray after passing through a refractive surface
/// Args:
///   incoming_direction: Normalized incoming ray direction
///   surface_normal: Normalized surface normal (pointing outward)
///   cos_theta_in: Cosine of incoming angle (dot of incoming and normal)
///   ior_in: Index of refraction of incoming medium (usually 1.0 for air)
///   ior_out: Index of refraction of outgoing medium
///   out_cos_theta_out: Output - cosine of refracted angle
/// Returns: Normalized refracted ray direction (or zero vector if total internal reflection)
Vec3 refraction_calculate_direction(Vec3 incoming_direction, Vec3 surface_normal,
                                    f32 cos_theta_in, f32 ior_in, f32 ior_out,
                                    f32* out_cos_theta_out);

/// Get transmission color for refracted rays
/// Applies color filtering and absorption to transmitted light
/// Args:
///   refraction_props: Refraction properties
///   distance_traveled: Distance ray traveled through medium
/// Returns: Color filter for refracted light
Vec3 refraction_get_transmission_color(const RefractionProperties* refraction_props,
                                       f32 distance_traveled);

/// Calculate caustic pattern for water/glass
/// Generates realistic caustic effects for light refracted through water/glass
/// Args:
///   position: 3D world position
///   time: Time for animation
///   frequency: Pattern frequency (wavelength)
/// Returns: Caustic intensity [0.0, 1.0]
f32 refraction_calculate_caustic(Vec3 position, f32 time, f32 frequency);

// ============================================================================
// Presets for Common Materials
// ============================================================================

/// Get reflection properties for water
MaterialSurfaceProperties material_get_water_properties(void);

/// Get reflection properties for glass
MaterialSurfaceProperties material_get_glass_properties(void);

/// Get reflection properties for metal
MaterialSurfaceProperties material_get_metal_properties(f32 roughness);

/// Get reflection properties for skin
MaterialSurfaceProperties material_get_skin_properties(void);

/// Get reflection properties for fabric
MaterialSurfaceProperties material_get_fabric_properties(void);

/// Get reflection properties for plastic
MaterialSurfaceProperties material_get_plastic_properties(bool glossy);

#endif // MATERIAL_REFLECTION_H
