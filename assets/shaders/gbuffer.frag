#version 450
/*
 * G-Buffer Fragment Shader
 * Outputs: Albedo, Normal (RG16F octahedral encoded), Material, Emissive, Velocity
 */

#include "normal_encoding.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;
layout(location = 5) in vec4 inClipPos;
layout(location = 6) in vec4 inPrevClipPos;

// Material Textures
layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;
layout(set = 1, binding = 2) uniform sampler2D texMetallicRoughness; // Approx glTF standard
layout(set = 1, binding = 3) uniform sampler2D texEmissive;

// Material Properties Uniforms (multipliers/factors)
layout(set = 1, binding = 4) uniform MaterialProps {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    vec3 emissiveFactor;
} material;

// Multiple Render Targets
layout(location = 0) out vec4 outAlbedo;    // RGBA8
layout(location = 1) out vec2 outNormal;    // RG16F (Octahedral Encoded)
layout(location = 2) out vec4 outMaterial;  // RGBA8 (Met, Rough, AO, ID)
layout(location = 3) out vec3 outEmissive;  // R11G11B10F (HDR, no alpha)
layout(location = 4) out vec2 outVelocity;  // RG16F

void main() {
    // 1. Albedo
    vec4 albedo = texture(texAlbedo, inUV) * material.baseColorFactor;
    if (albedo.a < 0.5) discard; // Simple alpha masking
    outAlbedo = albedo;

    // 2. Normal Mapping (Tangent Space -> World Space)
    vec3 normalMap = texture(texNormal, inUV).xyz * 2.0 - 1.0;
    normalMap.xy *= material.normalScale;
    mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
    vec3 N = normalize(TBN * normalMap);
    
    // Encode Normal using Octahedral projection (RG16F)
    outNormal = octEncode(N);

    // 3. Material Properties
    // glTF standard: B=Metallic, G=Roughness, R=AO
    vec4 mrSample = texture(texMetallicRoughness, inUV);
    float metallic = mrSample.b * material.metallicFactor;
    float roughness = mrSample.g * material.roughnessFactor;
    float ao = mrSample.r; // Ambient Occlusion
    
    outMaterial = vec4(metallic, roughness, ao, 1.0); // A can be Shading Model ID

    // 4. Emissive (R11G11B10F has no alpha channel)
    vec3 emissive = texture(texEmissive, inUV).rgb * material.emissiveFactor;
    outEmissive = emissive;

    // 5. Velocity (Motion Vectors for TAA)
    // Calculate screen space velocity from current and previous clip positions
    vec2 ndcPos = (inClipPos.xy / inClipPos.w) * 0.5 + 0.5;
    vec2 ndcPrevPos = (inPrevClipPos.xy / inPrevClipPos.w) * 0.5 + 0.5;
    vec2 velocity = ndcPos - ndcPrevPos;
    outVelocity = velocity;
}
