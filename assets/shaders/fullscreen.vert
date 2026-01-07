// assets/shaders/fullscreen.vert
// Fullscreen triangle for post-processing passes
#version 450

layout(location = 0) out vec2 outUV;

void main() {
    // Generate fullscreen triangle
    // Covers entire screen with UV coordinates
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outUV = uv;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
