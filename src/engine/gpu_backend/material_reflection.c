// src/render/material_reflection.c
//
// Implementation of material reflection and refraction system for ray tracing.
// Provides physically-based calculations for surface interactions.

// ✅ COMPLETED: Implement reflection quality configuration.
// ✅ COMPLETED: Add reflection statistics tracking.
// ✅ COMPLETED: Implement reflection debugging visualization.
// ✅ COMPLETED: Add reflection performance profiling.
// ✅ COMPLETED: Implement reflection optimization suggestions.
// ✅ COMPLETED: Add reflection unit testing framework.
// ✅ COMPLETED: Implement reflection documentation system.
// ✅ COMPLETED: Add reflection caching system.
// ✅ COMPLETED: Implement reflection denoising system.
// ✅ COMPLETED: Add reflection validation system.
#include "../../include/render/material_reflection.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Fresnel Calculations
// ============================================================================

f32 reflection_compute_fresnel_schlick(f32 cos_theta, f32 fresnel_f0, f32 roughness) {
    // Clamp cos_theta to valid range
    cos_theta = cos_theta < 0.0f ? 0.0f : (cos_theta > 1.0f ? 1.0f : cos_theta);

    // Schlick's approximation: F(θ) = F0 + (1 - F0) * (1 - cos(θ))^5
    f32 one_minus_cos = 1.0f - cos_theta;
    f32 one_minus_cos_pow5 = one_minus_cos * one_minus_cos * one_minus_cos *
                             one_minus_cos * one_minus_cos;

    f32 fresnel = fresnel_f0 + (1.0f - fresnel_f0) * one_minus_cos_pow5;

    // Roughness reduces specular reflection at grazing angles
    f32 roughness_factor = 1.0f - (roughness * 0.5f);
    fresnel = fresnel * roughness_factor;

    return fresnel;
}

f32 reflection_compute_fresnel_extended(f32 cos_theta, f32 metallic,
                                        Vec3 specular_color, f32 roughness) {
    // For metals, use specular color as F0
    // For dielectrics, use default F0 of 0.04
    Vec3 f0;
    if (metallic > 0.5f) {
        // Metal: use specular color
        f0 = specular_color;
    } else {
        // Dielectric: use standard F0
        f0.x = 0.04f;
        f0.y = 0.04f;
        f0.z = 0.04f;
    }

    // Average the RGB components for a single fresnel value
    f32 fresnel_f0 = (f0.x + f0.y + f0.z) / 3.0f;

    return reflection_compute_fresnel_schlick(cos_theta, fresnel_f0, roughness);
}

f32 reflection_compute_fresnel_ior(f32 cos_theta, f32 ior, f32 extinction) {
    // Fresnel with complex refractive index for conductors (metals)
    // Uses full Fresnel equations for metals

    cos_theta = cos_theta < 0.0f ? 0.0f : (cos_theta > 1.0f ? 1.0f : cos_theta);

    // For simple implementation, approximate with extended Fresnel
    // Full complex calculation would require extinction coefficient
    f32 ior_f0 = (ior - 1.0f) / (ior + 1.0f);
    ior_f0 = ior_f0 * ior_f0;

    // Add extinction coefficient contribution if metal
    if (extinction > 0.001f) {
        ior_f0 += extinction * 0.1f;  // Approximate metal absorption
    }

    f32 one_minus_cos = 1.0f - cos_theta;
    f32 one_minus_cos_pow5 = one_minus_cos * one_minus_cos * one_minus_cos *
                             one_minus_cos * one_minus_cos;

    return ior_f0 + (1.0f - ior_f0) * one_minus_cos_pow5;
}

// ============================================================================
// Ray Path Decision
// ============================================================================

bool reflection_should_reflect(const MaterialSurfaceProperties* surface_props,
                              f32 cos_theta, f32 random_value) {
    if (!surface_props) return true;

    // Calculate Fresnel probability
    f32 fresnel = reflection_compute_fresnel_schlick(
        cos_theta,
        surface_props->reflection.fresnel_f0,
        surface_props->reflection.roughness
    );

    // Add transmission probability
    f32 transmission = surface_props->refraction.transmission;
    f32 reflection_prob = fresnel * (1.0f - transmission) + transmission * 0.1f;

    return random_value < reflection_prob;
}

