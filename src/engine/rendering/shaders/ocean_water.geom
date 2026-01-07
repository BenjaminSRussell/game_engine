#version 460 core

/**
 * =================================================================================================
 *                              OCEAN WATER GEOMETRY SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 *
 * Optional geometry shader for additional effects like wireframe rendering,
 * normal visualization, or LOD-based culling.
 *
 * =================================================================================================
 */

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 in_world_pos[];
layout(location = 1) in vec3 in_normal[];
layout(location = 2) in vec2 in_texcoord[];
layout(location = 3) in vec3 in_view_dir[];
layout(location = 4) in float in_depth[];
layout(location = 5) in float in_foam[];

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;
layout(location = 3) out vec3 out_view_dir;
layout(location = 4) out float out_depth;
layout(location = 5) out float out_foam;

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
    
    // Geometry shader options
    bool enable_wireframe;
    bool enable_normal_visualization;
    bool enable_backface_culling;
} ubo;

/* =================================================================================================
 *                                    LOD CULLING
 * =================================================================================================
 */
bool shouldCullTriangle() {
    if (!ubo.enable_backface_culling) {
        return false;
    }
    
    // Calculate triangle normal
    vec3 edge1 = in_world_pos[1] - in_world_pos[0];
    vec3 edge2 = in_world_pos[2] - in_world_pos[0];
    vec3 triangle_normal = normalize(cross(edge1, edge2));
    
    // Check if triangle is facing away from camera
    vec3 view_dir = normalize(ubo.camera_pos - in_world_pos[0]);
    return dot(triangle_normal, view_dir) < 0.0;
}

/* =================================================================================================
 *                                    MAIN GEOMETRY SHADER
 * =================================================================================================
 */
void main() {
    // Cull back-facing triangles if enabled
    if (shouldCullTriangle()) {
        return;
    }
    
    // Pass through triangle data
    for (int i = 0; i < 3; i++) {
        out_world_pos = in_world_pos[i];
        out_normal = in_normal[i];
        out_texcoord = in_texcoord[i];
        out_view_dir = in_view_dir[i];
        out_depth = in_depth[i];
        out_foam = in_foam[i];
        
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    
    EndPrimitive();
}
