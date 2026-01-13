#version 450 core

// Uber Post-Processing Fragment Shader
// Combines all post-processing effects in a single shader for optimal performance

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vViewRay;

layout(location = 0) out vec4 fragColor;

// Input textures
uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uVelocityTexture;
uniform sampler2D uBloomTexture;
uniform sampler2D uAOTexture;
uniform sampler2D uLUTTexture;
uniform sampler2D uLensDirtTexture;

// Matrices
uniform mat4 uInverseProjection;
uniform mat4 uInverseView;
uniform mat4 uProjection;
uniform mat4 uPreviousViewProjection;

// Post-processing settings
uniform int uEnabledEffects; // Bitmask of enabled effects
uniform int uToneMapOperator;
uniform float uExposure;
uniform float uGamma;
uniform float uContrast;
uniform float uBrightness;
uniform float uSaturation;
uniform float uHueShift;
uniform vec3 uColorFilter;
uniform vec3 uLift;
uniform vec3 uGammaCorrection;
uniform vec3 uGain;
uniform float uBloomThreshold;
uniform float uBloomIntensity;
uniform float uBloomKnee;
uniform float uFocusDistance;
uniform float uFocusRange;
uniform float uMaxBlur;
uniform float uMotionScale;
uniform int uMotionSamples;
uniform float uAOStrength;
uniform float uAORadius;
uniform float uAOBias;
uniform int uAOSamples;
uniform float uVignetteStrength;
uniform float uVignetteRadius;
uniform vec3 uVignetteColor;
uniform float uChromaticAberrationStrength;
uniform float uChromaticAberrationOffset;
uniform bool uFilmGrainEnabled;
uniform float uFilmGrainStrength;
uniform float uTime;
uniform int uAAMethod;
uniform float uTAABlendFactor;
uniform float uTAASharpening;

// Effect flags
#define EFFECT_TONE_MAPPING    (1 << 0)
#define EFFECT_COLOR_GRADING    (1 << 1)
#define EFFECT_BLOOM           (1 << 2)
#define EFFECT_DEPTH_OF_FIELD   (1 << 3)
#define EFFECT_MOTION_BLUR     (1 << 4)
#define EFFECT_SSR             (1 << 5)
#define EFFECT_SSAO            (1 << 6)
#define EFFECT_FXAA            (1 << 7)
#define EFFECT_TAA             (1 << 8)
#define EFFECT_VIGNETTE        (1 << 9)
#define EFFECT_CHROMATIC       (1 << 10)
#define EFFECT_FILM_GRAIN      (1 << 11)

// Include helper functions from other shaders
// (In a real implementation, these would be in separate include files)

vec3 toneMapACES(vec3 color) {
    vec3 exposed = color * uExposure * 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((exposed * (a * exposed + b)) / (exposed * (c * exposed + d) + e), 0.0, 1.0);
}

vec3 applyLiftGammaGain(vec3 color) {
    color = color + uLift;
    color = pow(color, 1.0 / uGammaCorrection);
    color = color * uGain;
    return color;
}

vec3 applySaturationHue(vec3 color) {
    // Simple HSV conversion for saturation and hue
    float maxVal = max(color.r, max(color.g, color.b));
    float minVal = min(color.r, min(color.g, color.b));
    float delta = maxVal - minVal;
    
    if (delta > 0.0) {
        vec3 hsv;
        hsv.z = maxVal;
        hsv.y = delta / maxVal;
        
        if (maxVal == color.r) {
            hsv.x = (color.g - color.b) / delta;
        } else if (maxVal == color.g) {
            hsv.x = 2.0 + (color.b - color.r) / delta;
        } else {
            hsv.x = 4.0 + (color.r - color.g) / delta;
        }
        
        hsv.x += uHueShift;
        hsv.x = mod(hsv.x, 6.0);
        hsv.y *= uSaturation;
        hsv.y = clamp(hsv.y, 0.0, 1.0);
        
        // Convert back to RGB
        float c = hsv.z * hsv.y;
        float x = c * (1.0 - abs(mod(hsv.x, 2.0) - 1.0));
        float m = hsv.z - c;
        
        if (hsv.x < 1.0) color = vec3(c, x, 0.0);
        else if (hsv.x < 2.0) color = vec3(x, c, 0.0);
        else if (hsv.x < 3.0) color = vec3(0.0, c, x);
        else if (hsv.x < 4.0) color = vec3(0.0, x, c);
        else if (hsv.x < 5.0) color = vec3(x, 0.0, c);
        else color = vec3(c, 0.0, x);
        
        color += vec3(m, m, m);
    }
    
    return color;
}

vec3 applyBloom(vec3 color) {
    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
    return color + bloomColor * uBloomIntensity;
}

vec3 applyVignette(vec3 color) {
    vec2 center = vec2(0.5);
    float dist = distance(vTexCoord, center);
    float vignette = 1.0 - smoothstep(uVignetteRadius * 0.5, uVignetteRadius, dist);
    vignette = pow(vignette, uVignetteStrength);
    return mix(color * vignette, uVignetteColor, 1.0 - vignette);
}

