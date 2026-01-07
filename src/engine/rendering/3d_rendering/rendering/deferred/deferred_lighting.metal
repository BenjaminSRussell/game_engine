// deferred_lighting.metal
#include <metal_stdlib>
using namespace metal;

// Helper function to decode octahedron encoded normals
static float3 decode_octahedron(float2 e) {
    float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) {
        float2 s = sign(v.xy);
        v.xy = (1.0 - abs(v.yx)) * s;
    }
    return normalize(v);
}

// GGX BRDF
static float3 brdf_ggx(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic) {
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    // float VdotH = max(dot(V, H), 0.0); // Unused in basic version

    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    // D: Trowbridge-Reitz GGX
    float num = alpha2;
    float denom = (NdotH * NdotH * (alpha2 - 1.0) + 1.0);
    denom = M_PI_F * denom * denom;
    float D = num / max(denom, 0.00001);

    // F: Fresnel-Schlick
    float3 F0 = mix(float3(0.04), albedo, metallic);
    float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    // G: Smith's method
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1 = NdotV / (NdotV * (1.0 - k) + k);
    float G2 = NdotL / (NdotL * (1.0 - k) + k);
    float G = G1 * G2;

    float3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    
    float3 specular = numerator / denominator;
    
    // kS is Fresnel, kD is remaining energy
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;

    return (kD * albedo / M_PI_F + specular) * NdotL;
}

struct LightData {
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct LightingUniforms {
    float4x4 inv_view_proj;
    float3 camera_pos;
    uint light_count;
};

vertex float4 deferred_lighting_vertex(uint vertexID [[vertex_id]]) {
    float4 positions[3] = {
        float4(-1.0, -1.0, 0.0, 1.0),
        float4( 3.0, -1.0, 0.0, 1.0),
        float4(-1.0,  3.0, 0.0, 1.0)
    };
    return positions[vertexID];
}

fragment float4 deferred_lighting_fragment(
    float4 position [[position]],
    texture2d<float> gbuffer_albedo [[texture(0)]],
    texture2d<float> gbuffer_normal [[texture(1)]],
    texture2d<float> gbuffer_material [[texture(2)]],
    depth2d<float> gbuffer_depth [[texture(3)]],
    constant LightingUniforms& uniforms [[buffer(0)]],
    constant LightData* lights [[buffer(1)]]
) {
    uint2 coord = uint2(position.xy);

    // Sample G-buffer
    float4 albedo_ao = gbuffer_albedo.read(coord);
    float2 encoded_normal = gbuffer_normal.read(coord).xy;
    float4 material = gbuffer_material.read(coord);
    float depth = gbuffer_depth.read(coord);

    // Reconstruct world position
    float2 uv = float2(coord) / float2(gbuffer_albedo.get_width(), gbuffer_albedo.get_height());
    // Convert 0..1 UV to -1..1 Clip Space
    float4 clip_pos = float4(uv * 2.0 - 1.0, depth, 1.0);
    // Metal NDCs y is down? No, Metal texture coordinates are top-left usually? 
    // Standard clip space Y is up. UV is usually top-left or bottom-left depending on convention.
    // Assuming standard full-screen quad UVs.
    // Flip Y if needed based on projection matrix.
    clip_pos.y = -clip_pos.y; 
    
    float4 world_pos = uniforms.inv_view_proj * clip_pos;
    world_pos /= world_pos.w;

    // Decode normal (octahedron encoding)
    float3 N = decode_octahedron(encoded_normal);
    float3 V = normalize(uniforms.camera_pos - world_pos.xyz);

    float3 albedo = albedo_ao.rgb;
    float ao = albedo_ao.a;
    float roughness = material.r;
    float metallic = material.g;

    // Accumulate lighting
    float3 Lo = float3(0);

    for (uint i = 0; i < uniforms.light_count; i++) {
        LightData light = lights[i];

        float3 L = light.position - world_pos.xyz;
        float distance = length(L);
        L /= distance; // normalize

        if (distance < light.radius) {
            float attenuation = 1.0 / (distance * distance + 1.0);
            attenuation *= saturate(1.0 - distance / light.radius);

            // Basic point light
            float3 radiance = light.color * light.intensity * attenuation;
            Lo += brdf_ggx(N, V, L, albedo, roughness, metallic) * radiance;
        }
    }

    // Ambient
    float3 ambient = albedo * 0.03 * ao;
    float3 color = ambient + Lo;

    // Reinhard tone mapping (simple) - usually done in post-processing, but useful for debug
    // color = color / (color + float3(1.0));
    // gamma correction
    // color = pow(color, float3(1.0/2.2));

    return float4(color, 1.0);
}
