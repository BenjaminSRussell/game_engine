// ui_effects.frag
// UI effects rendering shader (gradients, shadows, glow)
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

// Effect type constants
const int EFFECT_NONE = 0;
const int EFFECT_GRADIENT = 1;
const int EFFECT_SHADOW = 2;
const int EFFECT_GLOW = 3;

// Effect parameters
layout(binding = 4) uniform EffectBuffer {
    int effectType;
    float effectIntensity;
    vec4 effectColor;
    vec4 effectParams;  // gradient start, angle, blur radius, etc.
};

// Gradient calculation
vec4 renderGradient(vec2 uv, vec4 startColor, vec4 endColor, float angle) {
    // Calculate gradient based on angle
    vec2 direction = vec2(cos(angle), sin(angle));
    float gradient = dot(uv - 0.5, direction);
    gradient = clamp(gradient + 0.5, 0.0, 1.0);

    return mix(startColor, endColor, gradient);
}

// Shadow calculation
vec4 renderShadow(vec4 baseColor, vec4 shadowColor, float intensity) {
    // Apply shadow offset and blur
    float shadow = intensity;
    return mix(baseColor, shadowColor, shadow);
}

// Glow calculation
vec4 renderGlow(vec4 baseColor, vec4 glowColor, float intensity) {
    // Add glow effect with intensity
    return baseColor + glowColor * intensity;
}

void main() {
    vec4 finalColor = fs_in.color;

    if (effectType == EFFECT_GRADIENT) {
        finalColor = renderGradient(fs_in.texCoord, fs_in.color, effectColor,
                                    effectParams.y);
    }
    else if (effectType == EFFECT_SHADOW) {
        finalColor = renderShadow(fs_in.color, effectColor, effectIntensity);
    }
    else if (effectType == EFFECT_GLOW) {
        finalColor = renderGlow(fs_in.color, effectColor, effectIntensity);
    }

    outColor = finalColor;
}
