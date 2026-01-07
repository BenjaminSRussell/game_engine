#version 450

// Image-Based Lighting (IBL) Fragment Shader
// Pre-filtered environment map for specular + irradiance map for diffuse

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Environment maps
layout(binding = 5) uniform samplerCube irradianceMap;      // Pre-convolved diffuse
layout(binding = 6) uniform samplerCube prefilterMap;       // Pre-filtered specular
layout(binding = 7) uniform sampler2D brdfLUT;              // BRDF integration LUT

// Material (from PBR push constants)
layout(push_constant) uniform Material {
    float metallic;
    float roughness;
    float ao;
    vec3 albedo;
    vec3 cameraPos;
} material;

const float PI = 3.14159265359;

// Fresnel-Schlick with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(material.cameraPos - fragWorldPos);
    vec3 R = reflect(-V, N);
    
    // Base reflectivity
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);
    
    // Fresnel with roughness for IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, material.roughness);
    
    // Diffuse IBL (irradiance)
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - material.metallic;
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * material.albedo;
    
    // Specular IBL (pre-filtered environment)
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, material.roughness * MAX_REFLECTION_LOD).rgb;
    
    // BRDF integration (2D LUT: NdotV, roughness)
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), material.roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
    
    // Combine diffuse + specular IBL
    vec3 ambient = (kD * diffuse + specular) * material.ao;
    
    // Simple tonemapping
    vec3 color = ambient / (ambient + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    
    outColor = vec4(color, 1.0);
}
