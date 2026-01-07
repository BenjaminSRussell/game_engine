/*
 * volumetric_clouds.c
 * Main volume cloud rendering system
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#include "environment/atmosphere/core/atmosphere/clouds/volumetric_clouds.h"
#include "environment/atmosphere/core/atmosphere/clouds/cloud_density.h"
#include "environment/atmosphere/core/atmosphere/clouds/cloud_lighting.h"
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

#define MAX_CLOUD_STEPS 64
#define MAX_LIGHT_STEPS 6
#define CLOUD_STEP_SIZE 150.0f
#define PLANET_RADIUS 6360000.0f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct volumetric_cloud_context {
    int quality_level;
    bool enable_shadows;
    bool initialized;
    
    // Cache for optimization
    vec3_t sun_direction;
} volumetric_cloud_context_t;

static volumetric_cloud_context_t g_cloud_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void get_cloud_shell_intersections(
    vec3_t origin, 
    vec3_t dir, 
    float start_height, 
    float end_height, 
    float* dist_to_start, 
    float* dist_to_end
) {
    // Ray-Sphere intersection logic for cloud layer shells
    // Simple planar approximation for local area usually suffices, but spherical is better for horizon
    // Here using simple planar for implementation brevity if not spherical
    
    // Assuming flat earth for local cloud layer for now to save math complexity in this snippet
    // In full engine, use sphere intersection
    
    if (dir.y <= 0.0f) {
        *dist_to_start = -1.0f;
        *dist_to_end = -1.0f;
        return;
    }
    
    // dist = (target_y - origin_y) / dir_y
    *dist_to_start = (start_height - origin.y) / dir.y;
    *dist_to_end = (end_height - origin.y) / dir.y;
    
    if (*dist_to_start < 0.0f) *dist_to_start = 0.0f;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int atmosphere_volumetric_clouds_init(void) {
    if (g_cloud_ctx.initialized) return 0;
    
    atmosphere_cloud_density_init();
    atmosphere_cloud_lighting_init();
    
    g_cloud_ctx.quality_level = 1;
    g_cloud_ctx.enable_shadows = true;
    g_cloud_ctx.sun_direction = vec3_set(0.0f, 1.0f, 0.0f);
    
    g_cloud_ctx.initialized = true;
    return 0;
}

void atmosphere_volumetric_clouds_shutdown(void) {
    atmosphere_cloud_density_shutdown();
    atmosphere_cloud_lighting_shutdown();
    g_cloud_ctx.initialized = false;
}

// Main render function for clouds
// Returns accumulated color and alpha/transmittance
void atmosphere_volumetric_clouds_render(
    vec3_t cam_pos, 
    vec3_t view_dir, 
    vec3_t sun_dir, 
    vec3_t* out_color, 
    float* out_alpha
) {
    if (!g_cloud_ctx.initialized) {
        *out_color = vec3_zero();
        *out_alpha = 0.0f;
        return;
    }
    
    float start_height = 1500.0f; // Sync with density.c constants ideally via getter
    float end_height = 5500.0f;
    
    float dist_start, dist_end;
    get_cloud_shell_intersections(cam_pos, view_dir, start_height, end_height, &dist_start, &dist_end);
    
    if (dist_end < 0.0f || dist_start > dist_end) {
        *out_color = vec3_zero();
        *out_alpha = 0.0f;
        return;
    }
    
    vec3_t current_pos;
    // current_pos = cam_pos + view_dir * dist_start
    current_pos.x = cam_pos.x + view_dir.x * dist_start;
    current_pos.y = cam_pos.y + view_dir.y * dist_start;
    current_pos.z = cam_pos.z + view_dir.z * dist_start;
    
    float remaining_dist = dist_end - dist_start;
    int steps = MAX_CLOUD_STEPS;
    float step_size = remaining_dist / (float)steps;
    
    // Dynamic step size optimization is common
    
    vec3_t accum_color = vec3_zero();
    float transmittance = 1.0f;
    
    vec3_t step_vec;
    step_vec.x = view_dir.x * step_size;
    step_vec.y = view_dir.y * step_size;
    step_vec.z = view_dir.z * step_size;
    
    for (int i = 0; i < steps; i++) {
        if (transmittance < 0.01f) break;
        
        float density = atmosphere_cloud_density_sample(current_pos);
        
        if (density > 0.0f) {
            // Lighting MARCH towards sun
            float light_od = 0.0f;
            vec3_t light_pos = current_pos;
            // Simplified light march usually uses fewer steps and larger stride / cone step
            for (int j = 0; j < MAX_LIGHT_STEPS; j++) {
                vec3_t l_step = vec3_set(sun_dir.x * (step_size * 2.0f), sun_dir.y * (step_size * 2.0f), sun_dir.z * (step_size * 2.0f));
                light_pos = vec3_add(light_pos, l_step);
                
                // break if out of bounds (simplified check)
                if (light_pos.y > end_height) break;
                
                float l_density = atmosphere_cloud_density_sample(light_pos);
                light_od += l_density * (step_size * 2.0f);
            }
            
            float height_fraction = (current_pos.y - start_height) / (end_height - start_height);
            vec3_t light_res = atmosphere_cloud_lighting_compute(density, light_od, height_fraction, view_dir, sun_dir);
            
            // Accumulate
            // Beer's law for this step transm
            float step_transmittance = expf(-density * step_size * 0.1f); // 0.1 extinction coeff
            
            // color += light * transmittance * (1 - step_transmittance)
            // (Standard volume integration)
            float coverage = (1.0f - step_transmittance);
            
            accum_color.x += light_res.x * transmittance * coverage;
            accum_color.y += light_res.y * transmittance * coverage;
            accum_color.z += light_res.z * transmittance * coverage;
            
            transmittance *= step_transmittance;
        }
        
        current_pos = vec3_add(current_pos, step_vec);
    }
    
    *out_color = accum_color;
    *out_alpha = 1.0f - transmittance;
}

void atmosphere_volumetric_clouds_set_quality(int quality) {
    g_cloud_ctx.quality_level = quality;
}
