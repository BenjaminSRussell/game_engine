#include "lighting/sources/spot_light.h"

light_t spot_light_create(simd_float3 position,
                         simd_float3 direction,
                         simd_float3 color,
                         float intensity,
                         float radius,
                         float inner_cone,
                         float outer_cone,
                         bool cast_shadows) {
    light_t light = {0};
    light.position = position;
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.inner_cone = inner_cone;
    light.outer_cone = outer_cone;
    light.cast_shadows = cast_shadows;
    light.type = LIGHT_TYPE_SPOT;
    
    light.shadow_map_index = 0;
    light.ies_texture_id = 0;
    light.source_radius = 0.0f;
    light.source_length = 0.0f;
    
    return light;
}

void spot_light_set_ies_profile(light_t* light, uint32_t ies_texture_id) {
    if (light) {
        light->ies_texture_id = ies_texture_id;
    }
}

void spot_light_set_source_radius(light_t* light, float source_radius) {
    if (light) {
        light->source_radius = source_radius;
    }
}
