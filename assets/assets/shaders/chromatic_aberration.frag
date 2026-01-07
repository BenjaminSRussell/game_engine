// assets/shaders/chromatic_aberration.frag
// Chromatic aberration effect shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform ChromaticParams {
    float offset;
    float padding1;
    float padding2;
    float padding3;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    // Offset from center for stronger aberration at screen edges
    vec2 centerUV = inUV - vec2(0.5);
    float aberration = params.offset * length(centerUV);

    // Sample RGB channels at different positions
    vec4 color = vec4(0.0);

    // Red channel (shifted right)
    color.r = texture(inTexture, inUV + vec2(aberration, 0.0)).r;

    // Green channel (no shift)
    color.g = texture(inTexture, inUV).g;

    // Blue channel (shifted left)
    color.b = texture(inTexture, inUV - vec2(aberration, 0.0)).b;

    // Preserve alpha
    color.a = texture(inTexture, inUV).a;

    outColor = color;
}
