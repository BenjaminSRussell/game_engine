#include "../3d_rendering.h"
#include <math.h>

/* ==================== Concrete Crack Rendering ==================== */

typedef struct {
    float crack_width;
    float crack_depth;
    Vec3 crack_color;
} concrete_crack_params_t;

// Sample a distance field texture where 0..1 represents distance to crack center
// 0.5 = edge, >0.5 = outside, <0.5 = inside
static float sample_crack_sdf(ResourceHandle mask_map, Vec2 uv) {
    return 1.0f; // Placeholder: returns "far away" from crack
}

// Modify Normal and Albedo based on cracks
void apply_concrete_cracks(Vec3* albedo, Vec3* normal, Vec2 uv, 
                          ResourceHandle crack_map, concrete_crack_params_t* params) {
    if (!params) return;

    float sdf = sample_crack_sdf(crack_map, uv);
    
    // Crack logic: SDF
    // If sdf < width, we are in crack
    // Smoothstep for anti-aliasing edge
    
    float edge = params->crack_width;
    float smoothness = 0.01f; // hardness of edge
    
    // alpha = 1.0 inside crack, 0.0 outside
    // using smoothstep(edge, edge - smooth, sdf) for "1 if small distance"
    // Assuming SDF: 0 is center, 1 is far.
    float alpha = 0.0f;
    if (sdf < edge) {
        float t = (edge - sdf) / smoothness;
        if (t > 1.0f) t = 1.0f;
        alpha = t;
    }
    
    // Apply crack color
    albedo->x = albedo->x * (1.0f - alpha) + params->crack_color.x * alpha;
    albedo->y = albedo->y * (1.0f - alpha) + params->crack_color.y * alpha;
    albedo->z = albedo->z * (1.0f - alpha) + params->crack_color.z * alpha;
    
    // Apply Normal perturbation for depth illusion
    // If we are near the edge (sdf approx edge), slope the normal inward
    // Simple bump mapping derivation from SDF gradient
    
    if (alpha > 0.0f && alpha < 1.0f) {
        // We are at the edge
        // Fake a normal pointing towards center
        // In real SDF we'd use ddx/ddy of SDF to get direction
        // Here we just perturb Z
        // Flatten normal inside crack? Or make it chaotic
        
        // Perturb normal to look like a cavity
        // Simple hack: Scale down Z
        normal->z *= 0.5f; 
        
        // Normalize
        float len = sqrtf(normal->x*normal->x + normal->y*normal->y + normal->z*normal->z);
        if (len > 0) {
            normal->x /= len; normal->y /= len; normal->z /= len;
        }
    }
}
