#ifndef LIGHTING_DIRECTIONAL_LIGHT_H
#define LIGHTING_DIRECTIONAL_LIGHT_H

#include "light_culling.h"
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new directional light structure.
 * 
 * @param direction Light direction (normalized)
 * @param color Light color (RGB)
 * @param intensity Light intensity
 * @param cast_shadows Whether this light casts shadows
 */
light_t directional_light_create(simd_float3 direction,
                                simd_float3 color,
                                float intensity,
                                bool cast_shadows);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_DIRECTIONAL_LIGHT_H
