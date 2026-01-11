#version 460 core

/**
 * =================================================================================================
 *                              OCEAN WATER FRAGMENT SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 */

layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_view_dir;
layout(location = 4) in float in_depth;
layout(location = 5) in float in_foam;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal; // For deferred rendering
layout(location = 2) out vec4 out_roughness_metallic; // For PBR

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

layout(set = 2, binding = 0) uniform sampler2D reflection_texture;
layout(set = 2, binding = 1) uniform sampler2D refraction_texture;
layout(set = 2, binding = 2) uniform sampler2D caustics_texture;
layout(set = 2, binding = 3) uniform samplerCube environment_texture;

layout(set = 3, binding = 0) uniform sampler2D noise_texture;

/* =================================================================================================
 *                                    COLOR BLENDING
 * =================================================================================================
 */
vec4 depthColorBlend(float depth) {
    float depth_factor = clamp(depth / ubo.depth_max, 0.0, 1.0);
    depth_factor = pow(depth_factor, ubo.depth_falloff);
    
    return mix(ubo.shallow_color, ubo.deep_color, depth_factor);
}

/* =================================================================================================
 *                                    FRESNEL REFLECTION
 * =================================================================================================
 */
float fresnelReflection(float cos_theta) {
    float f0 = 0.02; // Water reflectance at normal incidence
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, ubo.fresnel_power);
}

/* =================================================================================================
 *                                    REFLECTIONS
 * =================================================================================================
 */
vec3 calculateReflections(vec3 world_pos, vec3 normal, vec3 view_dir) {
    vec3 reflection_color = vec3(0.0);
    
    // Calculate reflection vector
    vec3 reflect_dir = reflect(-view_dir, normal);
    
    // Fresnel term
    float cos_theta = dot(view_dir, normal);
    float fresnel = fresnelReflection(cos_theta);
    
    // Screen-space reflections
    if (ubo.use_ssr) {
        // Sample SSR texture (simplified)
        vec2 ssr_uv = reflect_dir.xy * 0.5 + 0.5;
        vec3 ssr_color = texture(reflection_texture, ssr_uv).rgb;
        reflection_color += ssr_color * fresnel * ubo.reflection_strength;
    }
    
    // Planar reflections
    if (ubo.use_planar_reflection) {
        vec3 planar_color = texture(reflection_texture, in_texcoord).rgb;
        reflection_color += planar_color * fresnel * ubo.reflection_strength * 0.5;
    }
    
    // Environment reflections
    vec3 env_color = texture(environment_texture, reflect_dir).rgb;
    reflection_color += env_color * fresnel * ubo.reflection_strength * 0.3;
    
    return reflection_color;
}

/* =================================================================================================
 *                                    REFRACTION
 * =================================================================================================
 */
vec3 calculateRefraction(vec3 world_pos, vec3 normal, vec3 view_dir) {
    vec3 refracted_color = vec3(0.0);
    
    // Simple refraction with chromatic aberration
    float ior = 1.333; // Water refractive index
    vec3 refract_dir = refract(-view_dir, normal, 1.0 / ior);
    
    if (length(refract_dir) > 0.001) {
        // Sample refraction texture with distortion
        vec2 distorted_uv = in_texcoord + refract_dir.xy * ubo.distortion_strength;
        distorted_uv += texture(noise_texture, in_texcoord + ubo.time * 0.1).xy * 0.01;
        
        refracted_color = texture(refraction_texture, distorted_uv).rgb;
        
        // Apply absorption based on depth
        vec3 absorption_factor = exp(-ubo.absorption * in_depth);
        refracted_color *= absorption_factor;
    }
    
    return refracted_color * ubo.refraction_strength;
}

/* =================================================================================================
 *                                    CAUSTICS
 * =================================================================================================
 */
