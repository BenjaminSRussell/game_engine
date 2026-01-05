#version 460

layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_view_pos;

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

void main() {
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_view_pos - in_world_pos);
    vec3 L = normalize(-light.sun_direction);
    vec3 H = normalize(L + V);

    // Basic PBR-like water shading
    vec3 albedo = vec3(0.0, 0.1, 0.3); // Deep blue
    float roughness = 0.1;
    float metallic = 0.0;
    
    // Specular
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, 128.0);
    
    // Fresnel
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = 0.02 + (1.0 - 0.02) * pow(1.0 - NdotV, 5.0);
    
    // Combine
    vec3 color = albedo * light.ambient_color + light.sun_color * specular + fresnel * vec3(0.5, 0.7, 1.0); // Simple sky reflection approximation

    out_color = vec4(color, 0.9); // Slight transparency
}
