// PBR Shading - Metal Shaders
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float3 tangent [[attribute(2)]];
    float2 uv [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 world_pos;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float2 uv;
};

struct MaterialUniforms {
    float3 base_color;
    float metallic;
    float roughness;
    float specular;
    float3 emissive;
    float emissive_strength;
    float opacity;
    float3 subsurface_color;
    float subsurface_radius;
    float clear_coat;
    float clear_coat_roughness;
};

struct SceneUniforms {
    float4x4 view_proj;
    float4x4 model;
    float3 camera_pos;
    float3 light_dir;
    float3 light_color;
    float light_intensity;
};

// ===== VERTEX SHADER =====
vertex VertexOut pbr_vertex(
    VertexIn in [[stage_in]],
    constant SceneUniforms& scene [[buffer(0)]]
) {
    VertexOut out;
    
    float4 world_pos = scene.model * float4(in.position, 1.0);
    out.position = scene.view_proj * world_pos;
    out.world_pos = world_pos.xyz;
    
    float3x3 normal_matrix = float3x3(scene.model[0].xyz, scene.model[1].xyz, scene.model[2].xyz);
    out.normal = normalize(normal_matrix * in.normal);
    out.tangent = normalize(normal_matrix * in.tangent);
    out.bitangent = cross(out.normal, out.tangent);
    
    out.uv = in.uv;
    
    return out;
}

// ===== PBR LIGHTING FUNCTIONS =====

