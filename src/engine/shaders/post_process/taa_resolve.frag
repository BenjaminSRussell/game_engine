#version 450 core

// Temporal Anti-Aliasing History Resolve Fragment Shader
// Resolves TAA history with clamping and variance filtering

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uCurrentColorTexture;
uniform sampler2D uPreviousColorTexture;
uniform sampler2D uVelocityTexture;
uniform sampler2D uDepthTexture;
uniform mat4 uInverseViewProjection;
uniform mat4 uPreviousViewProjection;
uniform float uAlpha;
uniform float uColorBoxSigma;
uniform bool uUseVarianceFiltering;

// Reproject previous frame pixel
vec2 reprojectPixel(vec2 currentUV, vec3 worldPos) {
    vec4 previousClip = uPreviousViewProjection * vec4(worldPos, 1.0);
    previousClip /= previousClip.w;
    return previousClip.xy * 0.5 + 0.5;
}

// Reconstruct world position from depth
vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = uInverseViewProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

// Neighborhood clamping to prevent ghosting
vec3 neighborhoodClamping(vec3 current, vec3 history, vec2 uv, vec2 texelSize) {
    // Sample 3x3 neighborhood
    vec3 minColor = current;
    vec3 maxColor = current;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            
            vec2 sampleUV = uv + vec2(x, y) * texelSize;
            vec3 sampleColor = texture(uCurrentColorTexture, sampleUV).rgb;
            
            minColor = min(minColor, sampleColor);
            maxColor = max(maxColor, sampleColor);
        }
    }
    
    return clamp(history, minColor, maxColor);
}

// Variance-based temporal filtering
vec3 varianceFiltering(vec3 current, vec3 history, vec2 uv, vec2 texelSize) {
    // Calculate local variance
    vec3 mean = vec3(0.0);
    vec3 variance = vec3(0.0);
    int samples = 0;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 sampleUV = uv + vec2(x, y) * texelSize;
            vec3 sampleColor = texture(uCurrentColorTexture, sampleUV).rgb;
            
            mean += sampleColor;
            variance += sampleColor * sampleColor;
            samples++;
        }
    }
    
    mean /= float(samples);
    variance /= float(samples);
    variance -= mean * mean;
    
    // Calculate confidence based on variance
    vec3 stdDev = sqrt(max(variance, vec3(0.0)));
    vec3 confidence = 1.0 / (stdDev + 0.001);
    confidence = clamp(confidence, 0.0, 1.0);
    
    // Blend based on confidence
    return mix(history, current, confidence);
}

// Color box filtering for better temporal stability
vec3 colorBoxFilter(vec3 current, vec3 history, vec2 uv, vec2 texelSize) {
    // Sample 3x3 neighborhood for current frame
    vec3 currentMean = vec3(0.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 sampleUV = uv + vec2(x, y) * texelSize;
            currentMean += texture(uCurrentColorTexture, sampleUV).rgb;
        }
    }
    currentMean /= 9.0;
    
    // Calculate color box
    vec3 minColor = currentMean - vec3(uColorBoxSigma);
    vec3 maxColor = currentMean + vec3(uColorBoxSigma);
    
    // Clamp history to color box
    return clamp(history, minColor, maxColor);
}

// Detect disocclusion
bool isDisoccluded(vec2 currentUV, vec2 previousUV, float currentDepth, float previousDepth) {
    // Check if depth difference is too large
    float depthDiff = abs(currentDepth - previousDepth);
    if (depthDiff > 0.1) return true;
    
    // Check if reprojection is out of bounds
    if (previousUV.x < 0.0 || previousUV.x > 1.0 ||
        previousUV.y < 0.0 || previousUV.y > 1.0) {
        return true;
    }
    
    return false;
}

void main() {
    vec2 texelSize = 1.0 / textureSize(uCurrentColorTexture, 0);
    
    // Get current frame data
    vec3 currentColor = texture(uCurrentColorTexture, vTexCoord).rgb;
    float currentDepth = texture(uDepthTexture, vTexCoord).r;
    vec2 velocity = texture(uVelocityTexture, vTexCoord).rg;
    
    // Reconstruct world position
    vec3 worldPos = reconstructWorldPosition(vTexCoord, currentDepth);
    
    // Reproject to previous frame
    vec2 previousUV = reprojectPixel(vTexCoord, worldPos);
    
    vec3 resolvedColor = currentColor;
    
    // Check if reprojection is valid
    if (previousUV.x >= 0.0 && previousUV.x <= 1.0 &&
        previousUV.y >= 0.0 && previousUV.y <= 1.0) {
        
        // Sample previous frame
        vec3 previousColor = texture(uPreviousColorTexture, previousUV).rgb;
        float previousDepth = texture(uDepthTexture, previousUV).r;
        
        // Check for disocclusion
        if (!isDisoccluded(vTexCoord, previousUV, currentDepth, previousDepth)) {
            // Apply temporal filtering
            if (uUseVarianceFiltering) {
                resolvedColor = varianceFiltering(currentColor, previousColor, vTexCoord, texelSize);
            } else {
                resolvedColor = neighborhoodClamping(currentColor, previousColor, vTexCoord, texelSize);
                resolvedColor = colorBoxFilter(currentColor, resolvedColor, vTexCoord, texelSize);
            }
            
            // Blend with current frame
            resolvedColor = mix(resolvedColor, currentColor, uAlpha);
        }
    }
    
    fragColor = vec4(resolvedColor, 1.0);
}
