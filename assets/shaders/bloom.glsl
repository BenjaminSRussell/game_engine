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
// DOWNSAMPLE / UPSAMPLE TENT FILTER (Fragment)
// ============================================================================
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    vec2 texelSize; // 1.0 / inputTextureSize
    float filterRadius;
} pc;

void main() {
    // 13-tap tent filter (Karis 2013)
    // A B C
    // D E F
    // G H I
    // J K L
    // M
    
    float x = pc.texelSize.x;
    float y = pc.texelSize.y;

    // Take 13 samples as specified in Karis 2013 "Physically Based Shading in Unreal Engine 4"
    vec3 a = texture(inputTexture, vec2(fragUV.x - 2*x, fragUV.y + 2*y)).rgb;
    vec3 b = texture(inputTexture, vec2(fragUV.x,       fragUV.y + 2*y)).rgb;
    vec3 c = texture(inputTexture, vec2(fragUV.x + 2*x, fragUV.y + 2*y)).rgb;

    vec3 d = texture(inputTexture, vec2(fragUV.x - 2*x, fragUV.y)).rgb;
    vec3 e = texture(inputTexture, vec2(fragUV.x,       fragUV.y)).rgb;
    vec3 f = texture(inputTexture, vec2(fragUV.x + 2*x, fragUV.y)).rgb;

    vec3 g = texture(inputTexture, vec2(fragUV.x - 2*x, fragUV.y - 2*y)).rgb;
    vec3 h = texture(inputTexture, vec2(fragUV.x,       fragUV.y - 2*y)).rgb;
    vec3 i = texture(inputTexture, vec2(fragUV.x + 2*x, fragUV.y - 2*y)).rgb;

    vec3 j = texture(inputTexture, vec2(fragUV.x - x, fragUV.y + y)).rgb;
    vec3 k = texture(inputTexture, vec2(fragUV.x + x, fragUV.y + y)).rgb;
    vec3 l = texture(inputTexture, vec2(fragUV.x - x, fragUV.y - y)).rgb;
    vec3 m = texture(inputTexture, vec2(fragUV.x + x, fragUV.y - y)).rgb;

    vec3 result = e * 0.125;
    result += (a + c + g + i) * 0.03125;
    result += (b + d + f + h) * 0.0625;
    result += (j + k + l + m) * 0.125;

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
    vec3 colorShift;     // Bloom color tint (default 1,1,1)
} pc;

void main() {
    vec3 hdrColor = texture(originalScene, fragUV).rgb;
    vec3 bloomColor = texture(blurredBloom, fragUV).rgb;
    
    // Additive blend with intensity and color shift
    vec3 result = hdrColor + bloomColor * pc.intensity * pc.colorShift;
    
    outColor = vec4(result, 1.0);
}
