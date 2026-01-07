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
    
    return light;
}
