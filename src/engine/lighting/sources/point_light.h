#ifndef LIGHTING_POINT_LIGHT_H
#define LIGHTING_POINT_LIGHT_H

#include "lighting/sources/light_culling.h"
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new point light structure.
 * 
 * @param position World space position
 * @param color Light color (RGB)
 * @param intensity Light intensity
 * @param radius Influence radius
 * @param cast_shadows Whether this light casts shadows
 */
light_t point_light_create(simd_float3 position, 
                          simd_float3 color, 
                          float intensity, 
                          float radius, 
                          bool cast_shadows);

/**
 * Set IES profile for the point light.
 *
 * @param light Pointer to the light
 * @param ies_texture_id ID of the IES profile texture
 */
void point_light_set_ies_profile(light_t* light, uint32_t ies_texture_id);

/**
 * Set source radius for soft shadows/specular.
 *
 * @param light Pointer to the light
 * @param source_radius Radius of the light source geometry
 */
void point_light_set_source_radius(light_t* light, float source_radius);

/**
 * Set source length for capsule lights.
 *
 * @param light Pointer to the light
 * @param source_length Length of the light source geometry
 */
void point_light_set_source_length(light_t* light, float source_length);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_POINT_LIGHT_H
