/**
 * =================================================================================================
 *                                 LIGHT EVALUATION SHADER
 * =================================================================================================
 *
 * Purpose: Implements evaluation for different light types (Directional, Point, Spot)
 * including physical attenuation models.
 */

#include <metal_stdlib>
using namespace metal;

/* =================================================================================================
 *                                    STRUCTURES
 * =================================================================================================
 */

struct Light {
    float3 position;
    float radius;
    float3 direction;
    float inner_cone;
    float3 color;
    float outer_cone;
    float intensity;
    uint type;              // 0=directional, 1=point, 2=spot
    uint cast_shadows;
    uint shadow_map_index;
    uint padding;
};

/* =================================================================================================
 *                                    ATTENUATION MODELS
 * =================================================================================================
 */

/**
 * Physical inverse-square attenuation with a windowing function to ensure
 * light intensity reaches zero at the influence radius.
 */
float apply_light_attenuation(float distance, float radius) {
    // Inverse square falloff
    float attenuation = 1.0 / max(distance * distance, 0.0001);
    
    // Windowing function (Karis 2013)
    float factor = distance / radius;
    float factor2 = factor * factor;
    float factor4 = factor2 * factor2;
    float windowing = saturate(1.0 - factor4);
    windowing = windowing * windowing;
    
    return attenuation * windowing;
}

/**
 * Soft spotlight cone attenuation.
 */
float apply_spot_attenuation(float3 L, float3 light_dir, float inner_cone, float outer_cone) {
    float cos_theta = dot(L, -light_dir);
    float cos_inner = cos(inner_cone);
    float cos_outer = cos(outer_cone);
    
    // Smooth transition between inner and outer cones
    float attenuation = saturate((cos_theta - cos_outer) / (cos_inner - cos_outer));
    return attenuation * attenuation; // Squared for smoother falloff
}

/* =================================================================================================
 *                                    LIGHT EVALUATION
 * =================================================================================================
 */

/**
 * Evaluates light properties for a given light source and surface position.
 * Returns light direction (L), intensity at surface (radiance), and distance.
 */
void evaluate_light(
    Light light, 
    float3 world_pos, 
    out float3 L, 
    out float3 radiance, 
    out float dist) 
{
    if (light.type == 0) {
        // Directional Light
        L = normalize(-light.direction);
        radiance = light.color * light.intensity;
        dist = 1e6; // Effectively infinite
    } 
    else if (light.type == 1) {
        // Point Light
        float3 to_light = light.position - world_pos;
        dist = length(to_light);
        L = to_light / max(dist, 0.0001);
        
        float attenuation = apply_light_attenuation(dist, light.radius);
        radiance = light.color * light.intensity * attenuation;
    } 
    else if (light.type == 2) {
        // Spot Light
        float3 to_light = light.position - world_pos;
        dist = length(to_light);
        L = to_light / max(dist, 0.0001);
        
        float dist_attenuation = apply_light_attenuation(dist, light.radius);
        float spot_attenuation = apply_spot_attenuation(L, light.direction, light.inner_cone, light.outer_cone);
        
        radiance = light.color * light.intensity * dist_attenuation * spot_attenuation;
    }
}
