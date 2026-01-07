/*
 * cloud_density.c
 * Cloud density field generation
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "cloud_density.h"
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

#define CLOUD_LAYER_START 1500.0f
#define CLOUD_LAYER_THICKNESS 4000.0f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloud_density_params {
    float global_coverage;
    float global_density;
    
    // Scale factors for noise
    float scale_base;
    float scale_detail;
    
    vec3_t wind_offset;
    
    // Height gradient
    float height_start;
    float height_end;
} cloud_density_params_t;

static cloud_density_params_t g_density_params = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Simple pseudo-noise helper (placeholder for actual 3D texture sample)
static float sample_noise_3d(vec3_t p, float scale) {
    // In a real engine, this samples a 3D texture containing Perlin-Worley noise
    // Here we simulate a basic structural noise
    float v = sinf(p.x * scale) * cosf(p.z * scale) + sinf(p.y * scale * 0.5f);
    return v * 0.5f + 0.5f;
}

static float remap(float v, float low1, float high1, float low2, float high2) {
    return low2 + (v - low1) * (high2 - low2) / (high1 - low1);
}

static float get_height_fraction(vec3_t pos) {
    float height = pos.y - g_density_params.height_start;
    return height / (g_density_params.height_end - g_density_params.height_start);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_cloud_density_init(void) {
    g_density_params.global_coverage = 0.5f;
    g_density_params.global_density = 1.0f;
    g_density_params.scale_base = 0.001f;
    g_density_params.scale_detail = 0.01f;
    g_density_params.height_start = CLOUD_LAYER_START;
    g_density_params.height_end = CLOUD_LAYER_START + CLOUD_LAYER_THICKNESS;
    
    return 0;
}

void atmosphere_cloud_density_shutdown(void) {
    // Clean up
}

void atmosphere_cloud_density_update(float time, float wind_speed) {
    g_density_params.wind_offset.x += wind_speed * 0.1f; // Simple X movement
}

// Calculate density at world position p
float atmosphere_cloud_density_sample(vec3_t p) {
    // Check height bounds
    if (p.y < g_density_params.height_start || p.y > g_density_params.height_end) {
        return 0.0f;
    }
    
    // Height gradient (clouds are denser in the middle usually)
    float h_fraction = get_height_fraction(p);
    float height_gradient = 1.0f - fabsf(h_fraction * 2.0f - 1.0f); // 0 at edges, 1 in center
    
    // Base shape noise (Perlin-Worley)
    vec3_t sample_pos = vec3_add(p, g_density_params.wind_offset);
    float base_noise = sample_noise_3d(sample_pos, g_density_params.scale_base);
    
    // Erode with detail noise
    float detail_noise = sample_noise_3d(sample_pos, g_density_params.scale_detail);
    float fbm = base_noise - (detail_noise * 0.3f);
    
    // Apply coverage
    // If coverage is 0.5, we want values > 0.5 to be clouds
    // We remap noise so that (1-coverage) becomes 0
    
    float coverage_threshold = 1.0f - g_density_params.global_coverage;
    if (fbm < coverage_threshold) return 0.0f;
    
    float cloud_density = remap(fbm, coverage_threshold, 1.0f, 0.0f, 1.0f);
    
    // Apply height and global density multiplier
    return cloud_density * height_gradient * g_density_params.global_density;
}

void atmosphere_cloud_density_set_coverage(float coverage) {
    g_density_params.global_coverage = coverage;
}
