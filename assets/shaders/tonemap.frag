// assets/shaders/tonemap.frag
// Tone mapping and color correction shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform TonemapParams {
    float exposure;
    float gamma;
    float contrast;
    float saturation;
} params;

layout(location = 0) out vec4 outColor;

// Reinhard tone mapping
vec3 reinhard_tonemap(vec3 color) {
    return color / (color + vec3(1.0));
}

// ACES tone mapping (more cinematic)
vec3 aces_tonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec4 color = texture(inTexture, inUV);

    // Apply exposure
    color.rgb *= pow(2.0, params.exposure);

    // Tone mapping
    color.rgb = aces_tonemap(color.rgb);

    // Gamma correction
    color.rgb = pow(color.rgb, vec3(1.0 / params.gamma));

    // Contrast
    color.rgb = mix(vec3(0.5), color.rgb, params.contrast);

    // Saturation
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = mix(vec3(luma), color.rgb, params.saturation);

    outColor = color;
}
