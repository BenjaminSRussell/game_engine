#version 460
// location = 0: Position
// location = 1: Normal
// location = 2: Tangent
// location = 3: TexCoord

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_texcoord;

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;
layout(location = 3) out vec3 out_view_pos;

layout(binding = 0) uniform GlobalParams {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    vec3 camera_pos;
    float time;
} global;

layout(binding = 1) uniform OceanParams {
    float wave_amplitude;
    float wave_frequency;
    float wave_speed;
    float height_scale;
} ocean;

void main() {
    vec3 world_pos = in_position;
    
    // Simple verification wave
    float wave = sin(world_pos.x * ocean.wave_frequency + global.time * ocean.wave_speed) * 
                 cos(world_pos.z * ocean.wave_frequency + global.time * ocean.wave_speed * 0.8);
    world_pos.y += wave * ocean.wave_amplitude;

    out_world_pos = world_pos;
    out_normal = in_normal; // In real implementation, recompute normal
    out_texcoord = in_texcoord;
    out_view_pos = global.camera_pos;

    gl_Position = global.view_projection * vec4(world_pos, 1.0);
}
