#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct render_config {
    // Quality Settings
    uint32_t shadow_quality;      // 0: Off, 1: Low, 2: Medium, 3: High, 4: Ultra
    uint32_t lighting_quality;    // 0-3
    uint32_t texture_quality;     // 0-3
    uint32_t particles_quality;   // 0-3
    
    // Feature Toggles
    bool enable_shadows;
    bool enable_bloom;
    bool enable_taa;
    bool enable_ssao;
    bool enable_ssr;
    bool enable_volumetric_fog;
    bool enable_motion_blur;
    
    // Debug
    bool debug_draw_lights;
    bool debug_draw_physics;
    bool debug_draw_bounds;
    bool debug_wireframe;
    
    // Resolution Scaling
    float render_scale;           // 0.5 - 2.0
    
} render_config_t;

// Defaults
void render_config_init_defaults(render_config_t* config);

#endif // RENDER_CONFIG_H
