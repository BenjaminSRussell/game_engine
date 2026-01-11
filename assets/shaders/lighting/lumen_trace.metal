// Lumen Screen-Space and Ray Tracing Shader
#include <metal_stdlib>
using namespace metal;

struct LumenProbe {
    float3 position;
    float radius;
    float4 radiance[6];  // Spherical harmonics coefficients
    float last_update_time;
};

struct SurfaceCache {
    float3 position;
    float3 normal;
    float3 albedo;
    float3 emissive;
    float roughness;
    float metallic;
};

// Screen-space GI trace
kernel void lumen_trace_screen_space(
    texture2d<float, access::read> depth [[texture(0)]],
    texture2d<float, access::read> normals [[texture(1)]],
    texture2d<float, access::read> albedo [[texture(2)]],
    texture2d<float, access::write> output_diffuse [[texture(3)]],
    texture2d<float, access::write> output_specular [[texture(4)]],
    constant float4x4& inv_view_proj [[buffer(0)]],
    constant float4x4& view_proj [[buffer(1)]],
    constant LumenProbe* probes [[buffer(2)]],
    constant uint& probe_count [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 size = uint2(depth.get_width(), depth.get_height());
    if (gid.x >= size.x || gid.y >= size.y) return;
    
    // Read G-buffer
    float depth_val = depth.read(gid).r;
    float3 normal = normals.read(gid).rgb * 2.0 - 1.0;
    float3 base_color = albedo.read(gid).rgb;
    
    if (depth_val >= 1.0) {
        // Sky
        output_diffuse.write(float4(0.1, 0.15, 0.3, 1.0), gid);
        return;
    }
    
    // Reconstruct world position
    float2 uv = (float2(gid) + 0.5) / float2(size);
    float2 ndc = uv * 2.0 - 1.0;
    float4 clip_pos = float4(ndc.x, -ndc.y, depth_val, 1.0);
    float4 world_pos4 = inv_view_proj * clip_pos;
    float3 world_pos = world_pos4.xyz / world_pos4.w;
    
    // Find nearest probe
    float3 gi = float3(0.0);
    float min_dist = 1000000.0;
    int nearest_probe = -1;
    
    for (uint i = 0; i < probe_count; i++) {
        float dist = length(probes[i].position - world_pos);
        if (dist < min_dist && dist < probes[i].radius) {
            min_dist = dist;
            nearest_probe = i;
        }
    }
    
    if (nearest_probe >= 0) {
        // Sample probe radiance
        LumenProbe probe = probes[nearest_probe];
        
        // Simple spherical harmonic lookup (would use proper SH evaluation)
        float3 dir = normalize(normal);
        float weight = max(dot(dir, float3(0, 1, 0)), 0.0);
        gi = probe.radiance[0].rgb * weight;
        
        // Falloff based on distance
        float falloff = 1.0 - saturate(min_dist / probe.radius);
        gi *= falloff;
    }
    
    // Combine with albedo
    float3 diffuse = base_color * gi;
    
    output_diffuse.write(float4(diffuse, 1.0), gid);
    output_specular.write(float4(0.0), gid);
}

// Probe update with ray tracing
kernel void lumen_update_probes(
    device LumenProbe* probes [[buffer(0)]],
    constant SurfaceCache* surfaces [[buffer(1)]],
    constant uint& surface_count [[buffer(2)]],
    constant float& delta_time [[buffer(3)]],
    uint probe_id [[thread_position_in_grid]]
) {
    LumenProbe probe = probes[probe_id];
    
    // Trace rays from probe position
    float3 radiance = float3(0.0);
    const uint ray_count = 64;
    
    for (uint i = 0; i < ray_count; i++) {
        // Generate ray direction (uniform hemisphere)
        float phi = (float(i) / float(ray_count)) * 2.0 * M_PI_F;
        float theta = acos(1.0 - 2.0 * (float(i) / float(ray_count)));
        
        float3 ray_dir = float3(
            sin(theta) * cos(phi),
            cos(theta),
            sin(theta) * sin(phi)
        );
        
        // Find closest surface
        float min_t = 10000.0;
        int hit_surface = -1;
        
        for (uint j = 0; j < surface_count; j++) {
            SurfaceCache surf = surfaces[j];
            
            // Simple sphere intersection (would use proper ray tracing)
            float3 to_surf = surf.position - probe.position;
            float t = dot(to_surf, ray_dir);
            
            if (t > 0.0 && t < min_t) {
                float3 closest = probe.position + ray_dir * t;
                if (length(closest - surf.position) < 0.5) {
                    min_t = t;
                    hit_surface = j;
                }
            }
        }
        
        if (hit_surface >= 0) {
            SurfaceCache surf = surfaces[hit_surface];
            radiance += surf.albedo * surf.emissive;
        }
    }
    
    // Average and update probe
    radiance /= float(ray_count);
    
    // Temporal blend
    float blend = 0.1 * delta_time;
    probe.radiance[0].rgb = mix(probe.radiance[0].rgb, radiance, blend);
    probe.last_update_time += delta_time;
    
    probes[probe_id] = probe;
}