// Fresnel-Schlick approximation
float3 fresnel_schlick(float cos_theta, float3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

// GGX/Trowbridge-Reitz normal distribution
float distribution_ggx(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI_F * denom * denom;
    
    return a2 / max(denom, 0.0001);
}

// Smith's Schlick-GGX geometry function
float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// ===== DEFAULT LIT FRAGMENT =====
fragment float4 pbr_fragment_default(
    VertexOut in [[stage_in]],
    constant SceneUniforms& scene [[buffer(0)]],
    constant MaterialUniforms& material [[buffer(1)]],
    texture2d<float> base_color_tex [[texture(0)]],
    texture2d<float> normal_tex [[texture(1)]],
    texture2d<float> metallic_tex [[texture(2)]],
    texture2d<float> roughness_tex [[texture(3)]],
    texture2d<float> ao_tex [[texture(4)]],
    texture2d<float> emissive_tex [[texture(5)]],
    sampler texSampler [[sampler(0)]]
) {
    // Sample textures
    float3 albedo = base_color_tex.sample(texSampler, in.uv).rgb * material.base_color;
    float metallic = metallic_tex.sample(texSampler, in.uv).r * material.metallic;
    float roughness = roughness_tex.sample(texSampler, in.uv).r * material.roughness;
    float ao = ao_tex.sample(texSampler, in.uv).r;
    float3 emissive = emissive_tex.sample(texSampler, in.uv).rgb * material.emissive * material.emissive_strength;
    
    // Normal mapping
    float3 normal_sample = normal_tex.sample(texSampler, in.uv).xyz * 2.0 - 1.0;
    float3x3 TBN = float3x3(in.tangent, in.bitangent, in.normal);
    float3 N = normalize(TBN * normal_sample);
    
    float3 V = normalize(scene.camera_pos - in.world_pos);
    float3 L = normalize(-scene.light_dir);
    float3 H = normalize(V + L);
    
    // Calculate F0 (surface reflection at zero incidence)
    float3 F0 = float3(0.04);  // Dielectric base
    F0 = mix(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);
    float3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = numerator / max(denominator, 0.001);
    
    // Energy conservation
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    
    float NdotL = max(dot(N, L), 0.0);
    float3 radiance = scene.light_color * scene.light_intensity;
    
    float3 Lo = (kD * albedo / M_PI_F + specular) * radiance * NdotL;
    
    // Ambient
    float3 ambient = float3(0.03) * albedo * ao;
    
    float3 color = ambient + Lo + emissive;
    
    // Tone mapping
    color = color / (color + float3(1.0));
    
    // Gamma correction
    color = pow(color, float3(1.0 / 2.2));
    
    return float4(color, material.opacity);
}

// ===== SUBSURFACE SCATTERING FRAGMENT =====
fragment float4 pbr_fragment_subsurface(
    VertexOut in [[stage_in]],
    constant SceneUniforms& scene [[buffer(0)]],
    constant MaterialUniforms& material [[buffer(1)]],
    texture2d<float> base_color_tex [[texture(0)]],
    texture2d<float> normal_tex [[texture(1)]],
    texture2d<float> subsurface_tex [[texture(8)]],
    sampler texSampler [[sampler(0)]]
) {
    // First do standard PBR
    float3 albedo = base_color_tex.sample(texSampler, in.uv).rgb * material.base_color;
    float3 subsurface_col = subsurface_tex.sample(texSampler, in.uv).rgb * material.subsurface_color;
    
    float3 N = normalize(in.normal);
    float3 V = normalize(scene.camera_pos - in.world_pos);
    float3 L = normalize(-scene.light_dir);
    
    // Subsurface scattering approximation (Burley/Disney model)
    float wrap = 0.5;  // Wrap lighting
    float NdotL_wrapped = (dot(N, L) + wrap) / (1.0 + wrap);
    NdotL_wrapped = max(NdotL_wrapped, 0.0);
    
    // Thickness approximation (view dependent)
    float thickness = pow(1.0 - max(dot(N, V), 0.0), material.subsurface_radius);
    
    // Translucent backscatter
    float3 H = normalize(L + N * 0.01);  // Distorted by normal
    float VdotH = pow(saturate(dot(V, -H)), 4.0);
    float3 sss = VdotH * thickness * subsurface_col * scene.light_color;
    
    // Diffuse component with wrap lighting
    float3 diffuse = albedo * NdotL_wrapped * scene.light_color * scene.light_intensity;
    
    return float4(diffuse + sss, material.opacity);
}

// ===== CLEAR COAT FRAGMENT =====
fragment float4 pbr_fragment_clearcoat(
    VertexOut in [[stage_in]],
    constant SceneUniforms& scene [[buffer(0)]],
    constant MaterialUniforms& material [[buffer(1)]],
    texture2d<float> base_color_tex [[texture(0)]],
    texture2d<float> normal_tex [[texture(1)]],
    texture2d<float> clearcoat_tex [[texture(10)]],
    texture2d<float> clearcoat_roughness_tex [[texture(11)]],
    sampler texSampler [[sampler(0)]]
) {
    float3 albedo = base_color_tex.sample(texSampler, in.uv).rgb * material.base_color;
    float clearcoat = clearcoat_tex.sample(texSampler, in.uv).r * material.clear_coat;
    float clearcoat_roughness = clearcoat_roughness_tex.sample(texSampler, in.uv).r * material.clear_coat_roughness;
    
    float3 N = normalize(in.normal);
    float3 V = normalize(scene.camera_pos - in.world_pos);
    float3 L = normalize(-scene.light_dir);
    float3 H = normalize(V + L);
    
    // Base layer PBR (simplified)
    float NdotL = max(dot(N, L), 0.0);
    float3 base_color = albedo * NdotL * scene.light_color * scene.light_intensity;
    
    // Clear coat layer
    float coat_NDF = distribution_ggx(N, H, clearcoat_roughness);
    float coat_G = geometry_smith(N, V, L, clearcoat_roughness);
    float3 coat_F = fresnel_schlick(max(dot(H, V), 0.0), float3(0.04));
    
    float3 coat_spec = (coat_NDF * coat_G * coat_F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.001);
    coat_spec *= clearcoat;
    
    float3 final_color = base_color + coat_spec * scene.light_color * scene.light_intensity;
    
    return float4(final_color, material.opacity);
}

// ===== UNLIT FRAGMENT =====
fragment float4 pbr_fragment_unlit(
    VertexOut in [[stage_in]],
    constant MaterialUniforms& material [[buffer(1)]],
    texture2d<float> base_color_tex [[texture(0)]],
    texture2d<float> emissive_tex [[texture(5)]],
    sampler texSampler [[sampler(0)]]
) {
    float3 color = base_color_tex.sample(texSampler, in.uv).rgb * material.base_color;
    float3 emissive = emissive_tex.sample(texSampler, in.uv).rgb * material.emissive * material.emissive_strength;
    
    return float4(color + emissive, material.opacity);
}
