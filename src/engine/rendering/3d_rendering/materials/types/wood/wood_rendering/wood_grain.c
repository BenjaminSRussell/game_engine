#include "../3d_rendering.h"
#include <math.h>

/* ==================== Wood Grain Rendering ==================== */

// Math helpers
static float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float mixf(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

// Procedural 3D noise placeholder
// In a real engine this would sample a 3D noise texture or use a robust simplex noise implementation
static float noise3d(float x, float y, float z) {
    // Very simple pseudo-noise for compilation validity
    float n = sinf(x * 12.9898 + y * 78.233 + z * 54.53);
    return fractf(n * 43758.5453) * 2.0f - 1.0f; // Range -1 to 1
}

// Helper for fract since undefined in standard C math without gnu extensions sometimes
static float fractf(float x) {
    return x - floorf(x);
}

typedef struct {
    float grain_scale;
    float grain_contrast;
    float ring_scale;
    float ring_distortion;
    Vec3 wood_color_light;
    Vec3 wood_color_dark;
    Vec3 grain_direction; // Local axis of the trunk
} wood_material_t;

// Returns diffuse color and modifies roughness/normal based on grain
void shade_wood_grain(Vec3 position_local, wood_material_t* params, 
                     Vec3* out_albedo, float* out_roughness) {
    if (!params) return;

    // Project position onto the plane perpendicular to grain direction (assuming grain is Y-up for simplification)
    // r = distance from center (rings)
    float r = sqrtf(position_local.x * position_local.x + position_local.z * position_local.z);
    
    // Add noise to radius for distortion
    float noise = noise3d(position_local.x * params->grain_scale, 
                         position_local.y * params->grain_scale, 
                         position_local.z * params->grain_scale);
    
    float distorted_r = r * params->ring_scale + noise * params->ring_distortion;
    
    // Generate rings pattern: sine wave of radius
    float rings = sinf(distorted_r * 20.0f); // 20.0f is arbitrary ring frequency
    rings = rings * 0.5f + 0.5f; // Norm to 0..1
    
    // Sharpen rings for wood look
    rings = powf(rings, params->grain_contrast);
    
    // Mix colors
    out_albedo->x = mixf(params->wood_color_dark.x, params->wood_color_light.x, rings);
    out_albedo->y = mixf(params->wood_color_dark.y, params->wood_color_light.y, rings);
    out_albedo->z = mixf(params->wood_color_dark.z, params->wood_color_light.z, rings);
    
    // Modulate roughness: darker rings are usually more porous/rougher
    // Lighter wood (summer growth) is softer/smoother or vice versa depending on wood type
    // Let's say dark rings (winter growth) are denser/smoother in some woods, but often rougher in shading due to pores.
    // We'll modulate base roughness.
    *out_roughness = mixf(*out_roughness + 0.1f, *out_roughness, rings);
}
