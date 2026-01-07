#ifndef LIGHTING_SKY_LIGHT_H
#define LIGHTING_SKY_LIGHT_H

#include "lighting/light_types.h"
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new sky light structure.
 *
 * @param cubemap_id ID of the cubemap texture
 * @param tint_color Tint color (RGB)
 * @param intensity Light intensity
 * @param cast_shadows Whether this light affects volumetric shadowing (usually AO)
 */
SkyLight sky_light_create(uint32_t cubemap_id,
                          Vec3 tint_color,
                          float intensity,
                          bool cast_shadows);

/**
 * Set the rotation of the sky light.
 *
 * @param light Pointer to the sky light
 * @param rotation Rotation angle in degrees
 */
void sky_light_set_rotation(SkyLight* light, float rotation);

/**
 * Set the contrast of the sky light.
 *
 * @param light Pointer to the sky light
 * @param contrast Contrast value
 */
void sky_light_set_contrast(SkyLight* light, float contrast);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_SKY_LIGHT_H
