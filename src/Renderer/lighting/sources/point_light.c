#include "lighting/sources/point_light.h"

light_t point_light_create(simd_float3 position, 
                          simd_float3 color, 
                          float intensity, 
                          float radius, 
                          bool cast_shadows) {
    light_t light = {0};
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.cast_shadows = cast_shadows;
    light.type = LIGHT_TYPE_POINT;
    
    // Initialize defaults for fields not used by point lights
    light.direction = (simd_float3){0, -1, 0};
    light.inner_cone = 0.0f;
    light.outer_cone = 0.0f;
    light.shadow_map_index = 0; // Will be assigned by shadow system often
    light.ies_texture_id = 0;
    light.source_radius = 0.0f;
    light.source_length = 0.0f;
    
    return light;
}

void point_light_set_ies_profile(light_t* light, uint32_t ies_texture_id) {
    if (light) {
        light->ies_texture_id = ies_texture_id;
    }
}

void point_light_set_source_radius(light_t* light, float source_radius) {
    if (light) {
        light->source_radius = source_radius;
    }
}

void point_light_set_source_length(light_t* light, float source_length) {
    if (light) {
        light->source_length = source_length;
    }
}
