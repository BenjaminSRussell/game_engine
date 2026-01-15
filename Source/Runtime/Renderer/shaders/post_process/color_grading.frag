#version 450 core

// Color Grading Fragment Shader
// Implements LUT-based color grading and color correction

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uLUTTexture;
uniform int uColorGradeMode; // 0=None, 1=LUT, 2=ColorCorrection
uniform vec3 uColorFilter;
uniform vec3 uLift;
uniform vec3 uGammaCorrection;
uniform vec3 uGain;
uniform float uSaturation;
uniform float uHueShift;
uniform float uTemperature;
uniform float uTint;
uniform float uContrast;
uniform float uBrightness;

// Convert RGB to HSV
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Convert HSV to RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Apply color filter (tint)
vec3 applyColorFilter(vec3 color, vec3 filter) {
    return color * filter;
}

// Apply lift/gamma/gain (ASC-CDL color correction)
vec3 applyLiftGammaGain(vec3 color, vec3 lift, vec3 gamma, vec3 gain) {
    // Apply lift (shadows)
    color = color + lift;
    
    // Apply gamma (midtones)
    color = pow(color, 1.0 / gamma);
    
    // Apply gain (highlights)
    color = color * gain;
    
    return color;
}

// Apply saturation and hue shift
vec3 applySaturationHue(vec3 color, float saturation, float hueShift) {
    vec3 hsv = rgb2hsv(color);
    
    // Apply hue shift
    hsv.x += hueShift;
    hsv.x = mod(hsv.x, 1.0);
    
    // Apply saturation
    hsv.y *= saturation;
    hsv.y = clamp(hsv.y, 0.0, 1.0);
    
    return hsv2rgb(hsv);
}

// Apply temperature and tint
vec3 applyTemperatureTint(vec3 color, float temperature, float tint) {
    // Temperature: warm (red) to cool (blue)
    vec3 tempColor = mix(vec3(0.6, 0.6, 1.0), vec3(1.0, 0.6, 0.4), temperature);
    
    // Tint: green to magenta
    vec3 tintColor = mix(vec3(0.8, 1.0, 0.8), vec3(1.0, 0.8, 1.0), tint);
    
    return color * tempColor * tintColor;
}

// Apply contrast and brightness
vec3 applyContrastBrightness(vec3 color, float contrast, float brightness) {
    // Apply brightness
    color += brightness;
    
    // Apply contrast
    color = (color - 0.5) * contrast + 0.5;
    
    return color;
}

// LUT-based color grading
vec3 applyLUTColorGrade(vec3 color, sampler2D lut) {
    // Normalize color to LUT coordinates
    float blue = color.b * 63.0;
    vec2 lutCoord;
    lutCoord.x = (blue + color.r * 64.0) / 512.0;
    lutCoord.y = color.g / 512.0;
    
    // Sample from LUT
    return texture(lut, lutCoord).rgb;
}

// Advanced color correction with film emulation
vec3 applyFilmEmulation(vec3 color) {
    // Film response curve
    vec3 filmColor = pow(color, vec3(0.8));
    
    // Add subtle film grain
    float grain = fract(sin(dot(vTexCoord, vec2(12.9898, 78.233))) * 43758.5453);
    grain = (grain - 0.5) * 0.02;
    
    filmColor += grain;
    
    return filmColor;
}

void main() {
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    
    switch (uColorGradeMode) {
        case 1: // LUT-based grading
            color = applyLUTColorGrade(color, uLUTTexture);
            break;
        case 2: // Color correction
            color = applyLiftGammaGain(color, uLift, uGammaCorrection, uGain);
            color = applySaturationHue(color, uSaturation, uHueShift);
            color = applyTemperatureTint(color, uTemperature, uTint);
            color = applyContrastBrightness(color, uContrast, uBrightness);
            color = applyColorFilter(color, uColorFilter);
            break;
        default:
            break;
    }
    
    // Optional film emulation
    color = applyFilmEmulation(color);
    
    // Ensure color is in valid range
    color = clamp(color, 0.0, 1.0);
    
    fragColor = vec4(color, 1.0);
}
