#version 460 core

/**
 * =================================================================================================
 *                              OCEAN WATER VERTEX SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 */

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;

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

// Tessellation factors
layout(location = 6) out vec3 out_tess_factors;

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
    
    // Sample from FFT displacement texture
    vec2 uv = pos.xz * ubo.fft_scale + vec2(ubo.fft_time * 0.1);
    uv = fract(uv);
    
    vec3 displacement = texture(fft_displacement_texture, uv).xyz;
    return displacement * vec3(1.0, 0.5, 1.0); // Scale down Y for realism
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
    
    // Wave breaking foam (from folding/Jacobian)
    if (folding < ubo.foam_threshold) {
        foam += 1.0 - (folding / ubo.foam_threshold);
    }
    
    // Shore foam (based on depth)
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
    // Sample depth texture (terrain height)
    vec2 depth_uv = (world_pos.xz + 512.0) / 1024.0; // Assuming 1024x1024 depth map
    float terrain_height = texture(depth_texture, depth_uv).r;
    
    return world_pos.y - terrain_height;
}

/* =================================================================================================
 *                                    TESSELLATION FACTORS
 * =================================================================================================
 */
vec3 calculateTessFactors(vec3 world_pos) {
    float distance = length(world_pos - ubo.camera_pos);
    
    // Distance-based tessellation
    float distance_factor = 1.0 - smoothstep(50.0, 500.0, distance);
    
    // Wave height factor (simplified)
    float wave_height = 2.0; // Maximum expected wave height
    float wave_factor = min(wave_height / 2.0, 1.0);
    
    float tess_factor = distance_factor * wave_factor * 16.0; // Max tess factor of 16
    
    return vec3(tess_factor, tess_factor, tess_factor);
}

/* =================================================================================================
 *                                    MAIN VERTEX SHADER
 * =================================================================================================
 */
void main() {
    vec3 world_pos = in_position;
    vec3 total_displacement = vec3(0.0);
    vec3 total_normal = vec3(0.0, 1.0, 0.0);
    
    // Calculate Gerstner wave displacement and normals
    for (int i = 0; i < ubo.wave_layer_count && i < 8; i++) {
        total_displacement += calculateGerstnerDisplacement(world_pos, i);
        total_normal += calculateGerstnerNormal(world_pos, i);
    }
    
    // Add FFT waves if enabled
    if (ubo.use_fft_waves) {
        total_displacement += sampleFFTDisplacement(world_pos);
        vec3 fft_normal = sampleFFTNormal(world_pos);
        total_normal += fft_normal;
    }
    
    // Apply displacement
    world_pos += total_displacement;
    
    // Calculate final normal
    total_normal = normalize(total_normal);
    
    // Calculate depth
    float depth = getWaterDepth(world_pos);
    
    // Calculate foam
    float folding = sampleFFTFolding(world_pos);
    float foam = calculateFoam(world_pos, folding, depth);
    
    // Calculate tessellation factors
    vec3 tess_factors = calculateTessFactors(world_pos);
    
    // Transform to clip space
    gl_Position = ubo.view_proj_matrix * vec4(world_pos, 1.0);
    
    // Output to fragment shader
    out_world_pos = world_pos;
    out_normal = total_normal;
    out_texcoord = in_texcoord;
    out_view_dir = normalize(ubo.camera_pos - world_pos);
    out_depth = depth;
    out_foam = foam;
    out_tess_factors = tess_factors;
}
