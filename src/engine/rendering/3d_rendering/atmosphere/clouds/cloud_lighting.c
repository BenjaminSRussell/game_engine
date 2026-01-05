/*
 * cloud_lighting.c
 * Cloud lighting and scattering models
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "cloud_lighting.h"
#include "../../math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define PI 3.14159265359f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloud_lighting_params {
    vec3_t sun_color;
    float sun_intensity;
    vec3_t ambient_color_top;
    vec3_t ambient_color_bottom;
    
    // Phase function
    float eccentricity; // First phase lobe g
    float silver_lining_spread; // Second lobe g
    float silver_lining_intensity;
    
    // Powder effect
    float powder_scale;
    
    // Attenuation
    float absorption_coeff;
} cloud_lighting_params_t;

static cloud_lighting_params_t g_cloud_light = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Dual-lobe Henyey-Greenstein phase function
static float hg_phase(float g, float cos_theta) {
    float g2 = g * g;
    float num = 1.0f - g2;
    float denom = 1.0f + g2 - 2.0f * g * cos_theta;
    return (1.0f / (4.0f * PI)) * (num / powf(denom, 1.5f));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_cloud_lighting_init(void) {
    g_cloud_light.sun_color = vec3_set(1.0f, 0.98f, 0.95f);
    g_cloud_light.sun_intensity = 1.0f;
    g_cloud_light.ambient_color_top = vec3_set(0.6f, 0.7f, 0.9f);
    g_cloud_light.ambient_color_bottom = vec3_set(0.2f, 0.25f, 0.3f);
    
    g_cloud_light.eccentricity = 0.6f; // Forward scattering
    g_cloud_light.silver_lining_spread = -0.2f; // Back scattering lobe
    g_cloud_light.silver_lining_intensity = 0.5f;
    
    g_cloud_light.powder_scale = 1.0f;
    g_cloud_light.absorption_coeff = 0.5f; // Beers law scaling
    
    return 0;
}

void atmosphere_cloud_lighting_shutdown(void) {
    // Cleanup
}

// Calculate lighting at a point within the cloud
// density: current sampled density
// optical_depth_to_sun: accumulated density towards light
vec3_t atmosphere_cloud_lighting_compute(
    float density, 
    float optical_depth_to_sun, 
    float height_fraction,
    vec3_t view_dir, 
    vec3_t sun_dir
) {
    float cos_theta = vec3_dot(view_dir, sun_dir);
    
    // 1. Direct Lighting (Beer's Law)
    // T = exp(-optical_depth * coeff)
    float transmittance = expf(-optical_depth_to_sun * g_cloud_light.absorption_coeff);
    
    // 2. Powder Effect (Beer's Powder)
    // Clouds are darker at edges where density is lower but also scatter more?
    // Powder effect approximates multiple scattering making edges darker/more contrasty
    // F = 1 - exp(-depth * 2)
    float powder = 1.0f - expf(-optical_depth_to_sun * 2.0f * g_cloud_light.powder_scale);
    float light_energy = transmittance * powder; 
    
    // 3. Phase Function
    float phase1 = hg_phase(g_cloud_light.eccentricity, cos_theta);
    float phase2 = hg_phase(g_cloud_light.silver_lining_spread, cos_theta);
    float phase = phase1 + phase2 * g_cloud_light.silver_lining_intensity;
    
    // Direct contribution
    vec3_t direct_light;
    direct_light.x = g_cloud_light.sun_color.x * g_cloud_light.sun_intensity * light_energy * phase;
    direct_light.y = g_cloud_light.sun_color.y * g_cloud_light.sun_intensity * light_energy * phase;
    direct_light.z = g_cloud_light.sun_color.z * g_cloud_light.sun_intensity * light_energy * phase;
    
    // 4. Ambient Lighting
    // Gradient based on height
    vec3_t ambient;
    float amb_factor = 0.6f + 0.4f * height_fraction;
    // Blend top/bottom
    ambient.x = g_cloud_light.ambient_color_bottom.x * (1.0f - height_fraction) + g_cloud_light.ambient_color_top.x * height_fraction;
    ambient.y = g_cloud_light.ambient_color_bottom.y * (1.0f - height_fraction) + g_cloud_light.ambient_color_top.y * height_fraction;
    ambient.z = g_cloud_light.ambient_color_bottom.z * (1.0f - height_fraction) + g_cloud_light.ambient_color_top.z * height_fraction;
    
    // Ambient is less affected by directional transmittance but still absorbed by density
    float ambient_transmittance = expf(-density * 0.5f);
    
    vec3_t result;
    result.x = (direct_light.x + ambient.x * ambient_transmittance) * density;
    result.y = (direct_light.y + ambient.y * ambient_transmittance) * density;
    result.z = (direct_light.z + ambient.z * ambient_transmittance) * density;
    
    return result;
}

void atmosphere_cloud_lighting_set_sun(vec3_t color, float intensity) {
    g_cloud_light.sun_color = color;
    g_cloud_light.sun_intensity = intensity;
}
