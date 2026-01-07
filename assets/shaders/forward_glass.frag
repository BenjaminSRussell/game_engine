// forward_glass.frag
// Forward rendering glass shader with refraction and chromatic aberration
#version 450 core

//===============================================================================
// INPUTS
//===============================================================================

layout(location = 0) in vec3 v_position;      // World space position
layout(location = 1) in vec3 v_normal;        // World space normal
layout(location = 2) in vec2 v_texcoord;      // Texture coordinates
layout(location = 3) in vec3 v_tangent;       // World space tangent
layout(location = 4) in vec3 v_bitangent;     // World space bitangent

//===============================================================================  
// UNIFORMS
//===============================================================================

layout(set = 0, binding = 0) uniform CameraUniforms {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(set = 1, binding = 0) uniform sampler2D u_scene_color; // Backbuffer copy for refraction
layout(set = 1, binding = 1) uniform sampler2D u_normal_map;
layout(set = 1, binding = 2) uniform sampler2D u_thickness_map;

layout(push_constant) uniform MaterialParams {
    vec4 tint;           // Glass color tint
    float ior;           // Index of Refraction (e.g. 1.52)
    float thickness;     // Simulated thickness
    float aberration;    // Chromatic aberration strength
    float roughness;     // Surface roughness
} material;

//===============================================================================
// OUTPUTS
//===============================================================================

layout(location = 0) out vec4 frag_color;

//===============================================================================
// HELPERS
//===============================================================================

// Simple Schlick approximation for Fresnel
float fresnel_schlick(float cos_theta, float f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

//===============================================================================
// MAIN
//===============================================================================

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(camera.position - v_position);
    float NdotV = max(dot(N, V), 0.0);

    // 1. Refraction Calculation
    // Simple screen-space distortion based on normal
    vec2 screen_size = vec2(textureSize(u_scene_color, 0));
    vec2 screen_uv = gl_FragCoord.xy / screen_size;
    
    // Scale offset by thickness and IOR
    float refraction_scale = material.thickness * (1.0 - 1.0 / material.ior);
    vec2 refraction_offset = N.xy * refraction_scale;

    // 2. Chromatic Aberration
    // Sample R, G, B with different offsets
    float r = texture(u_scene_color, screen_uv + refraction_offset * (1.0 + material.aberration)).r;
    float g = texture(u_scene_color, screen_uv + refraction_offset).g;
    float b = texture(u_scene_color, screen_uv + refraction_offset * (1.0 - material.aberration)).b;
    vec3 transmission = vec3(r, g, b) * material.tint.rgb;

    // 3. Specular Reflection
    // Simplified specular highlight
    vec3 L = normalize(vec3(0.5, 1.0, 0.3)); // Hardcoded directional light
    vec3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, 128.0 / (material.roughness + 0.01));

    // 4. Fresnel Blending
    float f0 = pow((1.0 - material.ior) / (1.0 + material.ior), 2.0);
    float fresnel = fresnel_schlick(NdotV, f0);

    // Final Color = Transmission tinted + Specular Reflection
    vec3 final_color = mix(transmission, vec3(1.0), fresnel) + specular * fresnel;
    
    frag_color = vec4(final_color, material.tint.a);
}
