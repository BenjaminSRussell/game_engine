#version 450 core

// Anti-Aliasing Fragment Shader
// Implements FXAA and TAA with temporal filtering

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uPreviousColorTexture;
uniform sampler2D uVelocityTexture;
uniform int uAAMethod; // 0=None, 1=FXAA, 2=TAA
uniform float uFXAASubpixelQuality;
uniform float uFXAAEdgeThreshold;
uniform float uFXAAEdgeThresholdMin;
uniform float uTAABlendFactor;
uniform float uTAASharpening;

// FXAA quality settings
#define FXAA_QUALITY_PS 0
#define FXAA_QUALITY_P0 1.0
#define FXAA_QUALITY_P1 1.5
#define FXAA_QUALITY_P2 2.0
#define FXAA_QUALITY_P3 4.0
#define FXAA_QUALITY_P4 8.0

// FXAA implementation
vec3 applyFXAA(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * texelSize).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * texelSize).rgb;
    
    vec3 rgbM = texture(tex, uv).rgb;
    
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    float lumaM = dot(rgbM, vec3(0.299, 0.587, 0.114));
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    float lumaRange = lumaMax - lumaMin;
    
    if (lumaRange < max(uFXAAEdgeThresholdMin, lumaMax * uFXAAEdgeThreshold)) {
        return rgbM;
    }
    
    // Edge detection
    vec2 dir;
    dir.x = -(lumaNW + lumaNE) - (lumaSW + lumaSE);
    dir.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * uFXAASubpixelQuality), uFXAAEdgeThresholdMin);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_QUALITY_P0, FXAA_QUALITY_P0), max(vec2(FXAA_QUALITY_P1, FXAA_QUALITY_P1), dir * rcpDirMin)) * texelSize;
    
    vec3 rgbA = 0.5 * (
        texture(tex, uv + dir * (1.0/3.0 - 0.5)).rgb +
        texture(tex, uv + dir * (2.0/3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, uv + dir * -0.5).rgb +
        texture(tex, uv + dir * 0.5).rgb);
    
    float lumaB = dot(rgbB, vec3(0.299, 0.587, 0.114));
    
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return rgbA;
    } else {
        return rgbB;
    }
}

// TAA implementation
vec3 applyTAA(sampler2D currentTex, sampler2D previousTex, sampler2D velocityTex, 
               vec2 uv, vec2 texelSize) {
    // Get current frame color
    vec3 currentColor = texture(currentTex, uv).rgb;
    
    // Get velocity for this pixel
    vec2 velocity = texture(velocityTex, uv).rg;
    
    // Reproject previous frame position
    vec2 previousUV = uv - velocity;
    
    // Check if previous UV is valid
    if (previousUV.x >= 0.0 && previousUV.x <= 1.0 &&
        previousUV.y >= 0.0 && previousUV.y <= 1.0) {
        
        // Sample previous frame with neighborhood clamping
        vec3 previousColor = texture(previousTex, previousUV).rgb;
        
        // Neighborhood clamping to prevent ghosting
        vec3 minColor = min(min(
            texture(currentTex, uv + vec2(-texelSize.x, 0.0)).rgb,
            texture(currentTex, uv + vec2(texelSize.x, 0.0)).rgb),
            min(texture(currentTex, uv + vec2(0.0, -texelSize.y)).rgb,
            texture(currentTex, uv + vec2(0.0, texelSize.y)).rgb));
        
        vec3 maxColor = max(max(
            texture(currentTex, uv + vec2(-texelSize.x, 0.0)).rgb,
            texture(currentTex, uv + vec2(texelSize.x, 0.0)).rgb),
            max(texture(currentTex, uv + vec2(0.0, -texelSize.y)).rgb,
            texture(currentTex, uv + vec2(0.0, texelSize.y)).rgb));
        
        previousColor = clamp(previousColor, minColor, maxColor);
        
        // Blend current and previous
        currentColor = mix(currentColor, previousColor, uTAABlendFactor);
    }
    
    // Apply sharpening
    vec3 sharpened = currentColor;
    if (uTAASharpening > 0.0) {
        vec3 neighbors = 
            texture(currentTex, uv + vec2(-texelSize.x, 0.0)).rgb +
            texture(currentTex, uv + vec2(texelSize.x, 0.0)).rgb +
            texture(currentTex, uv + vec2(0.0, -texelSize.y)).rgb +
            texture(currentTex, uv + vec2(0.0, texelSize.y)).rgb;
        
        sharpened = currentColor + (currentColor - neighbors * 0.25) * uTAASharpening;
    }
    
    return sharpened;
}

// SMAA implementation (simplified)
vec3 applySMAA(sampler2D tex, vec2 uv, vec2 texelSize) {
    // This is a simplified SMAA 1x implementation
    // In production, you'd want the full SMAA implementation with edge detection, blending, etc.
    
    vec3 color = texture(tex, uv).rgb;
    
    // Simple edge detection
    vec3 hL = texture(tex, uv + vec2(-texelSize.x, 0.0)).rgb;
    vec3 hR = texture(tex, uv + vec2(texelSize.x, 0.0)).rgb;
    vec3 vT = texture(tex, uv + vec2(0.0, -texelSize.y)).rgb;
    vec3 vB = texture(tex, uv + vec2(0.0, texelSize.y)).rgb;
    
    float hDelta = abs(dot(hL, vec3(0.299, 0.587, 0.114)) - dot(hR, vec3(0.299, 0.587, 0.114)));
    float vDelta = abs(dot(vT, vec3(0.299, 0.587, 0.114)) - dot(vB, vec3(0.299, 0.587, 0.114)));
    
    float edge = max(hDelta, vDelta);
    
    if (edge > 0.1) {
        // Apply blur on edges
        color = (color + hL + hR + vT + vB) * 0.125;
    }
    
    return color;
}

void main() {
    vec2 texelSize = 1.0 / textureSize(uColorTexture, 0);
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    
    switch (uAAMethod) {
        case 1: // FXAA
            color = applyFXAA(uColorTexture, vTexCoord, texelSize);
            break;
        case 2: // TAA
            color = applyTAA(uColorTexture, uPreviousColorTexture, uVelocityTexture, vTexCoord, texelSize);
            break;
        case 3: // SMAA
            color = applySMAA(uColorTexture, vTexCoord, texelSize);
            break;
        default: // No AA
            break;
    }
    
    fragColor = vec4(color, 1.0);
}
