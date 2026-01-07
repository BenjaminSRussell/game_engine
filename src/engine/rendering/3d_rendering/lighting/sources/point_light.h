#ifndef LIGHTING_POINT_LIGHT_H
#define LIGHTING_POINT_LIGHT_H

#include "light_culling.h"
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

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_POINT_LIGHT_H
