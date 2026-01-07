#version 450
/*
 * Deferred Lighting Fragment Shader
 * Performs PBR lighting using G-Buffer inputs.
 */

#include "normal_encoding.glsl"

layout(location = 0) in vec2 inUV;

// G-Buffer Inputs
layout(set = 0, binding = 0) uniform sampler2D texAlbedo;
layout(set = 0, binding = 1) uniform sampler2D texNormal;   // RG16F octahedral encoded
layout(set = 0, binding = 2) uniform sampler2D texMaterial; // Met, Rough, AO
layout(set = 0, binding = 3) uniform sampler2D texDepth;
layout(set = 0, binding = 4) uniform sampler2D texEmissive;

// Global Uniforms
layout(set = 1, binding = 0) uniform GlobalParams {
    mat4 invViewProj;
    vec3 cameraPos;
    vec3 sunDirection;
    vec3 sunColor;
    float sunIntensity;
    int debugMode; // 0: None, 1: Albedo, 2: Normals, 3: Depth, 4: Material, 5: Emissive
} global;

layout(location = 0) out vec4 outColor;

/* --- PBR FUNCTIONS --- */
// ... (omitted PBR functions for brevity in this view, same as before)

void main() {
    // 1. Unpack G-Buffer
    vec4 albedoSample = texture(texAlbedo, inUV);
    vec3 albedo = albedoSample.rgb;
    
    // Decode octahedral normal from RG16F
    vec2 encodedNormal = texture(texNormal, inUV).rg;
    vec3 N = octDecode(encodedNormal);
    
    vec4 matSample = texture(texMaterial, inUV);
    float metallic = matSample.r;
    float roughness = matSample.g;
    float ao = matSample.b;
    
    float depth = texture(texDepth, inUV).r;
    vec3 emissive = texture(texEmissive, inUV).rgb;

    // --- DEBUG VISUALIZATION ---
    if (global.debugMode == 1) { outColor = vec4(albedo, 1.0); return; }
    if (global.debugMode == 2) { outColor = vec4(N * 0.5 + 0.5, 1.0); return; }
    if (global.debugMode == 3) { outColor = vec4(vec3(pow(depth, 32.0)), 1.0); return; } // Visualize exponential depth
    if (global.debugMode == 4) { outColor = vec4(metallic, roughness, ao, 1.0); return; }
    if (global.debugMode == 5) { outColor = vec4(emissive, 1.0); return; }

    // Early out for background/sky
    if (depth >= 1.0) {
        outColor = vec4(emissive, 1.0);
        return; 
    }
// ... (rest of lighting logic)

    vec3 WorldPos = WorldPosFromDepth(depth, inUV);
    vec3 V = normalize(global.cameraPos - WorldPos); // View Vector
    
    // Fresnel reflectance at normal incidence
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // 2. Lighting Calculation (Directional Light)
    vec3 Lo = vec3(0.0);
    
    vec3 L = normalize(-global.sunDirection); // Direction TO light
    vec3 H = normalize(V + L);
    vec3 radiance = global.sunColor * global.sunIntensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    // 3. Ambient Lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo + emissive;
    
    // Output linear HDR color (tone mapping done in post-process)
    outColor = vec4(color, 1.0);
}
