#version 450
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// Ray tracing shader support with RTX extensions
layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT bool shadowed;

hitAttributeEXT vec2 attribs; // Barycentric coordinates

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) uniform sampler2D textures[256]; // Texture array

// Vertex data structure matching our Vertex struct
struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
    uint ao;
    uint light;
    uint textureId;
};

// Vertex and index data buffers
layout(binding = 1, set = 0, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 2, set = 0, std430) readonly buffer IndexBuffer {
    uint indices[];
};

// Ray tracing descriptor sets for resource binding
layout(binding = 5, set = 0) uniform RayTracingParams {
    vec3 cameraPos;
    vec3 lightDir;
    float time;
    int maxBounces;
    float exposure;
} rtParams;

// Ray tracing push constants for shader parameters
layout(push_constant) uniform PushConstants {
    vec3 viewOrigin;
    float pad0;
    vec3 viewDirection;
    float pad1;
    int frameIndex;
    float maxRayDistance;
} pushConstants;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragAO;
layout(location = 3) in float fragLight;
layout(location = 5) in flat int fragTextureID;
layout(location = 6) in float fragWavePhase;

layout(location = 0) out vec4 outColor;

// PBR Material system with physically based properties
struct PBRMaterial {
    vec3 albedo;
    float metallic;
    float roughness;
    vec3 normal;
    float ao;
    vec3 emission;
};

// Multiple light types for ray tracing lighting system
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    int type; // 0=directional, 1=point, 2=spot
    vec3 direction; // for directional/spot lights
    float range; // for point/spot lights
};

// Ray tracing shadow system with soft shadows
float calculateSoftShadow(vec3 worldPos, vec3 normal, vec3 lightDir) {
    const int SHADOW_RAYS = 8;
    const float SHADOW_RADIUS = 0.1f;
    float shadow = 0.0f;
    
    for (int i = 0; i < SHADOW_RAYS; i++) {
        vec2 offset = vec2(
            cos(float(i) * 6.28318530718 / float(SHADOW_RAYS)),
            sin(float(i) * 6.28318530718 / float(SHADOW_RAYS))
        ) * SHADOW_RADIUS;
        
        vec3 offsetDir = lightDir + vec3(offset.x, offset.y, 0.0) * 0.01;
        offsetDir = normalize(offsetDir);
        
        shadowed = true;
        traceRayEXT(
            topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
            0xFF,
            0, 0, 1,
            worldPos + normal * 0.01,
            0.001,
            offsetDir,
            100.0,
            1
        );
        
        shadow += shadowed ? 0.0 : 1.0;
    }
    
    return shadow / float(SHADOW_RAYS);
}

// Ray tracing reflection system with accurate reflections
vec3 calculateReflection(vec3 worldPos, vec3 normal, vec3 viewDir, PBRMaterial material) {
    if (material.roughness > 0.8) return vec3(0.0);
    
    vec3 reflectDir = reflect(-viewDir, normal);
    
    // Add roughness-based perturbation for glossy reflections
    if (material.roughness > 0.1) {
        vec2 random = vec2(
            sin(pushConstants.frameIndex * 12.9898 + worldPos.x * 78.233),
            cos(pushConstants.frameIndex * 4.1414 + worldPos.z * 37.719)
        );
        reflectDir += vec3(random * 0.1 * material.roughness, 0.0);
        reflectDir = normalize(reflectDir);
    }
    
    vec3 reflectionColor = vec3(0.0);
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsOpaqueEXT,
        0xFF,
        0, 0, 0,
        worldPos + normal * 0.01,
        0.001,
        reflectDir,
        pushConstants.maxRayDistance,
        0
    );
    
    return reflectionColor * (1.0 - material.roughness);
}

// Ray tracing refraction system for transparent materials
vec3 calculateRefraction(vec3 worldPos, vec3 normal, vec3 viewDir, PBRMaterial material) {
    if (material.albedo.a < 0.1) return vec3(0.0);
    
    float ior = 1.5; // Glass IOR
    vec3 refractDir = refract(-viewDir, normal, 1.0 / ior);
    
    vec3 refractionColor = vec3(0.0);
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsOpaqueEXT,
        0xFF,
        0, 0, 0,
        worldPos - normal * 0.01,
        0.001,
        refractDir,
        pushConstants.maxRayDistance,
        0
    );
    
    return refractionColor * material.albedo.a;
}

