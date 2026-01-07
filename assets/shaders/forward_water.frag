// forward_water.frag
// Forward rendering water shader with waves, refraction, and foam
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

layout(set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 proj;
    vec3 camera_pos;
    float time;
} global;

layout(set = 1, binding = 0) uniform sampler2D u_scene_color; // For refraction
layout(set = 1, binding = 1) uniform sampler2D u_water_normal;
layout(set = 1, binding = 2) uniform sampler2D u_dudv_map;

layout(push_constant) uniform WaterParams {
    vec4 deep_color;
    vec4 shallow_color;
    float wave_speed;
    float wave_strength;
    float refraction_index;
} water;

//===============================================================================
// OUTPUTS
//===============================================================================

layout(location = 0) out vec4 frag_color;

//===============================================================================
// MAIN
//===============================================================================

void main() {
    // 1. Animated Normal Mapping
    vec2 dist_uv = v_texcoord + global.time * water.wave_speed;
    vec3 normal1 = texture(u_water_normal, dist_uv).rgb * 2.0 - 1.0;
    vec3 normal2 = texture(u_water_normal, dist_uv * 0.5 + 0.2).rgb * 2.0 - 1.0;
    vec3 N = normalize(normal1 + normal2);
    
    // 2. Screen-space Refraction
    vec2 screen_size = vec2(textureSize(u_scene_color, 0));
    vec2 screen_uv = gl_FragCoord.xy / screen_size;
    
    vec2 distortion = N.xy * water.wave_strength;
    vec3 transmission = texture(u_scene_color, screen_uv + distortion).rgb;

    // 3. Fresnel & Depth Effect (Simplified)
    vec3 V = normalize(global.camera_pos - v_position);
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    fresnel = pow(fresnel, 3.0);

    // 4. Color Selection
    vec3 water_base = mix(water.shallow_color.rgb, water.deep_color.rgb, 0.5);
    
    // 5. Final Composition
    vec3 final_color = mix(transmission * water_base, vec3(0.5, 0.8, 1.0), fresnel);
    
    // Add specular highlight
    vec3 L = normalize(vec3(0.5, 1.0, 0.2));
    vec3 H = normalize(V + L);
    float spec = pow(max(dot(N, H), 0.0), 64.0);
    final_color += spec * 0.5;

    frag_color = vec4(final_color, 0.8);
}
