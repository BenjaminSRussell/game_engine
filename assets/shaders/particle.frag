// assets/shaders/particle.frag
// Particle fragment shader with texture sampling and blending
#version 450

layout(location = 0) in VS_IN {
    vec2 uv;
    vec4 color;
    float depth;
} fs_in;

layout(set = 0, binding = 0) uniform sampler2D particleTexture;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample particle texture
    vec4 texColor = texture(particleTexture, fs_in.uv);

    // Alpha test - discard fully transparent pixels
    if (texColor.a < 0.01) {
        discard;
    }

    // Combine particle color with texture
    vec4 finalColor = fs_in.color * texColor;

    // Apply fade based on alpha
    finalColor.a = fs_in.color.a * texColor.a;

    // Additive blending for glow effects
    // Can be controlled by particle type
    outColor = finalColor;
}