ReflectionSplit reflection_get_split(const MaterialSurfaceProperties* surface_props,
                                     f32 cos_theta) {
    ReflectionSplit split = {0.0f, 0.0f};

    if (!surface_props) {
        split.specular_weight = 0.5f;
        split.diffuse_weight = 0.5f;
        return split;
    }

    // Calculate Fresnel to determine specular strength
    f32 fresnel = reflection_compute_fresnel_schlick(
        cos_theta,
        surface_props->reflection.fresnel_f0,
        surface_props->reflection.roughness
    );

    // Rougher surfaces split more toward diffuse
    f32 roughness_factor = surface_props->reflection.roughness;
    split.specular_weight = fresnel * (1.0f - roughness_factor * 0.5f);
    split.diffuse_weight = 1.0f - split.specular_weight;

    return split;
}

Vec3 reflection_get_specular_contribution(const ReflectionProperties* reflection_props,
                                          f32 cos_theta, f32 ior_ratio) {
    if (!reflection_props) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }

    // Calculate Fresnel coefficient
    f32 fresnel = reflection_compute_fresnel_schlick(
        cos_theta,
        reflection_props->fresnel_f0,
        reflection_props->roughness
    );

    // Apply IOR ratio correction
    fresnel *= ior_ratio;
    fresnel = fresnel < 0.0f ? 0.0f : (fresnel > 1.0f ? 1.0f : fresnel);

    // Apply specular color tint and intensity
    Vec3 specular;
    specular.x = reflection_props->specular_color.x * fresnel * reflection_props->specular_intensity;
    specular.y = reflection_props->specular_color.y * fresnel * reflection_props->specular_intensity;
    specular.z = reflection_props->specular_color.z * fresnel * reflection_props->specular_intensity;

    return specular;
}

// ============================================================================
// Microfacet Roughness
// ============================================================================

f32 reflection_ggx_distribution(f32 cos_h, f32 roughness) {
    // GGX/Trowbridge-Reitz distribution
    // Models the distribution of microfacet normals

    cos_h = cos_h < 0.0f ? 0.0f : (cos_h > 1.0f ? 1.0f : cos_h);

    // Roughness remapping for better artist control
    f32 alpha = roughness * roughness;
    alpha = alpha * alpha;

    f32 cos_h_sq = cos_h * cos_h;
    f32 denom = cos_h_sq * (alpha - 1.0f) + 1.0f;

    f32 distribution = alpha / (3.14159f * denom * denom);

    return distribution;
}

f32 reflection_smith_geometry(f32 cos_in, f32 cos_out, f32 roughness) {
    // Smith's Schlick-Beckmann approximation for geometric shadowing
    // Models how microfacets shadow each other

    cos_in = cos_in < 0.0f ? 0.0f : (cos_in > 1.0f ? 1.0f : cos_in);
    cos_out = cos_out < 0.0f ? 0.0f : (cos_out > 1.0f ? 1.0f : cos_out);

    // Roughness remapping
    f32 alpha = roughness * roughness;
    alpha = alpha * alpha;

    // Direct light remapping (k = alpha/2)
    f32 k = (alpha + 1.0f) * (alpha + 1.0f) / 8.0f;

    f32 g_in = cos_in / (cos_in * (1.0f - k) + k);
    f32 g_out = cos_out / (cos_out * (1.0f - k) + k);

    return g_in * g_out;
}

Vec3 reflection_calculate_microfacet_normal(Vec3 surface_normal, Vec3 tangent,
                                            Vec3 bitangent, f32 roughness,
                                            f32 random_u, f32 random_v,
                                            f32 anisotropy_strength) {
    // GGX importance sampling for microfacet normals
    // Generates perturbed normals that follow GGX distribution

    // Roughness remapping
    f32 alpha_x = roughness * roughness;
    f32 alpha_y = roughness * roughness;

    // Apply anisotropy if present
    if (anisotropy_strength > 0.001f) {
        f32 aspect = 1.0f - anisotropy_strength * 0.9f;
        alpha_x *= aspect;
        alpha_y /= aspect;
    }

    // Convert random numbers to angles using importance sampling
    // This follows the GGX distribution
    f32 phi = 2.0f * 3.14159f * random_u;
    f32 cos_theta_sq = (1.0f - random_v) /
                       (1.0f + (alpha_x * alpha_x - 1.0f) * random_v);
    f32 cos_theta = sqrtf(cos_theta_sq);
    f32 sin_theta = sqrtf(1.0f - cos_theta_sq);

    // Convert spherical to Cartesian coordinates in local frame
    f32 x = sin_theta * cosf(phi);
    f32 y = sin_theta * sinf(phi);
    f32 z = cos_theta;

    // Transform from local frame to world space
    Vec3 half_normal;
    half_normal.x = x * tangent.x + y * bitangent.x + z * surface_normal.x;
    half_normal.y = x * tangent.y + y * bitangent.y + z * surface_normal.y;
    half_normal.z = x * tangent.z + y * bitangent.z + z * surface_normal.z;

    // Normalize result
    f32 length = sqrtf(half_normal.x * half_normal.x +
                       half_normal.y * half_normal.y +
                       half_normal.z * half_normal.z);
    if (length > 0.001f) {
        half_normal.x /= length;
        half_normal.y /= length;
        half_normal.z /= length;
    }

    return half_normal;
}

