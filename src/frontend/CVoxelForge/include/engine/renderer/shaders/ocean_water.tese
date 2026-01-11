#version 460 core

/**
 * =================================================================================================
 *                              OCEAN WATER TESSELLATION EVALUATION SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 */

layout(triangles, equal_spacing, cw) in;

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
} ubo;

layout(set = 1, binding = 0) uniform sampler2D fft_displacement_texture;
layout(set = 1, binding = 1) uniform sampler2D fft_normal_texture;
layout(set = 1, binding = 2) uniform sampler2D fft_folding_texture;
layout(set = 1, binding = 3) uniform sampler2D foam_texture;
layout(set = 1, binding = 4) uniform sampler2D depth_texture;

/* =================================================================================================
 *                                    BARYCENTRIC INTERPOLATION
 * =================================================================================================
 */
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return vec3(gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2);
}

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return vec2(gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2);
}

float interpolateFloat(float v0, float v1, float v2) {
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

/* =================================================================================================
 *                                    GERSTNER WAVE CALCULATIONS
 * =================================================================================================
 */
vec3 calculateGerstnerDisplacement(vec3 pos, int layer) {
    vec2 direction = ubo.wave_data[layer].xy;
    float wavelength = ubo.wave_data[layer].z;
    float amplitude = ubo.wave_data[layer].w;
    
    float steepness = ubo.wave_extra[layer].x;
    float speed = ubo.wave_extra[layer].y;
    float phase_offset = ubo.wave_extra[layer].z;
    
    float k = 2.0 * PI / wavelength;
    float omega = k * speed;
    float phase = k * dot(pos.xz, direction) - omega * ubo.time + phase_offset;
    
    float cos_phase = cos(phase);
    float sin_phase = sin(phase);
    
    float steepness_factor = steepness / k;
    
    return vec3(
        steepness_factor * direction.x * sin_phase,
        amplitude * cos_phase,
        steepness_factor * direction.y * sin_phase
    );
}

vec3 calculateGerstnerNormal(vec3 pos, int layer) {
    vec2 direction = ubo.wave_data[layer].xy;
    float wavelength = ubo.wave_data[layer].z;
    float amplitude = ubo.wave_data[layer].w;
    
    float steepness = ubo.wave_extra[layer].x;
    float speed = ubo.wave_extra[layer].y;
    float phase_offset = ubo.wave_extra[layer].z;
    
    float k = 2.0 * PI / wavelength;
    float omega = k * speed;
    float phase = k * dot(pos.xz, direction) - omega * ubo.time + phase_offset;
    
    float cos_phase = cos(phase);
    float wa = amplitude * k;
    float steepness_factor = steepness;
    
    return vec3(
        direction.x * wa * cos_phase,
        steepness_factor * wa * sin(phase),
        direction.y * wa * cos_phase
    );
}

/* =================================================================================================
 *                                    FFT WAVE SAMPLING
 * =================================================================================================
 */
vec3 sampleFFTDisplacement(vec3 pos) {
    if (!ubo.use_fft_waves) {
        return vec3(0.0);
    }
    
    vec2 uv = pos.xz * ubo.fft_scale + vec2(ubo.fft_time * 0.1);
    uv = fract(uv);
    
    vec3 displacement = texture(fft_displacement_texture, uv).xyz;
    return displacement * vec3(1.0, 0.5, 1.0);
}

vec3 sampleFFTNormal(vec3 pos) {
    if (!ubo.use_fft_waves) {
        return vec3(0.0, 1.0, 0.0);
    }
    
    vec2 uv = pos.xz * ubo.fft_scale + vec2(ubo.fft_time * 0.1);
    uv = fract(uv);
    
    vec3 normal = texture(fft_normal_texture, uv).xyz;
    return normalize(normal * 2.0 - 1.0);
}

float sampleFFTFolding(vec3 pos) {
    if (!ubo.use_fft_waves) {
        return 1.0;
    }
    
    vec2 uv = pos.xz * ubo.fft_scale + vec2(ubo.fft_time * 0.1);
    uv = fract(uv);
    
    return texture(fft_folding_texture, uv).r;
}

/* =================================================================================================
 *                                    FOAM CALCULATION
 * =================================================================================================
 */
float calculateFoam(vec3 pos, float folding, float depth) {
    float foam = 0.0;
    
    // Wave breaking foam
    if (folding < ubo.foam_threshold) {
        foam += 1.0 - (folding / ubo.foam_threshold);
    }
    
    // Shore foam
    if (depth < ubo.shore_depth) {
        foam += 1.0 - (depth / ubo.shore_depth);
    }
    
    // Sample foam texture
    vec2 foam_uv = pos.xz * ubo.foam_scale + vec2(ubo.time * 0.05);
    foam *= texture(foam_texture, foam_uv).r;
    
    return clamp(foam * ubo.foam_intensity, 0.0, 1.0);
}

/* =================================================================================================
 *                                    DEPTH CALCULATION
 * =================================================================================================
 */
float getWaterDepth(vec3 world_pos) {
    vec2 depth_uv = (world_pos.xz + 512.0) / 1024.0;
    float terrain_height = texture(depth_texture, depth_uv).r;
    
    return world_pos.y - terrain_height;
}

/* =================================================================================================
 *                                    MAIN TESSELLATION EVALUATION
 * =================================================================================================
 */
void main() {
    // Interpolate vertex attributes
    vec3 world_pos = interpolate3D(in_world_pos[0], in_world_pos[1], in_world_pos[2]);
    vec3 normal = interpolate3D(in_normal[0], in_normal[1], in_normal[2]);
    vec2 texcoord = interpolate2D(in_texcoord[0], in_texcoord[1], in_texcoord[2]);
    vec3 view_dir = interpolate3D(in_view_dir[0], in_view_dir[1], in_view_dir[2]);
    float depth = interpolateFloat(in_depth[0], in_depth[1], in_depth[2]);
    float foam = interpolateFloat(in_foam[0], in_foam[1], in_foam[2]);
    
    // Calculate wave displacement and normals at interpolated position
    vec3 total_displacement = vec3(0.0);
    vec3 total_normal = vec3(0.0, 1.0, 0.0);
    
    // Gerstner waves
    for (int i = 0; i < ubo.wave_layer_count && i < 8; i++) {
        total_displacement += calculateGerstnerDisplacement(world_pos, i);
        total_normal += calculateGerstnerNormal(world_pos, i);
    }
    
    // FFT waves
    if (ubo.use_fft_waves) {
        total_displacement += sampleFFTDisplacement(world_pos);
        vec3 fft_normal = sampleFFTNormal(world_pos);
        total_normal += fft_normal;
    }
    
    // Apply displacement
    world_pos += total_displacement;
    
    // Calculate final normal
    total_normal = normalize(total_normal);
    
    // Recalculate depth after displacement
    depth = getWaterDepth(world_pos);
    
    // Calculate foam
    float folding = sampleFFTFolding(world_pos);
    foam = calculateFoam(world_pos, folding, depth);
    
    // Update view direction after displacement
    view_dir = normalize(ubo.camera_pos - world_pos);
    
    // Transform to clip space
    gl_Position = ubo.view_proj_matrix * vec4(world_pos, 1.0);
    
    // Output to fragment shader
    out_world_pos = world_pos;
    out_normal = total_normal;
    out_texcoord = texcoord;
    out_view_dir = view_dir;
    out_depth = depth;
    out_foam = foam;
}
