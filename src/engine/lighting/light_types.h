#ifndef LIGHT_TYPES_H
#define LIGHT_TYPES_H

#include "../include/math/vec3.h"
#include <simd/simd.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Light Types
 */
typedef enum {
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
    LIGHT_TYPE_AREA,
    LIGHT_TYPE_SKY
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

    // IES Light Profile
    uint32_t ies_texture_id;
    bool use_ies_profile;
    float source_radius; // For soft shadows
    float source_length; // For capsule light shape
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
    
    // IES Light Profile
    uint32_t ies_texture_id;
    bool use_ies_profile;
    float source_radius; // For soft shadows

    uint32_t cookie_texture_id;
} SpotLight;

/**
 * Sky Light
 * Ambient light source capturing the distant environment
 */
typedef struct {
    LightBase base;

    // Environment map texture
    uint32_t cubemap_texture_id;

    // Color of the sky light (modulates the cubemap)
    Vec3 tint_color;

    // Rotation of the sky light
    float rotation;

    // Contrast adjustment
    float contrast;

    // Whether to capture the scene dynamically
    bool real_time_capture;
} SkyLight;

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
        SkyLight sky;
    };
} Light;

// Helper to check if a light is a valid type
static inline bool light_is_valid(const Light* light) {
    return light && light->base.id != 0;
}

#endif // LIGHT_TYPES_H
