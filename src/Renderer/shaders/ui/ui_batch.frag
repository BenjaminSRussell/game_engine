// ui_batch.frag
// UI batch rendering fragment shader
// Advanced 3D Rendering Engine

#version 450 core

// Input from vertex shader
layout(location = 0) in VS_IN {
    vec2 texCoord;
    vec4 color;
    vec3 position;
    vec2 normal;
} fs_in;

// Output
layout(location = 0) out vec4 outColor;

// Textures
layout(binding = 1) uniform sampler2D mainTexture;
layout(binding = 2) uniform sampler2D normalTexture;
layout(binding = 3) uniform sampler2D glowTexture;

// Material properties
layout(binding = 4) uniform MaterialBuffer {
    vec4 tintColor;
    vec4 emissionColor;
    float metallic;
    float roughness;
    float emissionStrength;
    float pad;
};

// SDF text rendering
float sampleSDF(sampler2D sdfTexture, vec2 uv, float threshold) {
    float sdf = texture(sdfTexture, uv).r;
    float distance = sdf - threshold;
    float alpha = smoothstep(-0.1, 0.1, distance);
    return alpha;
}

void main() {
    // Sample textures
    vec4 texColor = texture(mainTexture, fs_in.texCoord);
    vec4 normalData = texture(normalTexture, fs_in.texCoord);
    vec4 glowData = texture(glowTexture, fs_in.texCoord);

    // Apply tint color
    vec4 baseColor = texColor * fs_in.color * tintColor;

    // Normal mapping for effects
    vec3 normal = normalize(normalData.rgb * 2.0 - 1.0);

    // Emission/glow
    vec4 emission = glowData * emissionColor * emissionStrength;

    // Combine lighting
    vec4 finalColor = baseColor + emission;

    // Apply alpha
    finalColor.a = baseColor.a;

    // Output final color
    outColor = finalColor;
}
