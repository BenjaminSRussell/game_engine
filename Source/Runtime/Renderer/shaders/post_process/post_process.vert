#version 450 core

// Post-Processing Vertex Shader
// Pass-through shader for full-screen quad rendering

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vViewRay;

uniform vec2 uScreenSize;
uniform mat4 uInverseProjection;
uniform mat4 uInverseView;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
    
    // Calculate view ray for depth reconstruction
    vec4 clipSpace = vec4(aTexCoord * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjection * clipSpace;
    vViewRay = viewSpace.xy / viewSpace.w;
}
