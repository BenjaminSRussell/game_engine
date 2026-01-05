#include "fire_effects.h"

// Blending configurations
#define BLEND_ADDITIVE 1
#define BLEND_ALPHA 2

// Helper function declarations (would be in full particle system header)
extern void render_particles(ParticleSystem* system, int blend_mode);

static Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    Vec3 result;
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    return result;
}

void apply_fire_shading(FireEffect* fire) {
    if (!fire) return;

    // In a real implementation, this would update GPU buffers or shaders
    // based on the fire's color properties and intensity.
    
    // Example: Modulate particle colors based on life
    // fire->flames->color_start = fire->base_color;
    // fire->flames->color_end = fire->tip_color;
}

void render_fire(FireEffect* fire, const Mat4* camera_view, const Mat4* camera_proj) {
    if (!fire) return;

    // 1. Render flame particles with additive blending
    apply_fire_shading(fire);
    if (fire->flames) {
        render_particles(fire->flames, BLEND_ADDITIVE);
    }

    // 2. Render embers
    if (fire->embers) {
        render_particles(fire->embers, BLEND_ADDITIVE);
    }

    // 3. Apply heat distortion
    apply_heat_distortion(fire->position, fire->intensity);
}
