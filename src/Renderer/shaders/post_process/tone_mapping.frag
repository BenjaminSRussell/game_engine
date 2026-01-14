#version 450 core

// Tone Mapping Fragment Shader
// Implements various tone mapping operators for HDR to LDR conversion

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uHDRTexture;
uniform int uToneMapOperator; // 0=Linear, 1=Reinhard, 2=ACES, 3=Uncharted2, 4=AGX
uniform float uExposure;
uniform float uGamma;
uniform float uContrast;
uniform float uBrightness;

// Linear tone mapping
vec3 toneMapLinear(vec3 color) {
    return color * uExposure;
}

// Reinhard tone mapping
vec3 toneMapReinhard(vec3 color) {
    vec3 exposed = color * uExposure;
    return exposed / (1.0 + exposed);
}

// ACES tone mapping (approximation)
vec3 toneMapACES(vec3 color) {
    vec3 exposed = color * uExposure * 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((exposed * (a * exposed + b)) / (exposed * (c * exposed + d) + e), 0.0, 1.0);
}

// Uncharted 2 tone mapping
vec3 toneMapUncharted2(vec3 color) {
    vec3 exposed = color * uExposure;
    
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    
    vec3 x = exposed * (A * exposed + C * B) + D * E;
    vec3 y = exposed * (A * exposed + B) + D * F;
    vec3 whiteScale = 1.0 / (W * (A * W + C * B) + D * E);
    
    return x * whiteScale / y;
}

// AGX tone mapping
vec3 toneMapAGX(vec3 color) {
    vec3 exposed = color * uExposure;
    
    // AGX look (simplified)
    vec3 agx = pow(exposed, vec3(0.425));
    agx = mix(agx, vec3(dot(agx, vec3(0.299, 0.587, 0.114))), 0.2);
    agx = pow(agx, vec3(2.4));
    
    return agx;
}

// Apply contrast and brightness adjustments
vec3 adjustContrastBrightness(vec3 color) {
    // Apply brightness
    color += uBrightness;
    
    // Apply contrast
    color = (color - 0.5) * uContrast + 0.5;
    
    return color;
}

// Apply gamma correction
vec3 applyGamma(vec3 color) {
    return pow(color, vec3(1.0 / uGamma));
}

void main() {
    vec3 hdrColor = texture(uHDRTexture, vTexCoord).rgb;
    
    // Apply tone mapping
    vec3 ldrColor;
    switch (uToneMapOperator) {
        case 0: ldrColor = toneMapLinear(hdrColor); break;
        case 1: ldrColor = toneMapReinhard(hdrColor); break;
        case 2: ldrColor = toneMapACES(hdrColor); break;
        case 3: ldrColor = toneMapUncharted2(hdrColor); break;
        case 4: ldrColor = toneMapAGX(hdrColor); break;
        default: ldrColor = toneMapLinear(hdrColor); break;
    }
    
    // Apply contrast and brightness
    ldrColor = adjustContrastBrightness(ldrColor);
    
    // Apply gamma correction
    ldrColor = applyGamma(ldrColor);
    
    fragColor = vec4(ldrColor, 1.0);
}
