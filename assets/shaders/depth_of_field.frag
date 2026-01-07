// assets/shaders/depth_of_field.frag
// Depth of field effect shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D colorTexture;
layout(set = 0, binding = 1) uniform sampler2D depthTexture;

layout(push_constant) uniform DOFParams {
    float focusDistance;
    float focusRange;
    float maxBlur;
    float padding;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(colorTexture, inUV);
    float depth = texture(depthTexture, inUV).r;

    // Calculate circle of confusion (CoC)
    float depthDiff = abs(depth - params.focusDistance);
    float coc = (depthDiff - params.focusRange / 2.0) / params.focusRange;
    coc = clamp(coc, 0.0, 1.0);

    // Blur radius based on CoC
    float blurRadius = coc * params.maxBlur;

    // Simple box blur for now (can be improved with better blur)
    vec4 blurred = vec4(0.0);
    float samples = 0.0;

    for (float x = -blurRadius; x <= blurRadius; x += 1.0) {
        for (float y = -blurRadius; y <= blurRadius; y += 1.0) {
            vec2 offset = vec2(x, y) / vec2(textureSize(colorTexture, 0));
            blurred += texture(colorTexture, inUV + offset);
            samples += 1.0;
        }
    }

    blurred /= samples;

    // Blend between sharp and blurred based on CoC
    outColor = mix(color, blurred, coc);
}