vec3 calculateCaustics(vec3 world_pos) {
    if (!ubo.enable_caustics) {
        return vec3(1.0);
    }
    
    // Animated caustics pattern
    vec2 caustics_uv = world_pos.xz * ubo.caustics_scale;
    caustics_uv += vec2(sin(ubo.time * ubo.caustics_speed), cos(ubo.time * ubo.caustics_speed * 1.3)) * 0.1;
    
    float caustics = texture(caustics_texture, caustics_uv).r;
    caustics = pow(caustics, 2.0); // Enhance contrast
    
    return vec3(1.0) + vec3(caustics) * ubo.caustics_strength;
}

/* =================================================================================================
 *                                    UNDERWATER EFFECTS
 * =================================================================================================
 */
bool isCameraUnderwater() {
    return ubo.camera_pos.y < 0.0; // Assuming sea level is at y=0
}

vec3 underwaterFog(vec3 color, float distance) {
    if (!ubo.enable_underwater || !isCameraUnderwater()) {
        return color;
    }
    
    float fog_factor = 1.0 - exp(-distance * ubo.underwater_fog_density);
    
    // Add god rays effect
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3)); // Simplified sun direction
    float god_rays = pow(max(dot(in_view_dir, -light_dir), 0.0), 8.0);
    
    vec3 fog_color = ubo.underwater_fog_color + god_rays * 0.3;
    
    return mix(color, fog_color, fog_factor);
}

/* =================================================================================================
 *                                    FOAM RENDERING
 * =================================================================================================
 */
vec3 renderFoam(vec3 base_color, float foam_factor) {
    if (foam_factor <= 0.001) {
        return base_color;
    }
    
    // Sample foam texture
    vec2 foam_uv = in_texcoord * ubo.foam_scale + vec2(ubo.time * 0.02);
    vec3 foam_pattern = texture(foam_texture, foam_uv).rgb;
    
    // Blend foam with base color
    vec3 foam_color = mix(base_color, ubo.foam_color, foam_factor);
    foam_color = mix(foam_color, foam_pattern, foam_factor * 0.5);
    
    return foam_color;
}

/* =================================================================================================
 *                                    SCATTERING
 * =================================================================================================
 */
vec3 calculateScattering(vec3 view_dir, vec3 light_dir) {
    float cos_angle = dot(view_dir, light_dir);
    float scatter_factor = pow(max(cos_angle, 0.0), 2.0) * ubo.scattering;
    
    return ubo.scatter_color * scatter_factor;
}

/* =================================================================================================
 *                                    MAIN FRAGMENT SHADER
 * =================================================================================================
 */
void main() {
    // Calculate base water color based on depth
    vec4 water_color = depthColorBlend(in_depth);
    
    // Calculate reflections
    vec3 reflection_color = calculateReflections(in_world_pos, in_normal, in_view_dir);
    
    // Calculate refraction
    vec3 refraction_color = calculateRefraction(in_world_pos, in_normal, in_view_dir);
    
    // Calculate caustics
    vec3 caustics_color = calculateCaustics(in_world_pos);
    
    // Calculate scattering
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3)); // Simplified sun direction
    vec3 scattering_color = calculateScattering(in_view_dir, light_dir);
    
    // Combine base components
    vec3 final_color = water_color.rgb;
    final_color = mix(final_color, reflection_color, 0.3); // Blend reflections
    final_color += refraction_color * 0.2; // Add refraction
    final_color *= caustics_color; // Apply caustics
    final_color += scattering_color; // Add scattering
    
    // Apply foam
    final_color = renderFoam(final_color, in_foam);
    
    // Apply underwater fog
    float camera_distance = length(in_world_pos - ubo.camera_pos);
    final_color = underwaterFog(final_color, camera_distance);
    
    // Set final alpha
    float alpha = water_color.a;
    if (in_foam > 0.001) {
        alpha = mix(alpha, 1.0, in_foam); // Foam is fully opaque
    }
    
    out_color = vec4(final_color, alpha);
    
    // Output for deferred rendering
    out_normal = vec4(in_normal * 0.5 + 0.5, 1.0);
    out_roughness_metallic = vec4(0.02, 0.0, 0.0, 1.0); // Water is smooth, non-metallic
}
