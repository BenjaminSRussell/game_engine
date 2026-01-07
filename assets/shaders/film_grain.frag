// assets/shaders/film_grain.frag
// Film grain effect shader
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D inTexture;

layout(push_constant) uniform FilmGrainParams {
    float intensity;
    float time;
    float padding1;
    float padding2;
} params;

layout(location = 0) out vec4 outColor;

// Pseudo-random number generator
float random(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233)) + params.time) * 43758.5453);
}

void main() {
    vec4 color = texture(inTexture, inUV);

    // Generate grain pattern
    float grain = random(inUV + params.time);
    grain = (grain - 0.5) * 2.0;  // Remap to [-1, 1]

    // Apply grain with intensity
    color.rgb += grain * params.intensity;

    outColor = color;
}
