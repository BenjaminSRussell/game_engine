// Bloom Post-Processing Shaders
// Multi-pass Gaussian blur with threshold extraction

// ============================================================================
// THRESHOLD EXTRACTION SHADER (Fragment)
// ============================================================================
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrInput;

layout(push_constant) uniform PushConstants {
    float threshold;    // Brightness threshold (e.g., 1.0)
    float softKnee;     // Soft threshold knee (e.g., 0.5)
} pc;

void main() {
    vec3 color = texture(hdrInput, fragUV).rgb;
    
    // Calculate luminance
    float brightness = max(color.r, max(color.g, color.b));
    
    // Soft threshold
    float knee = pc.threshold * pc.softKnee;
    float soft = brightness - pc.threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    
    float contribution = max(soft, brightness - pc.threshold);
    contribution /= max(brightness, 0.00001);
    
    outColor = vec4(color * contribution, 1.0);
}

// ============================================================================
// GAUSSIAN BLUR SHADER (Fragment) - Horizontal/Vertical Pass
// ============================================================================
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    vec2 direction;  // (1,0) for horizontal, (0,1) for vertical
    float spread;    // Blur spread multiplier
} pc;

// 9-tap Gaussian weights
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    vec3 result = texture(inputTexture, fragUV).rgb * weights[0];
    
    for(int i = 1; i < 5; i++) {
        vec2 offset = pc.direction * texelSize * float(i) * pc.spread;
        result += texture(inputTexture, fragUV + offset).rgb * weights[i];
        result += texture(inputTexture, fragUV - offset).rgb * weights[i];
    }
    
    outColor = vec4(result, 1.0);
}

// ============================================================================
// BLOOM COMPOSITE SHADER (Fragment)
// ============================================================================
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D originalScene;
layout(binding = 1) uniform sampler2D blurredBloom;

layout(push_constant) uniform PushConstants {
    float intensity;     // Bloom strength (e.g., 0.04)
    float dirtIntensity; // Lens dirt intensity (optional)
} pc;

void main() {
    vec3 hdrColor = texture(originalScene, fragUV).rgb;
    vec3 bloomColor = texture(blurredBloom, fragUV).rgb;
    
    // Additive blend with intensity control
    vec3 result = hdrColor + bloomColor * pc.intensity;
    
    outColor = vec4(result, 1.0);
}
