/**
 * =================================================================================================
 *                                 PBR BRDF SHADER LIBRARY
 * =================================================================================================
 *
 * Purpose: Implements the Cook-Torrance BRDF functions (GGX D, Smith G, Schlick F).
 */

#include <metal_stdlib>
using namespace metal;

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* =================================================================================================
 *                                    GGX DISTRIBUTION (D)
 * =================================================================================================
 */

/**
 * Trowbridge-Reitz GGX Normal Distribution Function.
 * Approximates higher specular peaks and longer tails for rough surfaces.
 */
float distribution_ggx(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / max(denom, 0.0001);
}

/* =================================================================================================
 *                                    SMITH GEOMETRY (G)
 * =================================================================================================
 */

/**
 * Schlick-GGX approximation for the geometry function.
 */
float geometry_schlick_ggx(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0001);
}

/**
 * Smith's method for combining shadowing/masking.
 */
float geometry_smith(float NdotV, float NdotL, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float ggx1 = geometry_schlick_ggx(NdotV, k);
    float ggx2 = geometry_schlick_ggx(NdotL, k);
    
    return ggx1 * ggx2;
}

/**
 * Height-correlated Smith G2 for better energy conservation.
 */
float geometry_smith_correlated(float NdotV, float NdotL, float alpha) {
    float a2 = alpha * alpha;
    float lambdaV = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float lambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    
    return 0.5 / max(lambdaV + lambdaL, 0.0001);
}

/* =================================================================================================
 *                                    SCHLICK FRESNEL (F)
 * =================================================================================================
 */

/**
 * Schlick approximation for the Fresnel effect.
 * Determines the ratio of light reflected vs refracted.
 */
float3 fresnel_schlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/**
 * Fresnel Schlick with roughness for ambient/indirect lighting.
 */
float3 fresnel_schlick_roughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/* =================================================================================================
 *                                    COOK-TORRANCE BRDF
 * =================================================================================================
 */

/**
 * Evaluates the full Cook-Torrance specular BRDF.
 */
float3 evaluating_specular_brdf(float3 L, float3 V, float3 N, float3 F0, float alpha, out float3 F) {
    float3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    float D = distribution_ggx(NdotH, alpha);
    float G = geometry_smith(NdotV, NdotL, sqrt(alpha));
    F = fresnel_schlick(HdotV, F0);
    
    float3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    
    return numerator / max(denominator, 0.0001);
}

/**
 * Evaluates the Lambertian diffuse BRDF.
 */
float3 evaluating_diffuse_brdf(float3 albedo) {
    return albedo / PI;
}

/**
 * Combines diffuse and specular with energy conservation.
 */
float3 shade_pbr(float3 L, float3 V, float3 N, float3 albedo, float metallic, float roughness) {
    float alpha = max(roughness * roughness, 0.002);
    float3 F0 = mix(float3(0.04), albedo, metallic);
    
    float3 F;
    float3 specular = evaluating_specular_brdf(L, V, N, F0, alpha, F);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    
    float3 diffuse = evaluating_diffuse_brdf(albedo);
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * diffuse + specular) * NdotL;
}
