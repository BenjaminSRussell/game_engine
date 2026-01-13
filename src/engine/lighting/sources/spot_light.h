#ifndef LIGHTING_SPOT_LIGHT_H
#define LIGHTING_SPOT_LIGHT_H

#include "lighting/sources/light_culling.h"
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new spot light structure.
 * 
 * @param position World space position
 * @param direction Spot light direction (normalized)
 * @param color Light color (RGB)
 * @param intensity Light intensity
 * @param radius Influence radius
 * @param inner_cone Inner cone angle in radians
 * @param outer_cone Outer cone angle in radians
 * @param cast_shadows Whether this light casts shadows
 */
light_t spot_light_create(simd_float3 position,
                         simd_float3 direction,
                         simd_float3 color,
                         float intensity,
                         float radius,
                         float inner_cone,
                         float outer_cone,
                         bool cast_shadows);

/**
 * Set IES profile for the spot light.
 *
 * @param light Pointer to the light
 * @param ies_texture_id ID of the IES profile texture
 */
void spot_light_set_ies_profile(light_t* light, uint32_t ies_texture_id);

/**
 * Set source radius for soft shadows/specular.
 *
 * @param light Pointer to the light
 * @param source_radius Radius of the light source geometry
 */
void spot_light_set_source_radius(light_t* light, float source_radius);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_SPOT_LIGHT_H
