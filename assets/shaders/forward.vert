#version 450 core
/*
 * forward.vert
 * General purpose forward rendering vertex shader.
 * Supports static meshes, skinning, and instancing via defines.
 */

// =============================================================================
// INPUTS
// =============================================================================
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec4 v_tangent; // xyz = tangent, w = handedness
layout(location = 4) in vec4 v_color;   // Vertex color

#ifdef FEATURE_SKINNING
layout(location = 5) in vec4 v_weights;
layout(location = 6) in uvec4 v_joints;
#endif

// =============================================================================
// UNIFORMS / BUFFERS
// =============================================================================

layout(std140, set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view_projection;
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    float time;
};

layout(push_constant) uniform PushConstants {
    mat4 model_matrix;
    // Material ID or other PC data could go here
};

// =============================================================================
// OUTPUTS
// =============================================================================
layout(location = 0) out vec3 f_world_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec2 f_uv;
layout(location = 3) out vec4 f_tangent;
layout(location = 4) out vec4 f_color;

// =============================================================================
// MAIN
// =============================================================================
void main() {
    vec4 world_pos = model_matrix * vec4(v_position, 1.0);
    
    // TODDO: Implement skinning logic if FEATURE_SKINNING is defined
    
    f_world_pos = world_pos.xyz;
    f_normal = mat3(model_matrix) * v_normal; // Note: Use inverse-transpose for non-uniform scaling
    f_uv = v_uv;
    f_tangent = v_tangent;
    f_color = v_color;
    
    gl_Position = view_projection * world_pos;
}
