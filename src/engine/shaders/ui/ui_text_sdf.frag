// ui_text_sdf.frag
// SDF text rendering shader
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

// SDF Atlas texture
layout(binding = 1) uniform sampler2D sdfAtlas;

// SDF parameters
layout(binding = 4) uniform SDFBuffer {
    float sdfScale;
    float sdfThreshold;
    float outlineWidth;
    float outlineAlpha;
    vec4 outlineColor;
    float pad0, pad1, pad2;
};

void main() {
    // Sample SDF value
    float sdf = texture(sdfAtlas, fs_in.texCoord).r;

    // Apply scale
    float scaledSDF = (sdf - 0.5) * sdfScale + 0.5;

    // Calculate main text alpha with smoothstep for antialiasing
    float mainAlpha = smoothstep(sdfThreshold - 0.1, sdfThreshold + 0.1, scaledSDF);

    // Calculate outline alpha
    float outlineAlpha = smoothstep(sdfThreshold - outlineWidth - 0.1,
                                      sdfThreshold - outlineWidth + 0.1,
                                      scaledSDF);
    float outlineOnly = outlineAlpha - mainAlpha;

    // Blend colors
    vec4 textColor = fs_in.color;
    vec4 outline = outlineColor * outlineOnly;

    vec4 finalColor = mix(outline, textColor, mainAlpha);
    finalColor.a = max(mainAlpha, outlineAlpha);

    outColor = finalColor;
}
