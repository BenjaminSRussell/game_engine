#version 450

// Temporal Anti-Aliasing (TAA) Shader
// Reduces aliasing through temporal reprojection

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D currentFrame;
layout(binding = 1) uniform sampler2D historyFrame;
layout(binding = 2) uniform sampler2D velocityBuffer;  // Motion vectors
layout(binding = 3) uniform sampler2D depthBuffer;

layout(push_constant) uniform TAAParams {
    vec2 jitterOffset;      // Current frame jitter
    float blendFactor;      // History blend weight (0.9 = 90% history)
    float varianceClamp;    // Neighborhood clamping radius
} params;

// Sample 3x3 neighborhood for variance clamping
vec3 sampleNeighborhood(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 minColor = vec3(1e10);
    vec3 maxColor = vec3(-1e10);
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    
    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 c = texture(tex, uv + offset).rgb;
            minColor = min(minColor, c);
            maxColor = max(maxColor, c);
            m1 += c;
            m2 += c * c;
        }
    }
    
    // Variance-based clamping (tighter than min/max)
    m1 /= 9.0;
    m2 /= 9.0;
    vec3 sigma = sqrt(max(m2 - m1 * m1, 0.0));
    return m1; // Could also return (m1, sigma) for variance clamp
}

void main() {
    vec2 texelSize = 1.0 / textureSize(currentFrame, 0);
    
    // Sample current frame
    vec3 currentColor = texture(currentFrame, fragUV).rgb;
    
    // Sample motion vector and reproject
    vec2 velocity = texture(velocityBuffer, fragUV).rg;
    vec2 historyUV = fragUV - velocity;
    
    // Reject history if out of bounds
    if(historyUV.x < 0.0 || historyUV.x > 1.0 || 
       historyUV.y < 0.0 || historyUV.y > 1.0) {
        outColor = vec4(currentColor, 1.0);
        return;
    }
    
    // Sample history with bilinear filtering
    vec3 historyColor = texture(historyFrame, historyUV).rgb;
    
    // Neighborhood clamping to reduce ghosting
    vec3 neighborAvg = sampleNeighborhood(currentFrame, fragUV, texelSize);
    
    // Clamp history to neighborhood (prevents ghosting)
    vec3 minColor = neighborAvg - params.varianceClamp;
    vec3 maxColor = neighborAvg + params.varianceClamp;
    historyColor = clamp(historyColor, minColor, maxColor);
    
    // Blend current and history
    vec3 result = mix(currentColor, historyColor, params.blendFactor);
    
    outColor = vec4(result, 1.0);
}
