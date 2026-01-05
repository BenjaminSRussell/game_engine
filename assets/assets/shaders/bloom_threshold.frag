// assets/shaders/bloom_threshold.frag
// Extract bright pixels for bloom effect
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform BloomThreshold {
    float threshold;
    float padding1;
    float padding2;
    float padding3;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(inTexture, inUV);

    // Calculate brightness
    float brightness = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    // Threshold and tone mapping
    if (brightness > params.threshold) {
        // Soft knee for smooth transition
        float soft = smoothstep(params.threshold - 0.1, params.threshold + 0.1, brightness);
        outColor = vec4(color.rgb * soft, color.a);
    } else {
        outColor = vec4(0.0);
    }
}
