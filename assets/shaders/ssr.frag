#version 450

// Screen-Space Reflections (SSR) Shader
// Ray marches in screen space for reflections

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorBuffer;
layout(binding = 1) uniform sampler2D normalBuffer;
layout(binding = 2) uniform sampler2D depthBuffer;
layout(binding = 3) uniform sampler2D roughnessBuffer;

layout(push_constant) uniform SSRParams {
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 invProjMatrix;
    float maxDistance;      // Max ray march distance
    float resolution;       // Step size
    float thickness;        // Ray thickness for hit detection
    float fadeStart;        // Edge fade start distance
} params;

// Reconstruct world position from depth
vec3 worldPosFromDepth(vec2 uv, float depth) {
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpace = params.invProjMatrix * clipSpace;
    return viewSpace.xyz / viewSpace.w;
}

// Ray march in screen space
bool rayMarch(vec3 rayOrigin, vec3 rayDir, out vec2 hitUV, out float hitDepth) {
    vec3 rayStep = rayDir * params.resolution;
    vec3 currentPos = rayOrigin;
    
    const int MAX_STEPS = 64;
    for(int i = 0; i < MAX_STEPS; i++) {
        currentPos += rayStep;
        
        // Project to screen space
        vec4 projPos = params.projMatrix * vec4(currentPos, 1.0);
        projPos.xyz /= projPos.w;
        vec2 screenUV = projPos.xy * 0.5 + 0.5;
        
        // Out of bounds check
        if(screenUV.x < 0.0 || screenUV.x > 1.0 || 
           screenUV.y < 0.0 || screenUV.y > 1.0) {
            return false;
        }
        
        // Sample depth buffer
        float sceneDepth = texture(depthBuffer, screenUV).r;
        vec3 scenePos = worldPosFromDepth(screenUV, sceneDepth);
        
        // Check intersection
        float delta = currentPos.z - scenePos.z;
        if(delta > 0.0 && delta < params.thickness) {
            hitUV = screenUV;
            hitDepth = sceneDepth;
            return true;
        }
        
        // Early exit if too far
        if(length(currentPos - rayOrigin) > params.maxDistance) {
            return false;
        }
    }
    
    return false;
}

void main() {
    // Sample G-buffer
    float depth = texture(depthBuffer, fragUV).r;
    vec3 normal = normalize(texture(normalBuffer, fragUV).rgb * 2.0 - 1.0);
    float roughness = texture(roughnessBuffer, fragUV).r;
    
    // Early exit for rough surfaces (use IBL instead)
    if(roughness > 0.5) {
        outColor = vec4(0.0);
        return;
    }
    
    // Reconstruct position
    vec3 viewPos = worldPosFromDepth(fragUV, depth);
    vec3 viewDir = normalize(viewPos);
    vec3 reflectDir = reflect(viewDir, normal);
    
    // Ray march
    vec2 hitUV;
    float hitDepth;
    if(rayMarch(viewPos, reflectDir, hitUV, hitDepth)) {
        // Sample reflected color
        vec3 reflectedColor = texture(colorBuffer, hitUV).rgb;
        
        // Edge fade
        vec2 edgeDist = abs(hitUV - 0.5) * 2.0;
        float edgeFade = 1.0 - smoothstep(params.fadeStart, 1.0, max(edgeDist.x, edgeDist.y));
        
        // Roughness fade
        float roughnessFade = 1.0 - smoothstep(0.2, 0.5, roughness);
        
        float finalAlpha = edgeFade * roughnessFade;
        outColor = vec4(reflectedColor, finalAlpha);
    } else {
        outColor = vec4(0.0);
    }
}
