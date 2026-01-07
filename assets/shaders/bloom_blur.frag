// assets/shaders/bloom_blur.frag
// Gaussian blur for bloom effect
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform BloomBlur {
    vec2 texelSize;
    float radius;
    float padding;
} params;

layout(location = 0) out vec4 outColor;

// Gaussian kernel weights
const float kernel[25] = float[](
    1.0/273.0,  4.0/273.0,  7.0/273.0,  4.0/273.0, 1.0/273.0,
    4.0/273.0, 16.0/273.0, 26.0/273.0, 16.0/273.0, 4.0/273.0,
    7.0/273.0, 26.0/273.0, 41.0/273.0, 26.0/273.0, 7.0/273.0,
    4.0/273.0, 16.0/273.0, 26.0/273.0, 16.0/273.0, 4.0/273.0,
    1.0/273.0,  4.0/273.0,  7.0/273.0,  4.0/273.0, 1.0/273.0
);

void main() {
    vec4 result = vec4(0.0);
    vec2 offset = vec2(0.0);
    int index = 0;

    // 5x5 Gaussian blur
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            offset = inUV + vec2(float(x), float(y)) * params.texelSize * params.radius;
            result += texture(inTexture, offset) * kernel[index];
            index++;
        }
    }

    outColor = result;
}
