#include "lighting/sources/sky_light.h"
#include <string.h>

SkyLight sky_light_create(uint32_t cubemap_id,
                          Vec3 tint_color,
                          float intensity,
                          bool cast_shadows) {
    SkyLight light;
    memset(&light, 0, sizeof(SkyLight));

    light.base.type = LIGHT_TYPE_SKY;
    light.base.enabled = true;
    light.base.cast_shadows = cast_shadows;
    light.base.intensity = intensity;

    // Base color isn't used much for sky light but we set it anyway
    light.base.color = tint_color;

    light.cubemap_texture_id = cubemap_id;
    light.tint_color = tint_color;
    light.rotation = 0.0f;
    light.contrast = 1.0f;
    light.real_time_capture = false;

    return light;
}

void sky_light_set_rotation(SkyLight* light, float rotation) {
    if (light) {
        light->rotation = rotation;
    }
}

void sky_light_set_contrast(SkyLight* light, float contrast) {
    if (light) {
        light->contrast = contrast;
    }
}
