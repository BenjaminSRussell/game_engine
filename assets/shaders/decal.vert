#version 450

layout(location = 0) in vec3 inPosition;

// Global Uniforms (Set 0)
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 projection;
    mat4 view;
    mat4 invProjection;
    mat4 invView;
    vec2 screenSize;
} global;

// Instance Uniforms (Set 2 - using Push Constants or Dynamic Uniform for per-decal?)
// For batching, we might use instance data. For now, assume Push Constant for Model Matrix
layout(push_constant) uniform PushConsts {
    mat4 model;
    mat4 invModel; // Needed for projecting world pos -> decal space
    vec4 colorTint; // and other params
} push;

layout(location = 0) out vec4 outClipPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    vec4 clipPos = global.projection * global.view * worldPos;
    
    outClipPos = clipPos;
    gl_Position = clipPos;
}