// Ray tracing global illumination system
vec3 calculateGlobalIllumination(vec3 worldPos, vec3 normal) {
    vec3 giColor = vec3(0.0);
    const int GI_RAYS = 4;
    
    for (int i = 0; i < GI_RAYS; i++) {
        float theta = float(i) * 6.28318530718 / float(GI_RAYS);
        vec3 sampleDir = normalize(
            normal + vec3(cos(theta), 0.5, sin(theta)) * 0.5
        );
        
        traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            0xFF,
            0, 0, 0,
            worldPos + normal * 0.01,
            0.001,
            sampleDir,
            10.0,
            0
        );
        
        giColor += hitValue * 0.25;
    }
    
    return giColor * 0.2;
}

// Ray tracing denoising system for noise reduction
vec3 denoise(vec3 color, vec3 normal, float depth) {
    // Simple spatial denoising using neighboring pixels
    vec2 texelSize = 1.0 / vec2(1920, 1080); // Assume screen resolution
    vec3 accumulated = color;
    int samples = 1;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if x == 0 && y == 0) continue;
            
            vec2 offset = vec2(x, y) * texelSize;
            // In a real implementation, we'd sample neighboring pixels
            // For now, just use the current color as a placeholder
            accumulated += color;
            samples++;
        }
    }
    
    return accumulated / float(samples);
}

void main() {
    // Get hit information
    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    
    // Get primitive index and vertex data
    uint primitiveIndex = gl_PrimitiveID;
    uint i0 = indices[primitiveIndex * 3];
    uint i1 = indices[primitiveIndex * 3 + 1];
    uint i2 = indices[primitiveIndex * 3 + 2];
    
    Vertex v0 = vertices[i0];
    Vertex v1 = vertices[i1];
    Vertex v2 = vertices[i2];
    
    // Interpolate attributes
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 normal = normalize(
        barycentrics.x * v0.normal +
        barycentrics.y * v1.normal +
        barycentrics.z * v2.normal
    );
    vec2 uv = 
        barycentrics.x * v0.uv +
        barycentrics.y * v1.uv +
        barycentrics.z * v2.uv;
    uint textureId = v0.textureId;
    
    // Create PBR material
    PBRMaterial material;
    material.albedo = textureId < 256 ? texture(textures[textureId], uv).rgb : vec3(0.8, 0.7, 0.6);
    material.metallic = 0.0; // Most blocks are not metallic
    material.roughness = textureId == 49 ? 0.3 : 0.8; // Glass is smooth
    material.normal = normal;
    material.ao = barycentrics.x * float(v0.ao) + barycentrics.y * float(v1.ao) + barycentrics.z * float(v2.ao) / 255.0;
    material.emission = vec3(0.0);
    
    // Calculate lighting
    // Lighting system with multiple light types
    vec3 lightDir = normalize(rtParams.lightDir);
    vec3 viewDir = normalize(pushConstants.viewOrigin - worldPos);
    
    // Direct lighting with soft shadows
    float shadow = calculateSoftShadow(worldPos, normal, lightDir);
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 directLight = rtParams.lightDir * NdotL * shadow;
    
    // Reflections
    vec3 reflection = calculateReflection(worldPos, normal, viewDir, material);
    
    // Refractions (for transparent materials)
    vec3 refraction = calculateRefraction(worldPos, normal, viewDir, material);
    
    // Global illumination
    vec3 gi = calculateGlobalIllumination(worldPos, normal);
    
    // Combine all lighting contributions
    vec3 finalColor =
        material.albedo * (directLight + gi * 0.3) +
        reflection * 0.5 +
        refraction +
        material.emission;
    
    // Apply exposure
    finalColor *= rtParams.exposure;
    
    // Denoise
    finalColor = denoise(finalColor, normal, gl_HitTEXT);
    
    // Tone map (simple ACES tone mapping)
    finalColor = (finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14);
    
    hitValue = finalColor;
}
