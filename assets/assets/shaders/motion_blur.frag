// assets/shaders/motion_blur.frag
// Motion blur effect shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D currentFrame;
layout(set = 0, binding = 1) uniform sampler2D previousFrame;

layout(push_constant) uniform MotionBlurParams {
    vec2 velocity;
    float intensity;
    uint sampleCount;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    // Sample motion trail
    for (uint i = 0; i < min(params.sampleCount, 16u); i++) {
        float t = float(i) / float(params.sampleCount);

        // Interpolate between frames
        float frameBlend = t;
        vec2 offset = params.velocity * params.intensity * (t - 0.5);

        // Sample from interpolated frame
        vec4 sample0 = texture(currentFrame, inUV + offset);
        vec4 sample1 = texture(previousFrame, inUV + offset);
        vec4 sample = mix(sample0, sample1, frameBlend);

        // Weight samples by temporal position
        float weight = 1.0 - abs(t - 0.5) * 2.0;
        result += sample * weight;
        totalWeight += weight;
    }

    outColor = result / totalWeight;
}
