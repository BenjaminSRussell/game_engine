// Example PBR Shader - Base Template
// This shader uses permutation defines to enable/disable features

// NOTE: #version and defines are injected by shader_generate_source()

// === Vertex Shader Inputs ===
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

#ifdef FEATURE_VERTEX_COLORS
layout(location = 3) in vec4 a_color;
#endif

#ifdef FEATURE_NORMAL_MAP
layout(location = 4) in vec3 a_tangent;
#endif

// === Outputs ===
out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_uv;

#ifdef FEATURE_VERTEX_COLORS
out vec4 v_color;
#endif

#ifdef FEATURE_NORMAL_MAP
out mat3 v_tbn;
#endif

// === Uniforms ===
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;

void main() {
    // Transform position
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    gl_Position = u_projection * u_view * world_pos;
    
    // Transform normal
    v_normal = normalize(u_normal_matrix * a_normal);
    v_uv = a_uv;
    
    #ifdef FEATURE_VERTEX_COLORS
    v_color = a_color;
    #endif
    
    #ifdef FEATURE_NORMAL_MAP
    vec3 T = normalize(u_normal_matrix * a_tangent);
    vec3 N = v_normal;
    vec3 B = cross(N, T);
    v_tbn = mat3(T, B, N);
    #endif
}
