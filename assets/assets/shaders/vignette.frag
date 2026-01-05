// assets/shaders/vignette.frag
// Vignette effect shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform VignetteParams {
    float strength;
    float radius;
    float smoothness;
    float padding;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(inTexture, inUV);

    // Calculate distance from center
    vec2 centerUV = inUV - vec2(0.5);
    float distFromCenter = length(centerUV);

    // Vignette falloff
    float vignette = 1.0 - smoothstep(
        params.radius - params.smoothness,
        params.radius + params.smoothness,
        distFromCenter
    );

    // Blend vignette with original
    vec3 vignetteColor = mix(
        vec3(0.0),
        color.rgb,
        mix(1.0, vignette, params.strength)
    );

    outColor = vec4(vignetteColor, color.a);
}
