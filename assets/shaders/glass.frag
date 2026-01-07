#version 450 core
/*
 * glass.frag
 * Glass material fragment shader.
 * Features: Screen-space refraction, Fresnel reflection, chromatic aberration.
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
layout(push_constant) uniform MaterialPC {
    vec4 base_color;
    float roughness;
    float metallic;
    float ior;
    float chromatic_aberration;
    // ...
} material;

layout(set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view_projection;
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    float time;
};

layout(set = 1, binding = 0) uniform sampler2D t_scene_color; // Backbuffer copy

// =============================================================================
// OUTPUTS
// =============================================================================
layout(location = 0) out vec4 out_accum;
layout(location = 1) out float out_reveal;

// =============================================================================
// FUNCTIONS
// =============================================================================

vec3 calculate_refraction(vec3 view_dir, vec3 normal, float ior) {
    // 1. Calculate offset based on normal and IOR
    // Simple screen-space approximation
    vec2 screen_uv = gl_FragCoord.xy / vec2(textureSize(t_scene_color, 0));
    
    // Scale distortion by normal facing
    vec3 refract_dir = refract(-view_dir, normal, 1.0 / ior);
    vec2 offset = refract_dir.xy * 0.05 * (1.0 + material.chromatic_aberration); // Scaling factor
    
    // 2. Chromatic Aberration (sample R, G, B separately)
    float r = texture(t_scene_color, screen_uv + offset).r;
    float g = texture(t_scene_color, screen_uv + offset * 1.02).g;
    float b = texture(t_scene_color, screen_uv + offset * 1.05).b;
    
    return vec3(r, g, b);
}

float calculate_weight(float z, float alpha) {
    // Standard WBOIT weight function
    float a = min(1.0, alpha * 10.0) + 0.01;
    float b = 1.0 - gl_FragCoord.z * 0.99;
    return clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e3);
}

// =============================================================================
// MAIN
// =============================================================================
void main() {
    vec3 N = normalize(f_normal);
    vec3 V = normalize(camera_position - f_world_pos);
    
    // Fresnel
    float f0 = pow((material.ior - 1.0) / (material.ior + 1.0), 2.0);
    float fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    
    // Refraction
    vec3 refraction_color = calculate_refraction(V, N, material.ior);
    
    // Reflection ( approximated with environment map or just specular highlight)
    // For now, just using base color as "reflection" tint
    vec3 reflection_color = material.base_color.rgb; 
    
    // Mix based on Fresnel
    vec3 final_color = mix(refraction_color, reflection_color, fresnel);
    
    // Apply tint absorption
    final_color *= material.base_color.rgb;
    
    float alpha = material.base_color.a; // Glass opacity
    
    // WBOIT Output
    float w = calculate_weight(gl_FragCoord.z, alpha);
    out_accum = vec4(final_color * alpha * w, alpha * w);
    out_reveal = alpha;
}
