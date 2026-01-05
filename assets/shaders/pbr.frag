#version 450

// PBR Fragment Shader - Metallic-Roughness Workflow
// Cook-Torrance BRDF with GGX distribution

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;

// PBR Material Parameters (Push Constants)
layout(push_constant) uniform PBRMaterial {
    float metallic;
    float roughness;
    float ao;
    float normalStrength;
    vec4 albedo;
    vec4 emissive;
} material;

// Textures
layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D metallicRoughnessMap;
layout(binding = 3) uniform sampler2D aoMap;

// Camera/Lighting
layout(binding = 4) uniform SceneUniforms {
    vec3 cameraPos;
    vec3 lightDir;      // Directional light
    vec3 lightColor;
    float lightIntensity;
} scene;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / max(denom, 0.001);
}

// Geometry Function (Smith's Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // Sample textures
    vec4 albedoSample = texture(albedoMap, fragUV);
    vec3 albedo = pow(albedoSample.rgb, vec3(2.2)) * material.albedo.rgb; // sRGB to linear
    
    vec3 normal = normalize(fragNormal);
    // TODO: Apply normal map using TBN matrix
    
    float metallicSample = texture(metallicRoughnessMap, fragUV).b; // Blue channel
    float roughnessSample = texture(metallicRoughnessMap, fragUV).g; // Green channel
    float metallic = metallicSample * material.metallic;
    float roughness = roughnessSample * material.roughness;
    
    float ao = texture(aoMap, fragUV).r * material.ao;
    
    // PBR Lighting
    vec3 N = normal;
    vec3 V = normalize(scene.cameraPos - fragWorldPos);
    vec3 L = normalize(-scene.lightDir);
    vec3 H = normalize(V + L);
    
    // Base reflectivity (F0) - lerp between dielectric and metallic
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);
    
    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Metals have no diffuse
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = scene.lightColor * scene.lightIntensity;
    
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient (very simple - should use IBL)
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo + material.emissive.rgb;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, albedoSample.a * material.albedo.a);
}
