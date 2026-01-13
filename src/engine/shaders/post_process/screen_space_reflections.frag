#version 450 core

// Screen Space Reflections Fragment Shader
// Implements high-quality SSR with ray marching and temporal filtering

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vViewRay;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uPreviousReflectionTexture;

uniform mat4 uInverseProjection;
uniform mat4 uInverseView;
uniform mat4 uPreviousViewProjection;
uniform vec3 uCameraPosition;
uniform vec2 uScreenSize;
uniform float uMaxRayDistance;
uniform int uMaxSteps;
uniform float uRayStepSize;
uniform float uEdgeFadeDistance;
uniform bool uTemporalFiltering;
uniform float uTemporalWeight;

// Convert from screen space to world space
vec3 screenToWorld(vec3 screenPos) {
    vec4 viewPos = uInverseProjection * vec4(screenPos, 1.0);
    viewPos /= viewPos.w;
    vec4 worldPos = uInverseView * viewPos;
    return worldPos.xyz;
}

// Convert from world space to screen space
vec3 worldToScreen(vec3 worldPos) {
    vec4 viewPos = uInverseView * vec4(worldPos, 1.0);
    vec4 clipPos = uProjection * viewPos;
    clipPos /= clipPos.w;
    return clipPos.xyz * 0.5 + 0.5;
}

// Ray marching for SSR
vec3 rayMarchSSR(vec3 rayOrigin, vec3 rayDir, float maxDistance, int maxSteps) {
    float stepSize = uRayStepSize;
    vec3 currentPos = rayOrigin;
    
    for (int i = 0; i < maxSteps; i++) {
        currentPos += rayDir * stepSize;
        
        // Convert to screen space
        vec3 screenPos = worldToScreen(currentPos);
        
        // Check bounds
        if (screenPos.x < 0.0 || screenPos.x > 1.0 ||
            screenPos.y < 0.0 || screenPos.y > 1.0 ||
            screenPos.z < 0.0 || screenPos.z > 1.0) {
            break;
        }
        
        // Sample depth
        float sceneDepth = texture(uDepthTexture, screenPos.xy).r;
        float rayDepth = screenPos.z;
        
        // Check intersection
        if (rayDepth > sceneDepth) {
            // Refine intersection
            float refinementSteps = 4.0;
            for (float j = 0.0; j < refinementSteps; j++) {
                float t = j / refinementSteps;
                vec3 refinedPos = mix(currentPos - rayDir * stepSize, currentPos, t);
                vec3 refinedScreen = worldToScreen(refinedPos);
                float refinedDepth = texture(uDepthTexture, refinedScreen.xy).r;
                
                if (refinedScreen.z > refinedDepth) {
                    return refinedScreen;
                }
            }
            return screenPos;
        }
        
        // Adaptive step size
        float depthDiff = abs(rayDepth - sceneDepth);
        stepSize = mix(uRayStepSize * 0.1, uRayStepSize * 2.0, depthDiff);
    }
    
    return vec3(-1.0); // No intersection
}

// Calculate edge fade for smooth transitions
float calculateEdgeFade(vec2 uv, float distance) {
    vec2 edgeDistance = min(uv, 1.0 - uv);
    float minEdgeDistance = min(edgeDistance.x, edgeDistance.y);
    return smoothstep(0.0, uEdgeFadeDistance, minEdgeDistance);
}

// Temporal filtering for stable reflections
vec3 temporalFilter(vec3 currentColor, vec2 uv, vec3 previousColor) {
    if (!uTemporalFiltering) {
        return currentColor;
    }
    
    // Reproject previous frame
    vec3 worldPos = screenToWorld(vec3(uv, texture(uDepthTexture, uv).r));
    vec4 previousClip = uPreviousViewProjection * vec4(worldPos, 1.0);
    previousClip /= previousClip.w;
    vec2 previousUV = previousClip.xy * 0.5 + 0.5;
    
    // Check if previous UV is valid
    if (previousUV.x >= 0.0 && previousUV.x <= 1.0 &&
        previousUV.y >= 0.0 && previousUV.y <= 1.0) {
        vec3 prevColor = texture(uPreviousReflectionTexture, previousUV).rgb;
        return mix(prevColor, currentColor, uTemporalWeight);
    }
    
    return currentColor;
}

void main() {
    // Sample current pixel data
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    float depth = texture(uDepthTexture, vTexCoord).r;
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
    vec2 metallicRoughness = texture(uMetallicRoughnessTexture, vTexCoord).rg;
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;
    
    // Skip SSR for non-metallic or very rough surfaces
    if (metallic < 0.1 || roughness > 0.8) {
        fragColor = vec4(color, 1.0);
        return;
    }
    
    // Reconstruct world position
    vec3 worldPos = screenToWorld(vec3(vTexCoord, depth));
    
    // Calculate reflection vector
    vec3 viewDir = normalize(worldPos - uCameraPosition);
    vec3 reflectDir = reflect(viewDir, normal);
    
    // Ray march for reflection
    vec3 hitPoint = rayMarchSSR(worldPos, reflectDir, uMaxRayDistance, uMaxSteps);
    
    vec3 reflectionColor = vec3(0.0);
    if (hitPoint.x >= 0.0) {
        // Sample reflection color
        reflectionColor = texture(uColorTexture, hitPoint.xy).rgb;
        
        // Apply edge fade
        float edgeFade = calculateEdgeFade(hitPoint.xy, length(hitPoint.xy - vTexCoord));
        reflectionColor *= edgeFade;
        
        // Apply fresnel
        float fresnel = pow(1.0 - dot(-viewDir, normal), 5.0);
        reflectionColor *= fresnel;
        
        // Apply roughness fade
        reflectionColor *= (1.0 - roughness);
        
        // Temporal filtering
        reflectionColor = temporalFilter(reflectionColor, vTexCoord, 
                                   texture(uPreviousReflectionTexture, vTexCoord).rgb);
    }
    
    // Blend reflection with original color
    vec3 finalColor = mix(color, reflectionColor, metallic);
    
    fragColor = vec4(finalColor, 1.0);
}
