// assets/shaders/bloom_composite.frag
// Composite bloom back into main image
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D originalTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;

layout(push_constant) uniform BloomComposite {
    float bloomIntensity;
    float bloomSoftness;
    float padding1;
    float padding2;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 original = texture(originalTexture, inUV);
    vec4 bloom = texture(bloomTexture, inUV);

    // Add bloom with soft blending
    vec3 bloomColor = bloom.rgb * params.bloomIntensity;

    // Apply softness curve
    float bloomMask = length(bloomColor) * params.bloomSoftness;
    bloomMask = smoothstep(0.0, 1.0, bloomMask);

    // Additive bloom blending
    vec3 result = original.rgb + bloomColor * bloomMask;

    outColor = vec4(result, original.a);
}
