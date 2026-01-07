#version 330 core

in vec3 v_frag_pos;
in vec3 v_normal;
in vec2 v_texcoord;

uniform vec3 u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;

uniform vec3 u_light_pos;
uniform vec3 u_view_pos;

out vec4 FragColor;

const float PI = 3.14159265359;

void main() {
    // Simple Lambertian for now to verify pipeline
    vec3 norm = normalize(v_normal);
    vec3 light_dir = normalize(u_light_pos - v_frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * u_albedo;
    
    vec3 ambient = 0.1 * u_albedo * u_ao;
    vec3 result = ambient + diffuse;
    
    FragColor = vec4(result, 1.0);
}
