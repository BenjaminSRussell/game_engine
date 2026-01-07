#include "render_config.h"

void render_config_init_defaults(render_config_t* config) {
    if (!config) return;

    // Default to High Quality
    config->shadow_quality = 3;
    config->lighting_quality = 2;
    config->texture_quality = 3;
    config->particles_quality = 2;

    config->enable_shadows = true;
    config->enable_bloom = true;
    config->enable_taa = true;
    config->enable_ssao = true;
    config->enable_ssr = true;
    config->enable_volumetric_fog = true;
    config->enable_motion_blur = true;

    config->debug_draw_lights = false;
    config->debug_draw_physics = false;
    config->debug_draw_bounds = false;
    config->debug_wireframe = false;

    config->render_scale = 1.0f;
}
