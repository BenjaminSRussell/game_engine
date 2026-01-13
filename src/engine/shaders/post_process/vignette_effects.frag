#version 450 core

// Vignette and Chromatic Aberration Fragment Shader
// Implements cinematic vignette and chromatic aberration effects

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform float uVignetteStrength;
uniform float uVignetteRadius;
uniform vec3 uVignetteColor;
uniform float uChromaticAberrationStrength;
uniform float uChromaticAberrationOffset;
uniform bool uFilmGrainEnabled;
uniform float uFilmGrainStrength;
uniform float uTime;

// Calculate vignette
float calculateVignette(vec2 uv, float radius, float strength) {
    vec2 center = vec2(0.5);
    float dist = distance(uv, center);
    
    // Smooth vignette falloff
    float vignette = 1.0 - smoothstep(radius * 0.5, radius, dist);
    vignette = pow(vignette, strength);
    
    return vignette;
}

// Apply chromatic aberration
vec3 applyChromaticAberration(sampler2D tex, vec2 uv, float strength, float offset) {
    // Calculate distance from center
    vec2 center = vec2(0.5);
    vec2 dist = uv - center;
    float distLength = length(dist);
    
    // Calculate aberration amount based on distance
    float aberration = distLength * strength;
    
    // Sample RGB channels at different positions
    vec2 redUV = uv + dist * aberration;
    vec2 greenUV = uv;
    vec2 blueUV = uv - dist * aberration;
    
    // Add offset for artistic effect
    redUV += vec2(offset, 0.0);
    blueUV -= vec2(offset, 0.0);
    
    vec3 color;
    color.r = texture(tex, redUV).r;
    color.g = texture(tex, greenUV).g;
    color.b = texture(tex, blueUV).b;
    
    return color;
}

// Generate film grain
float generateFilmGrain(vec2 uv, float time, float strength) {
    // Animated noise for film grain
    float noise = fract(sin(dot(uv * time, vec2(12.9898, 78.233))) * 43758.5453);
    noise = (noise - 0.5) * strength;
    
    // Add some high-frequency noise
    float highFreq = fract(sin(dot(uv * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    noise += (highFreq - 0.5) * strength * 0.5;
    
    return noise;
}

// Apply scanlines for CRT effect
vec3 applyScanlines(vec3 color, vec2 uv, float strength) {
    float scanline = sin(uv.y * 800.0) * 0.04;
    scanline = mix(1.0, 1.0 - scanline, strength);
    return color * scanline;
}

// Apply color bleeding for vintage look
vec3 applyColorBleeding(vec3 color, vec2 uv) {
    // Simple color bleeding simulation
    vec3 bleed = vec3(0.0);
    bleed.r = texture(uColorTexture, uv + vec2(0.002, 0.0)).r * 0.1;
    bleed.b = texture(uColorTexture, uv - vec2(0.002, 0.0)).b * 0.1;
    
    return color + bleed;
}

void main() {
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    
    // Apply chromatic aberration
    if (uChromaticAberrationStrength > 0.0) {
        color = applyChromaticAberration(uColorTexture, vTexCoord, 
                                     uChromaticAberrationStrength, 
                                     uChromaticAberrationOffset);
    }
    
    // Apply film grain
    if (uFilmGrainEnabled) {
        float grain = generateFilmGrain(vTexCoord, uTime, uFilmGrainStrength);
        color += grain;
    }
    
    // Apply vignette
    if (uVignetteStrength > 0.0) {
        float vignette = calculateVignette(vTexCoord, uVignetteRadius, uVignetteStrength);
        
        // Mix with vignette color
        color = mix(color * vignette, uVignetteColor, 1.0 - vignette);
    }
    
    // Optional effects for artistic looks
    // color = applyScanlines(color, vTexCoord, 0.1);
    // color = applyColorBleeding(color, vTexCoord);
    
    // Ensure color is in valid range
    color = clamp(color, 0.0, 1.0);
    
    fragColor = vec4(color, 1.0);
}
