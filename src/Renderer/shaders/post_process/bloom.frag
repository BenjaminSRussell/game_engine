#version 450 core

// Bloom Fragment Shader
// Implements high-quality bloom with multiple passes and lens effects

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uHDRTexture;
uniform sampler2D uBloomTexture;
uniform float uBloomThreshold;
uniform float uBloomIntensity;
uniform float uBloomKnee;
uniform int uBloomIterations;
uniform bool uLensDirtEnabled;
uniform sampler2D uLensDirtTexture;
uniform float uLensDirtIntensity;

// Knee function for smooth bloom threshold
float knee(float x, float knee) {
    return clamp(x - knee, 0.0, 1.0);
}

// Extract bright areas for bloom
vec3 extractBloom(vec3 color) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float kneeFactor = knee(brightness, uBloomKnee);
    return color * kneeFactor * smoothstep(uBloomThreshold - 0.01, uBloomThreshold + 0.01, brightness);
}

// Apply bloom with lens dirt
vec3 applyBloom(vec3 color, vec3 bloom) {
    vec3 finalBloom = bloom * uBloomIntensity;
    
    if (uLensDirtEnabled) {
        vec3 lensDirt = texture(uLensDirtTexture, vTexCoord).rgb;
        finalBloom += lensDirt * finalBloom * uLensDirtIntensity;
    }
    
    return color + finalBloom;
}

void main() {
    vec3 hdrColor = texture(uHDRTexture, vTexCoord).rgb;
    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
    
    // Extract bloom from original HDR texture if this is the first pass
    if (uBloomIterations == 0) {
        bloomColor = extractBloom(hdrColor);
    }
    
    // Apply bloom
    vec3 finalColor = applyBloom(hdrColor, bloomColor);
    
    fragColor = vec4(finalColor, 1.0);
}