vec3 applyChromaticAberration(vec3 color) {
    vec2 center = vec2(0.5);
    vec2 dist = vTexCoord - center;
    float distLength = length(dist);
    float aberration = distLength * uChromaticAberrationStrength;
    
    vec2 redUV = vTexCoord + dist * aberration;
    vec2 blueUV = vTexCoord - dist * aberration;
    
    color.r = texture(uColorTexture, redUV).r;
    color.b = texture(uColorTexture, blueUV).b;
    
    return color;
}

float generateFilmGrain() {
    float noise = fract(sin(dot(vTexCoord * uTime, vec2(12.9898, 78.233))) * 43758.5453);
    return (noise - 0.5) * uFilmGrainStrength;
}

void main() {
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    
    // Apply effects in order
    
    // 1. Tone Mapping
    if ((uEnabledEffects & EFFECT_TONE_MAPPING) != 0) {
        if (uToneMapOperator == 2) { // ACES
            color = toneMapACES(color);
        }
        // Add other tone mapping operators as needed
    }
    
    // 2. Color Grading
    if ((uEnabledEffects & EFFECT_COLOR_GRADING) != 0) {
        color = applyLiftGammaGain(color);
        color = applySaturationHue(color);
        color = applyContrastBrightness(color, uContrast, uBrightness);
        color *= uColorFilter;
    }
    
    // 3. Bloom
    if ((uEnabledEffects & EFFECT_BLOOM) != 0) {
        color = applyBloom(color);
    }
    
    // 4. Depth of Field (simplified)
    if ((uEnabledEffects & EFFECT_DEPTH_OF_FIELD) != 0) {
        float depth = texture(uDepthTexture, vTexCoord).r;
        float coc = abs(depth - uFocusDistance) / uFocusRange;
        coc = clamp(coc * uMaxBlur, 0.0, 1.0);
        
        if (coc > 0.1) {
            // Simple box blur for DoF
            vec3 blurred = vec3(0.0);
            vec2 texelSize = 1.0 / textureSize(uColorTexture, 0);
            int samples = 4;
            
            for (int x = -samples; x <= samples; x++) {
                for (int y = -samples; y <= samples; y++) {
                    vec2 offset = vec2(x, y) * texelSize * coc;
                    blurred += texture(uColorTexture, vTexCoord + offset).rgb;
                }
            }
            
            blurred /= float((samples * 2 + 1) * (samples * 2 + 1));
            color = mix(color, blurred, coc);
        }
    }
    
    // 5. Motion Blur (simplified)
    if ((uEnabledEffects & EFFECT_MOTION_BLUR) != 0) {
        vec2 velocity = texture(uVelocityTexture, vTexCoord).rg * uMotionScale;
        if (length(velocity) > 0.001) {
            vec3 blurred = vec3(0.0);
            for (int i = 0; i <= uMotionSamples; i++) {
                float t = float(i) / float(uMotionSamples);
                blurred += texture(uColorTexture, vTexCoord + velocity * t).rgb;
            }
            color = blurred / float(uMotionSamples + 1);
        }
    }
    
    // 6. SSAO
    if ((uEnabledEffects & EFFECT_SSAO) != 0) {
        float ao = texture(uAOTexture, vTexCoord).r;
        color *= ao;
    }
    
    // 7. Anti-Aliasing (simplified)
    if ((uEnabledEffects & EFFECT_FXAA) != 0) {
        // Simple edge detection and blur
        vec2 texelSize = 1.0 / textureSize(uColorTexture, 0);
        vec3 hL = texture(uColorTexture, vTexCoord + vec2(-texelSize.x, 0.0)).rgb;
        vec3 hR = texture(uColorTexture, vTexCoord + vec2(texelSize.x, 0.0)).rgb;
        vec3 vT = texture(uColorTexture, vTexCoord + vec2(0.0, -texelSize.y)).rgb;
        vec3 vB = texture(uColorTexture, vTexCoord + vec2(0.0, texelSize.y)).rgb;
        
        float hDelta = abs(dot(hL, vec3(0.299, 0.587, 0.114)) - dot(hR, vec3(0.299, 0.587, 0.114)));
        float vDelta = abs(dot(vT, vec3(0.299, 0.587, 0.114)) - dot(vB, vec3(0.299, 0.587, 0.114)));
        
        if (max(hDelta, vDelta) > 0.1) {
            color = (color + hL + hR + vT + vB) * 0.2;
        }
    }
    
    // 8. Chromatic Aberration
    if ((uEnabledEffects & EFFECT_CHROMATIC) != 0) {
        color = applyChromaticAberration(color);
    }
    
    // 9. Film Grain
    if ((uEnabledEffects & EFFECT_FILM_GRAIN) != 0 && uFilmGrainEnabled) {
        color += generateFilmGrain();
    }
    
    // 10. Vignette
    if ((uEnabledEffects & EFFECT_VIGNETTE) != 0) {
        color = applyVignette(color);
    }
    
    // Ensure color is in valid range
    color = clamp(color, 0.0, 1.0);
    
    fragColor = vec4(color, 1.0);
}