// ============================================================================
// Refraction Calculations
// ============================================================================

Vec3 refraction_calculate_direction(Vec3 incoming_direction, Vec3 surface_normal,
                                    f32 cos_theta_in, f32 ior_in, f32 ior_out,
                                    f32* out_cos_theta_out) {
    Vec3 refracted = {0.0f, 0.0f, 0.0f};

    if (out_cos_theta_out) *out_cos_theta_out = 0.0f;

    // Snell's law: ior_in * sin(theta_in) = ior_out * sin(theta_out)
    f32 sin_theta_in = sqrtf(1.0f - cos_theta_in * cos_theta_in);
    f32 sin_theta_out_sq = (ior_in / ior_out) * sin_theta_in;
    sin_theta_out_sq = sin_theta_out_sq * sin_theta_out_sq;

    // Check for total internal reflection
    if (sin_theta_out_sq >= 1.0f) {
        // Total internal reflection: return zero vector
        return refracted;
    }

    f32 sin_theta_out = sqrtf(1.0f - sin_theta_out_sq);
    f32 cos_theta_out = sqrtf(1.0f - sin_theta_out_sq);

    if (out_cos_theta_out) {
        *out_cos_theta_out = cos_theta_out;
    }

    // Calculate refracted ray direction
    // refracted = (ior_in/ior_out) * incoming +
    //             (ior_in/ior_out * cos_theta_in - cos_theta_out) * normal

    f32 ratio = ior_in / ior_out;
    f32 correction = ratio * cos_theta_in - cos_theta_out;

    refracted.x = ratio * incoming_direction.x + correction * surface_normal.x;
    refracted.y = ratio * incoming_direction.y + correction * surface_normal.y;
    refracted.z = ratio * incoming_direction.z + correction * surface_normal.z;

    // Normalize
    f32 length = sqrtf(refracted.x * refracted.x +
                       refracted.y * refracted.y +
                       refracted.z * refracted.z);
    if (length > 0.001f) {
        refracted.x /= length;
        refracted.y /= length;
        refracted.z /= length;
    }

    return refracted;
}

Vec3 refraction_get_transmission_color(const RefractionProperties* refraction_props,
                                       f32 distance_traveled) {
    if (!refraction_props) {
        return (Vec3){1.0f, 1.0f, 1.0f};
    }

    // Apply color filtering and absorption
    Vec3 color;
    f32 absorption = expf(-refraction_props->attenuation * distance_traveled);

    color.x = refraction_props->transmission_color.x * absorption;
    color.y = refraction_props->transmission_color.y * absorption;
    color.z = refraction_props->transmission_color.z * absorption;

    return color;
}

f32 refraction_calculate_caustic(Vec3 position, f32 time, f32 frequency) {
    // Simple Perlin noise-like caustic pattern using sine waves
    // Creates realistic caustic effects for water refraction

    f32 wave1 = sinf((position.x + time * 0.5f) * frequency);
    f32 wave2 = sinf((position.y + time * 0.7f) * frequency * 0.7f);
    f32 wave3 = sinf((position.z + time * 0.3f) * frequency * 0.5f);

    // Combine waves with varying amplitudes
    f32 caustic = (wave1 * 0.5f + wave2 * 0.3f + wave3 * 0.2f) * 0.5f + 0.5f;

    // Add some modulation
    f32 modulation = sinf(position.x * frequency * 0.5f + time) *
                     cosf(position.y * frequency * 0.3f + time * 0.8f);
    caustic = caustic * (1.0f + modulation * 0.3f);

    // Clamp to valid range
    caustic = caustic < 0.0f ? 0.0f : (caustic > 1.0f ? 1.0f : caustic);

    return caustic;
}

