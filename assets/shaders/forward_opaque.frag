#version 450 core
/*
 * forward_opaque.frag
 * Standard opaque forward pass fragment shader.
 * Implements PBR lighting (simplified for now).
 */

// =============================================================================
// INPUTS
// =============================================================================
layout(location = 0) in vec3 f_world_pos;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec2 f_uv;
layout(location = 3) in vec4 f_tangent;
layout(location = 4) in vec4 f_color;

// =============================================================================
// UNIFORMS
// =============================================================================
layout(set = 1, binding = 0) uniform sampler2D t_albedo;
layout(set = 1, binding = 1) uniform sampler2D t_normal;
layout(set = 1, binding = 2) uniform sampler2D t_metallic_roughness;
// ... other maps

struct MaterialProperties {
    vec4 base_color;
    float metallic;
    float roughness;
    float ao;
    float emissive;
};

layout(push_constant) uniform MaterialPC {
    layout(offset = 64) MaterialProperties material; // Offset after vertex PC data
};

// =============================================================================
// OUTPUTS
// =============================================================================
layout(location = 0) out vec4 out_color;

// =============================================================================
// MAIN
// =============================================================================
void main() {
    // 1. Base Color
    vec4 albedo = material.base_color * f_color;
#ifdef FEATURE_ALBEDO_MAP
    albedo *= texture(t_albedo, f_uv);
#endif

    // Alpha Test
#ifdef FEATURE_ALPHA_TEST
    if (albedo.a < 0.5) discard;
#endif

    // 2. Normal Mapping
    vec3 N = normalize(f_normal);
#ifdef FEATURE_NORMAL_MAP
    // TODO: Tangent space calculation
#endif

    // 3. Lighting (Simple directional for now)
    vec3 L = normalize(vec3(0.5, 1.0, 0.3)); // Hardcoded light dir
    float NdotL = max(dot(N, L), 0.0);
    
    vec3 ambient = vec3(0.1) * albedo.rgb;
    vec3 diffuse = albedo.rgb * NdotL;
    
    // Result
    vec3 final_color = ambient + diffuse;
    
    out_color = vec4(final_color, 1.0); // Opaque always 1.0 alpha
}
