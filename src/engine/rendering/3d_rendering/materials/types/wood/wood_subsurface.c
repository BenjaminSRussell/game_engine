#include "../3d_rendering.h"
#include <math.h>

/* ==================== Wood Subsurface Scattering ==================== */

// Wood SSS doesn't need to be as complex as skin.
// It mostly manifests as a softening of shadows and a "glow" on thin edges or backlighting.

static float maxf(float a, float b) {
    return a > b ? a : b;
}

static float powf_wrapper(float x, float y) {
    return powf(x, y);
}

typedef struct {
    float sss_strength;
    float sss_radius;
    Vec3 sss_color;
} wood_sss_t;

// Simple wrapped diffuse lighting to simulate subsurface scattering
// "Wrap shading"
Vec3 shade_wood_sss(Vec3 N, Vec3 L, Vec3 light_color, wood_sss_t* params) {
    if (!params) return (Vec3){0, 0, 0};

    float dotNL = N.x * L.x + N.y * L.y + N.z * L.z;
    
    // Wrap value relative to SSS radius/strength
    // 0.0 = hard Lambert, 1.0 = fully wrapped
    float wrap = params->sss_radius; // approximate usage
    
    // Calculate wrapped cosine
    float diff = maxf((dotNL + wrap) / (1.0f + wrap), 0.0f);
    
    // Apply strength and color
    Vec3 result;
    result.x = light_color.x * diff * params->sss_color.x * params->sss_strength;
    result.y = light_color.y * diff * params->sss_color.y * params->sss_strength;
    result.z = light_color.z * diff * params->sss_color.z * params->sss_strength;
    
    return result;
}

// Backlight transmission for thin wood objects
Vec3 shade_wood_transmission(Vec3 N, Vec3 V, Vec3 L, float thickness, wood_sss_t* params) {
    // Invert view for backlight check
    // If L is roughly opposite to V, we see transmission
    Vec3 H = { -(V.x + L.x), -(V.y + L.y), -(V.z + L.z) }; // Hacky, usually H is V+L
    // More accurate: Transmission depends on View and -Light.
    
    // Simple translucent distortion
    // phase function
    
    Vec3 L_inv = {-L.x, -L.y, -L.z};
    float dotVLi = V.x * L_inv.x + V.y * L_inv.y + V.z * L_inv.z;
    float transmission = maxf(dotVLi, 0.0f);
    
    // Power for falloff
    transmission = powf_wrapper(transmission, 4.0f); // Power factor for scattering phase
    
    // Beer's law for thickness absorption
    float attenuation = expf(-thickness * 2.0f); // 2.0 arbitrary density coefficient
    
    float total = transmission * attenuation * params->sss_strength;
    
    return (Vec3){
        params->sss_color.x * total,
        params->sss_color.y * total,
        params->sss_color.z * total
    };
}
