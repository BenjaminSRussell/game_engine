#ifndef LIGHT_TYPES_H
#define LIGHT_TYPES_H

#include "../include/math/vec3.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Light Types
 */
typedef enum {
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
    LIGHT_TYPE_AREA
} LightType;

/**
 * Common Light Parameters
 */
typedef struct {
    uint32_t id;
    LightType type;
    bool enabled;
    bool cast_shadows;
    
    Vec3 color;
    float intensity;
    
    // Shadow parameters
    float shadow_bias;
    float shadow_normal_bias;
    uint32_t shadow_map_resolution;
} LightBase;

/**
 * Directional Light
 * Represents a distant light source (sun, moon)
 */
typedef struct {
    LightBase base;
    Vec3 direction;
} DirectionalLight;

/**
 * Point Light
 * Omnidirectional light source
 */
typedef struct {
    LightBase base;
    Vec3 position;
    float range;
    
    // Attenuation coefficients: 1.0 / (c + l*d + q*d^2)
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
} PointLight;

/**
 * Spot Light
 * Conical light source
 */
typedef struct {
    LightBase base;
    Vec3 position;
    Vec3 direction;
    float range;
    
    float inner_cone_angle; // Radians
    float outer_cone_angle; // Radians
    
    // Attenuation
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
    
    uint32_t cookie_texture_id;
} SpotLight;

/**
 * Unified Light Structure
 * Useful for storage in a single array
 */
typedef struct {
    union {
        LightBase base;
        DirectionalLight directional;
        PointLight point;
        SpotLight spot;
    };
} Light;

// Helper to check if a light is a valid type
static inline bool light_is_valid(const Light* light) {
    return light && light->base.id != 0;
}

#endif // LIGHT_TYPES_H
