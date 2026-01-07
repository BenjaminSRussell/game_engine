/*
 * atmospheric_scattering.c
 * Atmospheric scattering integration
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "environment/atmosphere/core/atmosphere/sky/atmospheric_scattering.h"
#include "environment/atmosphere/core/atmosphere/sky/rayleigh_mie.h"
#include "include/math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ATMOSPHERE_PLANET_RADIUS 6360000.0f
#define ATMOSPHERE_LIMIT 6460000.0f // 100km atmosphere
#define ATMOSPHERE_DENSITY_FALLOFF_RAYLEIGH 8000.0f
#define ATMOSPHERE_DENSITY_FALLOFF_MIE 1200.0f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_parameters {
    float planet_radius;
    float atmosphere_limit;
    float rayleigh_height;
    float mie_height;
    
    vec3_t sun_direction;
    float sun_intensity;
    
    // Coefficients
    vec3_t beta_rayleigh;
    vec3_t beta_mie;
} atmosphere_parameters_t;

static atmosphere_parameters_t g_atmosphere_params;
static bool g_atmosphere_initialized = false;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float get_density_ratio(float height, float scale_height) {
    return expf(-height / scale_height);
}

// Intersect ray with sphere
// Returns distance to intersection or -1 if no intersection
static float ray_sphere_intersect(vec3_t ray_origin, vec3_t ray_dir, float sphere_radius) {
    // Simple ray-sphere intersection, assumign sphere at (0,0,0) usually
    // |o + d*t|^2 = r^2
    float a = vec3_dot(ray_dir, ray_dir);
    float b = 2.0f * vec3_dot(ray_origin, ray_dir);
    float c = vec3_dot(ray_origin, ray_origin) - sphere_radius * sphere_radius;
    
    float d = b*b - 4.0f*a*c;
    if (d < 0.0f) return -1.0f;
    
    d = sqrtf(d);
    float t1 = (-b - d) / (2.0f * a);
    float t2 = (-b + d) / (2.0f * a);
    
    if (t1 >= 0.0f) return t1;
    if (t2 >= 0.0f) return t2;
    return -1.0f;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_atmospheric_scattering_init(void) {
    if (g_atmosphere_initialized) return 0;
    
    g_atmosphere_params.planet_radius = ATMOSPHERE_PLANET_RADIUS;
    g_atmosphere_params.atmosphere_limit = ATMOSPHERE_LIMIT;
    g_atmosphere_params.rayleigh_height = ATMOSPHERE_DENSITY_FALLOFF_RAYLEIGH;
    g_atmosphere_params.mie_height = ATMOSPHERE_DENSITY_FALLOFF_MIE;
    g_atmosphere_params.sun_intensity = 1.0f;
    g_atmosphere_params.sun_direction = vec3_normalize(vec3_set(0.0f, 1.0f, 0.5f));
    
    // Sea level scattering coefficients
    g_atmosphere_params.beta_rayleigh = vec3_set(5.8e-6f, 13.5e-6f, 33.1e-6f);
    g_atmosphere_params.beta_mie = vec3_set(21.0e-6f, 21.0e-6f, 21.0e-6f);
    
    g_atmosphere_initialized = true;
    return 0;
}

void atmosphere_atmospheric_scattering_shutdown(void) {
    g_atmosphere_initialized = false;
}

// Integrate optical depth along a ray
// Simplified version for CPU-side reference/testing
float atmosphere_integrate_optical_depth(vec3_t start, vec3_t end) {
    int samples = 10;
    float step = 1.0f / samples;
    float optical_depth = 0.0f;
    
    vec3_t delta = vec3_sub(end, start);
    float len = sqrtf(vec3_dot(delta, delta));
    
    for (int i = 0; i < samples; i++) {
        float t = (float)i * step;
        vec3_t pos = vec3_add(start, (vec3_t){.x=delta.x*t, .y=delta.y*t, .z=delta.z*t});
        
        // Height above surface (assuming planet center 0, -Radius, 0 relative to camera 0 if on ground)
        // Here assuming positions are relative to planet center
        float r = sqrtf(vec3_dot(pos, pos));
        float h = r - g_atmosphere_params.planet_radius;
        if (h < 0) h = 0;
        
        optical_depth += get_density_ratio(h, g_atmosphere_params.rayleigh_height) * (len / samples);
    }
    
    return optical_depth;
}

void atmosphere_set_sun_direction(vec3_t direction) {
    g_atmosphere_params.sun_direction = vec3_normalize(direction);
}
