/*
 * sun_disk.c
 * Sun disk rendering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "sun_disk.h"
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

#define SUN_ANGULAR_RADIUS 0.00465f // ~0.266 degrees
#define PI 3.14159265359f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct sun_disk_params {
    vec3_t direction;
    vec3_t color;
    float intensity;
    float angular_size;
    bool enable_bloom;
    float bloom_intensity;
} sun_disk_params_t;

static sun_disk_params_t g_sun_params = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_sun_disk_init(void) {
    g_sun_params.direction = vec3_set(0.0f, 1.0f, 0.0f);
    g_sun_params.color = vec3_set(1.0f, 1.0f, 0.9f);
    g_sun_params.intensity = 10.0f;
    g_sun_params.angular_size = SUN_ANGULAR_RADIUS;
    g_sun_params.enable_bloom = true;
    g_sun_params.bloom_intensity = 2.0f;
    return 0;
}

void atmosphere_sun_disk_shutdown(void) {
    // Cleanup
}

void atmosphere_sun_disk_set_direction(vec3_t dir) {
    g_sun_params.direction = vec3_normalize(dir);
}

// Compute sun radiance for a given view direction
// Returns high dynamic range color
vec3_t atmosphere_sun_disk_evaluate(vec3_t view_dir) {
    float cos_theta = vec3_dot(view_dir, g_sun_params.direction);
    
    // Smoothstep for anti-aliased edge
    // we want 1.0 inside disk, 0.0 outside
    // cos(angle) ~ 1 - angle^2/2 for small angles
    
    float sun_cos = cosf(g_sun_params.angular_size);
    
    if (cos_theta >= sun_cos) {
        // Inside sun disk
        // Limb darkening could be applied here:
        // L = L0 * (1 - u * (1 - cos(angle_from_center)))
        
        float base_intensity = g_sun_params.intensity;
        
        // Simple bloom halo simulation around the core
        // This usually is post-process, but here we can add a glow factor near edge
        
        vec3_t result;
        result.x = g_sun_params.color.x * base_intensity;
        result.y = g_sun_params.color.y * base_intensity;
        result.z = g_sun_params.color.z * base_intensity;
        return result;
    } 
    else if (g_sun_params.enable_bloom) {
        // Fake bloom/glare outside the disk
        float bloom_falloff = 200.0f;
        float diff = fabsf(cos_theta - sun_cos); // rough approximation
        float halo = expf(-diff * bloom_falloff) * g_sun_params.bloom_intensity;
        
        if (halo > 0.001f) {
            vec3_t result;
            result.x = g_sun_params.color.x * halo;
            result.y = g_sun_params.color.y * halo;
            result.z = g_sun_params.color.z * halo;
            return result;
        }
    }
    
    return vec3_zero();
}
