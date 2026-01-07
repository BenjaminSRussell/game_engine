#version 450
/*
 * Deferred Lighting Fragment Shader
 * Performs PBR lighting using G-Buffer inputs.
 */

layout(location = 0) in vec2 inUV;

// G-Buffer Inputs
layout(set = 0, binding = 0) uniform sampler2D texAlbedo;
layout(set = 0, binding = 1) uniform sampler2D texNormal;
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
} global;

layout(location = 0) out vec4 outColor;

/* --- PBR FUNCTIONS --- */

const float PI = 3.14159265359;

// Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Schlick-GGX Geometry Function
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Reconstruct World Position from Depth
vec3 WorldPosFromDepth(float depth, vec2 uv) {
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0); // Assuming 0..1 depth? Or Gl clip space. Vulkan is 0..1
    vec4 worldPos = global.invViewProj * clipSpace;
    return worldPos.xyz / worldPos.w;
}

void main() {
    // 1. Unpack G-Buffer
    vec4 albedoSample = texture(texAlbedo, inUV);
    vec3 albedo = albedoSample.rgb;
    
    // Normal (Decoding simple RGB storage for now)
    vec3 N = normalize(texture(texNormal, inUV).rgb * 2.0 - 1.0);
    
    vec4 matSample = texture(texMaterial, inUV);
    float metallic = matSample.r;
    float roughness = matSample.g;
    float ao = matSample.b;
    
    float depth = texture(texDepth, inUV).r;
    vec3 emissive = texture(texEmissive, inUV).rgb;

    if (depth == 1.0) { // Background / Sky
        // Ideally we would sample environment map or separate skypass
        outColor = vec4(emissive, 1.0); // Assuming sky writes to emissive or is cleared
        return; 
    }

    vec3 WorldPos = WorldPosFromDepth(depth, inUV);
    vec3 V = normalize(global.cameraPos - WorldPos); // View Vector
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // 2. Lighting Calculation (Single Directional Light for now)
    vec3 Lo = vec3(0.0);
    
    // Light Params
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

    // 3. Ambient
    vec3 ambient = vec3(0.03) * albedo * ao; // Simple ambient
    
    vec3 color = ambient + Lo + emissive;
    
    // Tone mapping and gamma correction usually in PostProcess pass, 
    // so here we output linear HDR color.
    outColor = vec4(color, 1.0);
}
