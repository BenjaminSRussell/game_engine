#include "directional_light.h"

light_t directional_light_create(simd_float3 direction,
                                simd_float3 color,
                                float intensity,
                                bool cast_shadows) {
    light_t light = {0};
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.cast_shadows = cast_shadows;
    light.type = LIGHT_TYPE_DIRECTIONAL;
    
    // Defaults for unused fields
    light.position = (simd_float3){0, 0, 0};
    light.radius = 10000.0f; // Effectively infinite
    light.inner_cone = 0.0f;
    light.outer_cone = 0.0f;
    light.shadow_map_index = 0;
    
    return light;
}
