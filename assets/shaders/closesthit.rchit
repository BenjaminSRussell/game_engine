#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// Closest hit shader - handles surface intersection and shading
layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT ShadowPayload shadow_payload;

hitAttributeEXT vec2 attribs; // Barycentric coordinates

// Descriptor bindings
layout(set = 0, binding = 1) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3) uniform sampler2D textures[256];
layout(set = 0, binding = 4) readonly buffer LightBuffer {
    LightData lights[];
} light_buffer;
layout(set = 0, binding = 5) readonly buffer MaterialBuffer {
    MaterialData materials[];
} material_buffer;
layout(set = 0, binding = 2) uniform CameraProperties {
    vec3 position;
    vec3 direction;
    vec3 up;
    float fov;
    float aspect;
    float near_plane;
    float far_plane;
    float time;
    uint frame_count;
    uint max_bounces;
    uint samples_per_pixel;
} camera;

// Data structures
struct RayPayload {
    vec3 radiance;
    vec3 throughput;
    vec3 origin;
    vec3 direction;
    uint depth;
    uint seed;
    float t;
    uint hit_id;
    uint material_id;
};

struct ShadowPayload {
    float visibility;
};

struct LightData {
    vec3 position;
    vec3 color;
    float intensity;
    uint type;
    float radius;
    uint padding;
};

struct MaterialData {
    vec3 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    float ao;
    vec3 emission;
    uint material_id;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
    uint ao;
    uint light;
    uint textureId;
};

// Vertex and index data buffers
layout(binding = 6, set = 0, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 7, set = 0, std430) readonly buffer IndexBuffer {
    uint indices[];
};

// Random number generator (same as raygen)
uint wang_hash(uint seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand_float(inout uint seed) {
    seed = wang_hash(seed);
    return float(seed) / 4294967296.0;
}

vec2 rand_vec2(inout uint seed) {
    return vec2(rand_float(seed), rand_float(seed));
}

// GGX/Trowbridge-Reitz distribution
float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Geometry function (Smith method)
float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel equation
vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// PBR lighting calculation
vec3 calculate_pbr_lighting(vec3 N, vec3 V, vec3 worldPos, MaterialData material, uint seed) {
    vec3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ao;
    vec3 emission = material.emission;
    
    // Calculate reflectance at normal incidence
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    vec3 Lo = vec3(0.0);
    
    // Process each light
    for (int i = 0; i < light_buffer.lights.length(); ++i) {
        LightData light = light_buffer.lights[i];
        
        vec3 L;
        float attenuation;
        
        if (light.type == 0) { // Directional light
            L = normalize(-light.position);
            attenuation = light.intensity;
        } else if (light.type == 1) { // Point light
            vec3 lightDir = light.position - worldPos;
            float distance = length(lightDir);
            L = normalize(lightDir);
            attenuation = light.intensity / (distance * distance);
        } else { // Area light (simplified as point light)
            vec3 lightDir = light.position - worldPos;
            float distance = length(lightDir);
            L = normalize(lightDir);
            attenuation = light.intensity / (distance * distance);
        }
        
        vec3 radiance = light.color * attenuation;
        
        // Check shadow
        float shadow_factor = 1.0;
        if (light.type != 0) { // Don't shadow directional lights (sun)
            shadow_factor = trace_shadow(worldPos + N * 0.01, L, light.radius);
        }
        
        if (shadow_factor > 0.0) {
            vec3 H = normalize(V + L);
            
            // Cook-Torrance BRDF
            float NDF = distribution_ggx(N, H, roughness);
            float G = geometry_smith(N, V, L, roughness);
            vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            
            // Add to outgoing radiance Lo
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow_factor;
        }
    }
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo + emission;
    
    return color;
}

// Trace shadow ray
float trace_shadow(vec3 origin, vec3 direction, float max_distance) {
    shadow_payload.visibility = 1.0;
    
    uint ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    uint cull_mask = 0xFF;
    float tmin = 0.001;
    float tmax = max_distance - 0.001;
    
    traceRayEXT(topLevelAS, ray_flags, cull_mask, 1, 0, 1, origin, tmin, direction, tmax, 1);
    
    return shadow_payload.visibility;
}

// Main closest hit shader
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
    
    // Get texture ID and material ID
    uint textureId = v0.textureId;
    uint materialId = v0.textureId; // For now, use texture ID as material ID
    
    // Create material data
    MaterialData material;
    if (materialId < material_buffer.materials.length()) {
        material = material_buffer.materials[materialId];
    } else {
        // Default material
        material.albedo = vec3(0.8, 0.7, 0.6);
        material.metallic = 0.0;
        material.roughness = 0.8;
        material.ao = 1.0;
        material.emission = vec3(0.0);
    }
    
    // Sample texture for albedo
    if (textureId < 256) {
        vec3 textureColor = texture(textures[textureId], uv).rgb;
        material.albedo *= textureColor;
    }
    
    // Calculate view direction
    vec3 V = normalize(-gl_WorldRayDirectionEXT);
    
    // Calculate PBR lighting
    vec3 color = calculate_pbr_lighting(normal, V, worldPos, material, payload.seed);
    
    // Handle reflections for metallic surfaces
    if (material.metallic > 0.1 && payload.depth < camera.max_bounces) {
        vec3 R = reflect(-V, normal);
        vec3 reflection_color = trace_ray(worldPos + normal * 0.01, R, payload.depth + 1, payload.seed);
        color = mix(color, reflection_color, material.metallic);
    }
    
    // Set payload result
    payload.radiance = color;
    payload.t = gl_HitTEXT;
    payload.hit_id = gl_PrimitiveID;
    payload.material_id = materialId;
}
