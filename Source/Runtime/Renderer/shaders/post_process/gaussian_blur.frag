#version 450 core

// Gaussian Blur Fragment Shader
// High-quality separable Gaussian blur for bloom and other effects

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uDirection; // (1.0, 0.0) for horizontal, (0.0, 1.0) for vertical
uniform float uSigma;
uniform int uKernelSize;
uniform float uStrength;

// Gaussian function
float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * 3.14159265) * sigma);
}

void main() {
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    
    int halfKernel = uKernelSize / 2;
    vec2 texelSize = 1.0 / textureSize(uTexture, 0);
    
    for (int i = -halfKernel; i <= halfKernel; i++) {
        float weight = gaussian(float(i), uSigma);
        vec2 offset = uDirection * float(i) * texelSize;
        color += texture(uTexture, vTexCoord + offset).rgb * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0) {
        color /= totalWeight;
    }
    
    fragColor = vec4(color * uStrength, 1.0);
}
