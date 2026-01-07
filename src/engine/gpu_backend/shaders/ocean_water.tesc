#version 460 core

/**
 * =================================================================================================
 *                              OCEAN WATER TESSELLATION CONTROL SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 */

layout(vertices = 3) out;

layout(location = 0) in vec3 in_world_pos[];
layout(location = 1) in vec3 in_normal[];
layout(location = 2) in vec2 in_texcoord[];
layout(location = 3) in vec3 in_view_dir[];
layout(location = 4) in float in_depth[];
layout(location = 5) in float in_foam[];
layout(location = 6) in vec3 in_tess_factors[];

layout(location = 0) out vec3 out_world_pos[];
layout(location = 1) out vec3 out_normal[];
layout(location = 2) out vec2 out_texcoord[];
layout(location = 3) out vec3 out_view_dir[];
layout(location = 4) out float out_depth[];
layout(location = 5) out float out_foam[];

layout(set = 0, binding = 0) uniform OceanUBO {
    mat4 view_matrix;
    mat4 proj_matrix;
    mat4 view_proj_matrix;
    vec3 camera_pos;
    float time;
    
    // Gerstner waves
    int wave_layer_count;
    vec4 wave_data[8]; // direction(xy), wavelength, amplitude
    vec4 wave_extra[8]; // steepness, speed, phase_offset, padding
    
    // FFT waves
    bool use_fft_waves;
    float fft_scale;
    float fft_time;
    int fft_resolution;
    
    // Colors and depth
    vec4 shallow_color;
    vec4 deep_color;
    vec3 scatter_color;
    vec3 foam_color;
    float depth_max;
    float depth_falloff;
    float shore_depth;
    
    // Reflections
    float reflection_strength;
    float fresnel_power;
    float distortion_strength;
    bool use_ssr;
    bool use_planar_reflection;
    
    // Refraction
    float refraction_strength;
    vec3 absorption;
    float scattering;
    
    // Foam
    float foam_threshold;
    float foam_intensity;
    float foam_scale;
    
    // Caustics
    bool enable_caustics;
    float caustics_strength;
    float caustics_scale;
    float caustics_speed;
    
    // Underwater
    bool enable_underwater;
    float underwater_fog_density;
    vec3 underwater_fog_color;
} ubo;

/* =================================================================================================
 *                                    TESSELLATION LEVEL CALCULATION
 * =================================================================================================
 */
float calculateTessellationLevel(vec3 world_pos) {
    float distance = length(world_pos - ubo.camera_pos);
    
    // Distance-based tessellation
    float distance_factor = 1.0 - smoothstep(50.0, 500.0, distance);
    
    // Wave complexity factor (simplified based on time)
    float wave_factor = 0.5 + 0.5 * sin(ubo.time * 0.5);
    
    // View direction factor (more tessellation when viewed from above)
    vec3 view_dir = normalize(ubo.camera_pos - world_pos);
    float view_factor = max(view_dir.y, 0.0);
    
    float tess_level = distance_factor * wave_factor * view_factor * 32.0; // Max tess level of 32
    
    return clamp(tess_level, 1.0, 32.0);
}

/* =================================================================================================
 *                                    MAIN TESSELLATION CONTROL
 * =================================================================================================
 */
void main() {
    // Pass through vertex data
    out_world_pos[gl_InvocationID] = in_world_pos[gl_InvocationID];
    out_normal[gl_InvocationID] = in_normal[gl_InvocationID];
    out_texcoord[gl_InvocationID] = in_texcoord[gl_InvocationID];
    out_view_dir[gl_InvocationID] = in_view_dir[gl_InvocationID];
    out_depth[gl_InvocationID] = in_depth[gl_InvocationID];
    out_foam[gl_InvocationID] = in_foam[gl_InvocationID];
    
    if (gl_InvocationID == 0) {
        // Calculate tessellation levels for each edge
        float tess0 = calculateTessellationLevel(in_world_pos[0]);
        float tess1 = calculateTessellationLevel(in_world_pos[1]);
        float tess2 = calculateTessellationLevel(in_world_pos[2]);
        
        // Set tessellation levels
        gl_TessLevelOuter[0] = max(tess1, tess2);
        gl_TessLevelOuter[1] = max(tess2, tess0);
        gl_TessLevelOuter[2] = max(tess0, tess1);
        gl_TessLevelOuter[3] = max(tess0, tess1); // For triangles
        
        // Inner tessellation level (average of outer levels)
        gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3.0;
    }
}
