#version 450 core

// Screen Space Ambient Occlusion Fragment Shader
// Implements high-quality SSAO with temporal filtering and bent normals

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vViewRay;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uDepthTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uPreviousAOTexture;
uniform sampler2D uNoiseTexture;

uniform mat4 uProjection;
uniform mat4 uInverseProjection;
uniform vec2 uScreenSize;
uniform float uAOStrength;
uniform float uAORadius;
uniform float uAOBias;
uniform int uAOSamples;
uniform bool uTemporalFiltering;
uniform float uTemporalWeight;

// Generate sample kernel
vec3[16] generateSampleKernel() {
    vec3 kernel[16];
    for (int i = 0; i < 16; i++) {
        // Generate random samples in hemisphere
        float scale = float(i) / 16.0;
        scale = mix(0.1, 1.0, scale * scale);
        
        vec3 sample;
        sample.x = fract(sin(float(i) * 12.9898) * 43758.5453) * 2.0 - 1.0;
        sample.y = fract(sin(float(i) * 78.233) * 43758.5453) * 2.0 - 1.0;
        sample.z = fract(sin(float(i) * 37.719) * 43758.5453);
        
        sample = normalize(sample);
        sample *= scale;
        
        kernel[i] = sample;
    }
    return kernel;
}

// Rotate sample kernel
vec3 rotateSample(vec3 sample, vec3 normal, vec2 random) {
    // Create tangent space
    vec3 tangent = normalize(random.x * normal - normal.yzx * normal.z);
    vec3 bitangent = cross(normal, tangent);
    
    // Rotate sample
    return tangent * sample.x + bitangent * sample.y + normal * sample.z;
}

// Calculate AO for a single sample
float calculateAO(vec3 fragPos, vec3 normal, vec3 samplePos, float radius) {
    vec3 sampleDir = samplePos - fragPos;
    float distance = length(sampleDir);
    
    // Check if sample is within radius
    if (distance > radius) {
        return 0.0;
    }
    
    // Calculate occlusion
    sampleDir = normalize(sampleDir);
    float occlusion = max(dot(normal, sampleDir) - uAOBias, 0.0);
    
    // Apply distance falloff
    occlusion *= 1.0 - (distance / radius);
    
    return occlusion;
}

// Reconstruct view position from depth
vec3 reconstructViewPos(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = uInverseProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

// Temporal filtering for stable AO
float temporalFilterAO(float currentAO, vec2 uv, float previousAO) {
    if (!uTemporalFiltering) {
        return currentAO;
    }
    
    // Simple temporal accumulation
    return mix(previousAO, currentAO, uTemporalWeight);
}

void main() {
    // Sample depth and normal
    float depth = texture(uDepthTexture, vTexCoord).r;
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
    
    // Reconstruct view position
    vec3 fragPos = reconstructViewPos(vTexCoord, depth);
    
    // Generate sample kernel
    vec3 kernel[16] = generateSampleKernel();
    
    // Sample noise texture for rotation
    vec2 noiseScale = uScreenSize / 4.0;
    vec3 noise = texture(uNoiseTexture, vTexCoord * noiseScale).rgb;
    
    // Calculate AO
    float ao = 0.0;
    float totalWeight = 0.0;
    
    for (int i = 0; i < uAOSamples; i++) {
        // Rotate sample
        vec3 rotatedSample = rotateSample(kernel[i], normal, noise.xy);
        
        // Calculate sample position
        vec3 samplePos = fragPos + rotatedSample * uAORadius;
        
        // Project sample to screen space
        vec4 sampleClip = uProjection * vec4(samplePos, 1.0);
        vec2 sampleUV = (sampleClip.xy / sampleClip.w) * 0.5 + 0.5;
        
        // Check bounds
        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 &&
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            
            // Sample depth at sample position
            float sampleDepth = texture(uDepthTexture, sampleUV).r;
            vec3 sampleViewPos = reconstructViewPos(sampleUV, sampleDepth);
            
            // Calculate AO
            float sampleAO = calculateAO(fragPos, normal, sampleViewPos, uAORadius);
            ao += sampleAO;
            totalWeight += 1.0;
        }
    }
    
    if (totalWeight > 0.0) {
        ao /= totalWeight;
    }
    
    // Apply strength and normalize
    ao = 1.0 - (ao * uAOStrength);
    ao = clamp(ao, 0.0, 1.0);
    
    // Apply temporal filtering
    float previousAO = texture(uPreviousAOTexture, vTexCoord).r;
    ao = temporalFilterAO(ao, vTexCoord, previousAO);
    
    // Output AO as single channel
    fragColor = vec4(ao, ao, ao, 1.0);
}
