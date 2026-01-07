#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// Closest hit shader - handles surface intersection and shading
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

void main() {
    // Get hit information
    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    
    // Get primitive index from gl_PrimitiveID
    uint primitiveIndex = gl_PrimitiveID;
    
    // Get vertex indices for this triangle
    uint i0 = indices[primitiveIndex * 3];
    uint i1 = indices[primitiveIndex * 3 + 1];
    uint i2 = indices[primitiveIndex * 3 + 2];
    
    // Get vertices
    Vertex v0 = vertices[i0];
    Vertex v1 = vertices[i1];
    Vertex v2 = vertices[i2];
    
    // Calculate barycentric coordinates
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    // Interpolate normal using barycentric coordinates
    vec3 normal = normalize(
        barycentrics.x * v0.normal +
        barycentrics.y * v1.normal +
        barycentrics.z * v2.normal
    );
    
    // Interpolate UV coordinates
    vec2 uv = 
        barycentrics.x * v0.uv +
        barycentrics.y * v1.uv +
        barycentrics.z * v2.uv;
    
    // Get texture ID (use first vertex's texture ID)
    uint textureId = v0.textureId;

    // Simple diffuse lighting from sun direction
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDir), 0.2);

    // Shadow ray
    float tMin = 0.001;
    float tMax = 10000.0;
    shadowed = true;

    traceRayEXT(
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF,
        0,  // sbt record offset
        0,  // sbt record stride
        1,  // miss index (shadow miss)
        worldPos + normal * 0.01, // offset to avoid self-intersection
        tMin,
        lightDir,
        tMax,
        1   // payload location (shadowed)
    );

    float shadow = shadowed ? 0.3 : 1.0;

    // Sample texture
    vec3 baseColor;
    if (textureId < 256) {
        baseColor = texture(textures[textureId], uv).rgb;
    } else {
        // Fallback color for invalid texture ID
        baseColor = vec3(0.8, 0.7, 0.6);
    }

    // Final shading
    hitValue = baseColor * diffuse * shadow;
}
