#version 460
// FFT Ocean Surface Vertex Shader
// Applies displacement from FFT-generated textures

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_texcoord;

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;
layout(location = 3) out vec3 out_view_dir;
layout(location = 4) out float out_foam_factor;

layout(binding = 0) uniform GlobalParams {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    vec3 camera_pos;
    float time;
} global;

layout(binding = 1) uniform OceanFFTParams {
    float tile_size;          // Physical tile size
    float cascade_blend;      // Cascade blending factor
    uint cascade_count;       // Number of cascades
    float _padding;
} fft_params;

// FFT displacement textures (RGB32F: Dx, height, Dz)
layout(binding = 2) uniform sampler2D displacement_cascade_0;
layout(binding = 3) uniform sampler2D displacement_cascade_1;
layout(binding = 4) uniform sampler2D displacement_cascade_2;

// FFT normal maps (RGBA16F: Nx, Ny, Nz, foam)
layout(binding = 5) uniform sampler2D normal_cascade_0;
layout(binding = 6) uniform sampler2D normal_cascade_1;
layout(binding = 7) uniform sampler2D normal_cascade_2;

void main() {
    // Base world position (flat grid)
    vec3 world_pos = in_position;
    
    // Compute UV coordinates for texture sampling
    // Map world position to [0, 1] based on tile size
    vec2 uv = (world_pos.xz + fft_params.tile_size * 0.5) / fft_params.tile_size;
    
    // Sample displacement from cascade 0 (finest detail)
    vec3 displacement = texture(displacement_cascade_0, uv).rgb;
    
    // Apply displacement: (x + Dx, height, z + Dz)
    world_pos.x += displacement.x;  // Horizontal displacement X (choppy waves)
    world_pos.y = displacement.y;   // Height displacement
    world_pos.z += displacement.z;  // Horizontal displacement Z
    
    // Sample normal and foam
    vec4 normal_foam = texture(normal_cascade_0, uv);
    vec3 normal = normal_foam.rgb;
    float foam = normal_foam.a;
    
    // TODO: Add cascade blending based on distance from camera
    // This would sample other cascades and blend by weight
    
    // Output
    out_world_pos = world_pos;
    out_normal = normalize(normal);
    out_texcoord = uv;
    out_view_dir = normalize(global.camera_pos - world_pos);
    out_foam_factor = foam;
    
    gl_Position = global.view_projection * vec4(world_pos, 1.0);
}
