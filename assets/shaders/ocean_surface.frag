#version 460
// FFT Ocean Surface Fragment Shader
// Enhanced water shading with Fresnel, reflections, and foam

layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_view_dir;
layout(location = 4) in float in_foam_factor;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform GlobalParams {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    vec3 camera_pos;
    float time;
} global;

layout(binding = 2) uniform LightParams {
    vec3 sun_direction;
    vec3 sun_color;
    vec3 ambient_color;
} light;

// Optional: Skybox/environment map for reflections
layout(binding = 8) uniform samplerCube environment_map;

// Optional: Foam texture
layout(binding = 9) uniform sampler2D foam_texture;

void main() {
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_view_dir);
    vec3 L = normalize(-light.sun_direction);
    vec3 H = normalize(L + V);
    
    // Water base color (deep blue-green)
    vec3 water_albedo = vec3(0.0, 0.15, 0.3);
    float water_roughness = 0.05;  // Very smooth
    
    // Fresnel effect (Schlick approximation)
    float F0 = 0.02;  // Water's base reflectance at normal incidence
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
    
    // Specular highlight (sun reflection)
    float NdotH = max(dot(N, H), 0.0);
    float specular_power = 1.0 / (water_roughness * water_roughness);
    float specular = pow(NdotH, specular_power * 128.0);
    
    // Reflection vector for environment
    vec3 R = reflect(-V, N);
    
    // Sample environment map (or use simple sky color)
    vec3 sky_color = vec3(0.5, 0.7, 1.0);  // Fallback sky
    
    // If environment_map is bound, sample it
    // vec3 reflection_color = texture(environment_map, R).rgb;
    vec3 reflection_color = sky_color;  // Use simple sky for now
    
    // Diffuse lighting (minimal for water)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = water_albedo * NdotL;
    
    // Combine lighting
    vec3 color = vec3(0.0);
    
    // Ambient
    color += water_albedo * light.ambient_color;
    
    // Diffuse (less important for water)
    color += diffuse * light.sun_color * 0.3;
    
    // Specular highlight from sun
    color += specular * light.sun_color;
    
    // Fresnel reflection
    color += fresnel * reflection_color;
    
    // Foam rendering
    if (in_foam_factor > 0.01) {
        // Sample foam texture (or use procedural)
        vec2 foam_uv = in_texcoord * 10.0 + global.time * 0.1;
        // float foam_tex = texture(foam_texture, foam_uv).r;
        float foam_tex = 0.8;  // Fallback constant
        
        vec3 foam_color = vec3(1.0, 1.0, 1.0) * foam_tex;
        
        // Blend foam on top
        color = mix(color, foam_color, in_foam_factor * 0.7);
    }
    
    // Water transparency (slight, mostly for shallow water)
    float alpha = 0.95 + fresnel * 0.05;  // Nearly opaque, slightly more transparent at grazing angles
    
    out_color = vec4(color, alpha);
}
