// Tonemapping and Color Grading Shader
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sceneColor;   // HDR scene
layout(binding = 1) uniform sampler2D bloomColor;   // Bloom (additive)
layout(binding = 2) uniform sampler3D colorGradingLUT; // 3D LUT

layout(push_constant) uniform PushConstants {
    int tonemapOperator; // 0: ACES, 1: Reinhard, 2: Filmic, 3: Linear
    float exposure;      // Exposure multiplier
    float bloomIntensity;
    float contrast;      // default 1.0
    float saturation;    // default 1.0
    float gamma;         // default 2.2
    bool useLUT;
} pc;

// ============================================================================
// OPERATORS
// ============================================================================

// ACES Tone Mapping (Narkowicz 2015)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reinhard Tone Mapping
vec3 Reinhard(vec3 x) {
    return x / (1.0 + x);
}

// Filmic Tone Mapping (Uncharted 2)
vec3 Filmic(vec3 x) {
    vec3 X = max(vec3(0.0), x - 0.004);
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, vec3(2.2)); // Correct for implicit gamma in formula
}

// ============================================================================
// MAIN
// ============================================================================

void main() {
    vec3 color = texture(sceneColor, fragUV).rgb;
    vec3 bloom = texture(bloomColor, fragUV).rgb;
    
    // 1. Add Bloom
    color += bloom * pc.bloomIntensity;
    
    // 2. Exposure
    color *= pc.exposure;
    
    // 3. Tone Mapping
    vec3 mapped;
    if (pc.tonemapOperator == 0) mapped = ACESFilm(color);
    else if (pc.tonemapOperator == 1) mapped = Reinhard(color);
    else if (pc.tonemapOperator == 2) mapped = Filmic(color);
    else mapped = color; // Linear
    
    // 4. Contrast
    mapped = mix(vec3(0.5), mapped, pc.contrast);
    
    // 5. Saturation
    float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(luma), mapped, pc.saturation);
    
    // 6. Color Grading (LUT)
    if (pc.useLUT) {
        mapped = texture(colorGradingLUT, mapped).rgb;
    }
    
    // 7. Gamma Correction
    mapped = pow(mapped, vec3(1.0 / pc.gamma));
    
    outColor = vec4(mapped, 1.0);
}
