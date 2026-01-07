#version 450 core
/*
 * water.frag
 * Water surface fragment shader.
 * Features: Procedural waves, foam, underwater fog, reflection/refraction.
 */

// =============================================================================
// INPUTS
// =============================================================================
layout(location = 0) in vec3 f_world_pos;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec2 f_uv;
layout(location = 3) in vec4 f_tangent;
layout(location = 4) in vec4 f_color;

layout(push_constant) uniform MaterialPC {
    vec4 shallow_color;
    vec4 deep_color;
    float foam_threshold;
    float time;
    // ...
} material;

layout(set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view_projection;
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    float global_time;
};

layout(set = 1, binding = 0) uniform sampler2D t_scene_color; // Refraction
layout(set = 1, binding = 1) uniform sampler2D t_depth;       // Linear depth for foam/fog

// =============================================================================
// OUTPUTS
// =============================================================================
layout(location = 0) out vec4 out_accum;
layout(location = 1) out float out_reveal;

// =============================================================================
// FUNCTIONS
// =============================================================================

float linearize_depth(float d) {
    float near = 0.1;
    float far = 1000.0;
    return (2.0 * near) / (far + near - d * (far - near));
}

float calculate_weight(float z, float alpha) {
    float a = min(1.0, alpha * 10.0) + 0.01;
    float b = 1.0 - gl_FragCoord.z * 0.99;
    return clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e3);
}

// =============================================================================
// MAIN
// =============================================================================
void main() {
    // 1. Calculate View Vectors
    vec3 V = normalize(camera_position - f_world_pos);
    vec3 N = normalize(f_normal); // Perturbed by normal map or wave function
    
    // 2. Depth / Foam Calculation
    vec2 screen_uv = gl_FragCoord.xy / vec2(textureSize(t_depth, 0));
    float scene_depth_ndc = texture(t_depth, screen_uv).r;
    float pixel_depth_ndc = gl_FragCoord.z;
    
    // Linearize for easier distance math (simplified)
    // float scene_depth = linearize_depth(scene_depth_ndc);
    // float pixel_depth = linearize_depth(pixel_depth_ndc);
    // float water_depth = scene_depth - pixel_depth;
    
    // Just use raw diff for foam modulation
    float depth_diff = scene_depth_ndc - pixel_depth_ndc;
    float foam_mask = smoothstep(material.foam_threshold, 0.0, depth_diff * 100.0); // Tweak scale
    
    // 3. Color Mixing (Shallow vs Deep)
    vec4 water_color = mix(material.deep_color, material.shallow_color, foam_mask);
    vec4 foam_color = vec4(1.0, 1.0, 1.0, 1.0); // White foam
    
    vec4 final_color = mix(water_color, foam_color, foam_mask);
    
    // 4. Refraction (Distorted scene color)
    vec2 distortion = N.xy * 0.02;
    vec3 refraction = texture(t_scene_color, screen_uv + distortion).rgb;
    
    // Mix refraction with water color based on alpha/depth
    final_color.rgb = mix(refraction, final_color.rgb, final_color.a);
    
    // 5. Output (WBOIT)
    float alpha = final_color.a;
    float w = calculate_weight(gl_FragCoord.z, alpha);
    
    out_accum = vec4(final_color.rgb * alpha * w, alpha * w);
    out_reveal = alpha;
}
