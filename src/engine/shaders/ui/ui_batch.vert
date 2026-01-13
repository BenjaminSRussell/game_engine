// ui_batch.vert
// UI batch rendering vertex shader
// Advanced 3D Rendering Engine

#version 450 core

// Input vertex data
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 normal;

// Output to fragment shader
layout(location = 0) out VS_OUT {
    vec2 texCoord;
    vec4 color;
    vec3 position;
    vec2 normal;
} vs_out;

// Uniform buffer for transforms
layout(binding = 0) uniform TransformBuffer {
    mat4 projection;
    mat4 view;
    mat4 model;
    vec4 viewport;
};

void main() {
    // Transform position to clip space
    vec4 worldPos = model * vec4(position, 1.0);
    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;

    // Pass data to fragment shader
    vs_out.texCoord = texCoord;
    vs_out.color = color;
    vs_out.position = position;
    vs_out.normal = normal;
}
