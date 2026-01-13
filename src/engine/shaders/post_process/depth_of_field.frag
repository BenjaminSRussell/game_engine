#version 450 core

// Depth of Field Fragment Shader
// Implements physically-based depth of field with bokeh effects

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vViewRay;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform float uFocusDistance;
uniform float uFocusRange;
uniform float uMaxBlur;
uniform float uAperture;
uniform bool uAutoFocus;
uniform vec2 uFocusPoint;
uniform int uBokehShape; // 0=Circle, 1=Hexagon, 2=Octagon

// Calculate circle of confusion
float calculateCoC(float depth, float focusDistance, float focusRange) {
    float coc = abs(depth - focusDistance) / focusRange;
    return coc * uMaxBlur;
}

// Hexagonal bokeh sampling
vec3 hexagonalBokeh(sampler2D tex, vec2 uv, float radius) {
    vec3 color = vec3(0.0);
    float samples = 0.0;
    
    const float PI = 3.14159265;
    const int sides = 6;
    
    for (int i = 0; i < sides; i++) {
        float angle = 2.0 * PI * float(i) / float(sides);
        vec2 offset = vec2(cos(angle), sin(angle)) * radius;
        color += texture(tex, uv + offset).rgb;
        samples += 1.0;
    }
    
    return color / samples;
}

// Circular bokeh sampling
vec3 circularBokeh(sampler2D tex, vec2 uv, float radius) {
    vec3 color = vec3(0.0);
    float samples = 0.0;
    
    const int numSamples = 8;
    for (int i = 0; i < numSamples; i++) {
        float angle = 2.0 * 3.14159265 * float(i) / float(numSamples);
        vec2 offset = vec2(cos(angle), sin(angle)) * radius;
        color += texture(tex, uv + offset).rgb;
        samples += 1.0;
    }
    
    return color / samples;
}

// Get world-space depth from depth buffer
float getWorldDepth(vec2 uv) {
    float depth = texture(uDepthTexture, uv).r;
    return depth;
}

void main() {
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    float depth = getWorldDepth(vTexCoord);
    
    // Calculate circle of confusion
    float coc = calculateCoC(depth, uFocusDistance, uFocusRange);
    
    if (coc > 0.5) {
        // Apply bokeh blur
        vec3 blurredColor;
        if (uBokehShape == 1) {
            blurredColor = hexagonalBokeh(uColorTexture, vTexCoord, coc);
        } else {
            blurredColor = circularBokeh(uColorTexture, vTexCoord, coc);
        }
        
        // Blend between sharp and blurred based on CoC
        float blurAmount = smoothstep(0.0, 1.0, coc);
        color = mix(color, blurredColor, blurAmount);
    }
    
    // Add subtle vignetting for cinematic effect
    vec2 center = vec2(0.5);
    float dist = distance(vTexCoord, center);
    float vignette = 1.0 - smoothstep(0.7, 1.0, dist);
    color *= vignette;
    
    fragColor = vec4(color, 1.0);
}
