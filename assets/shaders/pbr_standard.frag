// Example PBR Shader - Fragment Shader
// Features enabled/disabled via permutation system

// === Inputs ===
in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_uv;

#ifdef FEATURE_VERTEX_COLORS
in vec4 v_color;
#endif

#ifdef FEATURE_NORMAL_MAP
in mat3 v_tbn;
#endif

// === Output ===
out vec4 frag_color;

// === Uniforms ===
uniform vec3 u_base_color;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;

#ifdef FEATURE_NORMAL_MAP
uniform sampler2D u_normal_map;
#endif

#ifdef FEATURE_ROUGHNESS_MAP
uniform sampler2D u_roughness_map;
#endif

#ifdef FEATURE_METALLIC_MAP
uniform sampler2D u_metallic_map;
#endif

#ifdef FEATURE_AO_MAP
uniform sampler2D u_ao_map;
#endif

#ifdef FEATURE_EMISSIVE
uniform vec3 u_emissive_color;
uniform sampler2D u_emissive_map;
#endif

// === Constants ===
const float PI = 3.14159265359;

// === PBR Functions ===
// Simplified for example

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // Base color
    vec3 albedo = u_base_color;
    
    #ifdef FEATURE_VERTEX_COLORS
    albedo *= v_color.rgb;
    #endif
    
    // Normal
    vec3 N = normalize(v_normal);
    
    #ifdef FEATURE_NORMAL_MAP
    vec3 normal_sample = texture(u_normal_map, v_uv).rgb;
    N = normalize(v_tbn * (normal_sample * 2.0 - 1.0));
    #endif
    
    // Material properties
    float metallic = u_metallic;
    float roughness = u_roughness;
    float ao = u_ao;
    
    #ifdef FEATURE_METALLIC_MAP
    metallic = texture(u_metallic_map, v_uv).r;
    #endif
    
    #ifdef FEATURE_ROUGHNESS_MAP
    roughness = texture(u_roughness_map, v_uv).r;
    #endif
    
    #ifdef FEATURE_AO_MAP
    ao = texture(u_ao_map, v_uv).r;
    #endif
    
    // Simplified lighting (single directional light for now)
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 view_dir = normalize(vec3(0.0, 0.0, 1.0)); // Placeholder
    
    vec3 H = normalize(view_dir + light_dir);
    float NdotL = max(dot(N, light_dir), 0.0);
    
    // PBR lighting calculation
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_schlick_ggx(max(dot(N, view_dir), 0.0), roughness);
    vec3 F = fresnel_schlick(max(dot(H, view_dir), 0.0), F0);
    
    vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, view_dir), 0.0) * NdotL, 0.001);
    
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;
    
    vec3 diffuse = kD * albedo / PI;
    
    vec3 radiance = vec3(1.0); // Light color
    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    
    #ifdef FEATURE_EMISSIVE
    color += u_emissive_color * texture(u_emissive_map, v_uv).rgb;
    #endif
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    frag_color = vec4(color, 1.0);
    
    #ifdef FEATURE_ALPHA_TEST
    if (frag_color.a < 0.5) discard;
    #endif
}
