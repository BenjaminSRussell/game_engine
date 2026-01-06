#include "../3d_rendering.h"
#include <math.h>

/* ==================== Glass Thickness Absorption ==================== */

typedef struct {
    Vec3 absorption_color;  // Color at 'absorption_distance' or unit density
    float absorption_density;
} glass_volume_t;

// Beer-Lambert Law: Intensity = exp(-density * distance)
// For colored glass: exp(-absorption_coefficient * distance) where coeff differs per channel

Vec3 shade_glass_absorption(Vec3 current_color, float dist_traveled_in_glass, glass_volume_t* params) {
    if (!params) return current_color;

    // We want the glass to look like 'absorption_color' as it gets thicker.
    // Actually typically we define "Absorption Coefficient" (sigma_a).
    // Transmittance = exp(-sigma_a * dist)
    
    // If params->absorption_color represents the transmittance at unit distance d=1:
    // T_r = C.r = exp(-sigma_r * 1) -> sigma_r = -ln(C.r)
    // Then T(d) = exp(-(-ln(C)*dist)) = C^dist
    
    // Let's use simpler direct density model from Beer's Law often used in games:
    // Absorb more if density is high.
    
    // If absorption_color is defining the "tint" that remains:
    // We absorb "white - tint"? No.
    // Usually: transmittance = expf(-absorption_vector * dist)
    // Where absorption_vector is property of material.
    
    // Let's assume params->absorption_color is the ABSORPTION COEFFICIENT directly.
    // (Higher values = darker/more colored).
    
    Vec3 transmittance;
    transmittance.x = expf(-params->absorption_color.x * params->absorption_density * dist_traveled_in_glass);
    transmittance.y = expf(-params->absorption_color.y * params->absorption_density * dist_traveled_in_glass);
    transmittance.z = expf(-params->absorption_color.z * params->absorption_density * dist_traveled_in_glass);
    
    // Multiply incoming light (current_color) by transmittance
    return (Vec3){
        current_color.x * transmittance.x,
        current_color.y * transmittance.y,
        current_color.z * transmittance.z
    };
}

// Alternative: Thickness based on NdotV approx for thin shells (windows)
float estimate_glass_thickness_thin_shell(float base_thickness, Vec3 N, Vec3 V) {
    // Path length through slab = thickness / dot(N, V_refracted)
    // Approximate with NdotV
    float NdotV = V.x * N.x + V.y * N.y + V.z * N.z;
    if (NdotV < 0.001f) NdotV = 0.001f;
    return base_thickness / NdotV;
}