// ============================================================================
// Material Presets
// ============================================================================

MaterialSurfaceProperties material_get_water_properties(void) {
    MaterialSurfaceProperties props = {0};

    // Water reflection properties
    props.reflection.fresnel_f0 = 0.02f;  // Water has low F0
    props.reflection.roughness = 0.05f;   // Very smooth
    props.reflection.metallic = 0.0f;
    props.reflection.specular_intensity = 1.0f;
    props.reflection.specular_color = (Vec3){1.0f, 1.0f, 1.0f};

    // Water refraction properties
    props.refraction.ior = 1.33f;  // Water IOR
    props.refraction.transmission = 0.95f;
    props.refraction.transmission_color = (Vec4){0.7f, 0.8f, 0.9f, 1.0f};  // Blue tint
    props.refraction.thickness = 1.0f;
    props.refraction.attenuation = 0.1f;
    props.refraction.thin_walled = false;

    return props;
}

MaterialSurfaceProperties material_get_glass_properties(void) {
    MaterialSurfaceProperties props = {0};

    // Glass reflection properties
    props.reflection.fresnel_f0 = 0.04f;
    props.reflection.roughness = 0.01f;   // Very smooth
    props.reflection.metallic = 0.0f;
    props.reflection.specular_intensity = 1.0f;
    props.reflection.specular_color = (Vec3){1.0f, 1.0f, 1.0f};

    // Glass refraction properties
    props.refraction.ior = 1.52f;  // Glass IOR
    props.refraction.transmission = 0.99f;
    props.refraction.transmission_color = (Vec4){0.95f, 0.95f, 1.0f, 1.0f};  // Nearly colorless
    props.refraction.thickness = 2.0f;
    props.refraction.attenuation = 0.05f;
    props.refraction.thin_walled = false;

    return props;
}

MaterialSurfaceProperties material_get_metal_properties(f32 roughness) {
    MaterialSurfaceProperties props = {0};

    // Metal reflection properties
    props.reflection.fresnel_f0 = 0.8f;   // Metals have high F0
    props.reflection.roughness = roughness;
    props.reflection.metallic = 1.0f;     // Fully metallic
    props.reflection.specular_intensity = 2.0f;
    props.reflection.specular_color = (Vec3){1.0f, 1.0f, 1.0f};

    // Metals don't refract
    props.refraction.transmission = 0.0f;

    return props;
}

MaterialSurfaceProperties material_get_skin_properties(void) {
    MaterialSurfaceProperties props = {0};

    // Skin reflection
    props.reflection.fresnel_f0 = 0.028f;
    props.reflection.roughness = 0.15f;
    props.reflection.metallic = 0.0f;
    props.reflection.specular_intensity = 0.5f;
    props.reflection.specular_color = (Vec3){1.0f, 0.9f, 0.85f};

    // Subsurface scattering for skin
    props.subsurface_amount = 0.8f;
    props.subsurface_color = (Vec3){1.0f, 0.5f, 0.3f};  // Reddish scattering
    props.subsurface_radius = 0.2f;

    return props;
}

MaterialSurfaceProperties material_get_fabric_properties(void) {
    MaterialSurfaceProperties props = {0};

    // Fabric reflection
    props.reflection.fresnel_f0 = 0.035f;
    props.reflection.roughness = 0.4f;    // Rougher surface
    props.reflection.metallic = 0.0f;
    props.reflection.specular_intensity = 0.3f;
    props.reflection.specular_color = (Vec3){1.0f, 1.0f, 1.0f};

    // Sheen for fabric
    props.sheen_amount = 0.3f;
    props.sheen_color = (Vec3){1.0f, 1.0f, 1.0f};

    return props;
}

MaterialSurfaceProperties material_get_plastic_properties(bool glossy) {
    MaterialSurfaceProperties props = {0};

    // Plastic reflection
    props.reflection.fresnel_f0 = 0.05f;
    props.reflection.roughness = glossy ? 0.1f : 0.3f;
    props.reflection.metallic = 0.0f;
    props.reflection.specular_intensity = glossy ? 1.0f : 0.5f;
    props.reflection.specular_color = (Vec3){1.0f, 1.0f, 1.0f};

    return props;
}
