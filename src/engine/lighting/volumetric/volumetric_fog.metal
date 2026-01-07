#include <metal_stdlib>
using namespace metal;

// ============================================================================
// STRUCTURES
// ============================================================================

struct FogUniforms {
    float4x4 inv_view_proj;
    float4x4 view;
    float3 camera_pos;
    float near;
    float far;
    uint3 resolution;
    float density_scale;
    float3 fog_color;
    uint light_count;
    float2 screen_size;
    float time;  // For temporal jitter
    float anisotropy;  // Phase function g-factor
};

struct Light {
    float3 position;
    float3 direction;
    float3 color;
    float range;
    float intensity;
    uint type;  // 0=directional, 1=point, 2=spot
    float2 spot_angles;  // inner_angle, outer_angle
};

struct FogVolume {
    float4x4 transform;
    float4x4 inv_transform;
    float3 extents;
    uint shape;  // 0=box, 1=sphere, 2=capsule
    float density;
    float absorption;
    float scattering;
    float anisotropy;
    float3 color;
    float3 emission;
};

// ============================================================================
// PHASE FUNCTIONS
// ============================================================================

// Henyey-Greenstein phase function
float hg_phase(float cos_theta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * M_PI_F * pow(1.0 + g2 - 2.0 * g * cos_theta, 1.5));
}

// Rayleigh phase function (for atmospheric scattering)
float rayleigh_phase(float cos_theta) {
    return (3.0 / (16.0 * M_PI_F)) * (1.0 + cos_theta * cos_theta);
}

// Isotropic phase function
float isotropic_phase() {
    return 1.0 / (4.0 * M_PI_F);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Convert froxel index to view-space Z
float froxel_z_to_view_z(uint z, uint depth, float near, float far) {
    float slice = (float(z) + 0.5) / float(depth);
    return near * pow(far / near, slice);
}

// Convert view Z to froxel Z index
float view_z_to_froxel_z(float view_z, uint depth, float near, float far) {
    return log(view_z / near) / log(far / near) * float(depth);
}

// Blue noise for jittering (simple hash-based)
float blue_noise(uint2 coord, float time) {
    uint h = coord.x + coord.y * 73856093u;
    h = (h ^ 61u) ^ (h >> 16u);
    h *= 0x27d4eb2du;
    h = (h ^ (h >> 15u)) * uint(time * 1000.0);
    return float(h) * (1.0 / 4294967296.0);
}

// Sample shadow map (simplified - would integrate with actual shadow system)
float sample_shadow(float3 world_pos, constant Light& light) {
    // Placeholder: would sample actual shadow map cascade
    return 1.0;  // No shadow
}

// Evaluate fog volume density at a point
float eval_fog_volume_density(float3 world_pos, constant FogVolume& volume) {
    // Transform to local space
    float4 local_pos4 = volume.inv_transform * float4(world_pos, 1.0);
    float3 local_pos = local_pos4.xyz / local_pos4.w;
    
    float density = 0.0;
    
    switch (volume.shape) {
        case 0: {  // Box
            if (all(abs(local_pos) <= volume.extents)) {
                density = volume.density;
            }
            break;
        }
        case 1: {  // Sphere
            float dist = length(local_pos);
            if (dist <= volume.extents.x) {  // extents.x = radius
                density = volume.density;
            }
            break;
        }
        case 2: {  // Capsule
            float half_height = volume.extents.y * 0.5;
            float clamped_y = clamp(local_pos.y, -half_height, half_height);
            float3 closest = float3(0.0, clamped_y, 0.0);
            float dist = length(local_pos - closest);
            if (dist <= volume.extents.x) {  // extents.x = radius
                density = volume.density;
            }
            break;
        }
    }
    
    return density;
}

// ============================================================================
// KERNELS
// ============================================================================

// Kernel 1: Inject density and lighting into froxels
kernel void inject_lighting(
    uint3 id [[thread_position_in_grid]],
    texture3d<float, access::write> froxel_scattering [[texture(0)]],
    texture3d<float, access::write> froxel_density [[texture(1)]],
    constant FogUniforms& uniforms [[buffer(0)]],
    constant Light* lights [[buffer(1)]],
    constant FogVolume* volumes [[buffer(2)]],
    constant uint& volume_count [[buffer(3)]])
{
    if (any(id >= uniforms.resolution)) return;
    
    // Calculate froxel center in screen space [0,1] and view Z
    float3 uvw = (float3(id) + 0.5) / float3(uniforms.resolution);
    float view_z = froxel_z_to_view_z(id.z, uniforms.resolution.z, uniforms.near, uniforms.far);
    
    // Reconstruct world position
    float2 ndc = uvw.xy * 2.0 - 1.0;
    float4 clip_pos = float4(ndc.x, ndc.y, 0.5, 1.0);  // Mid-depth for froxel
    float4 world_pos4 = uniforms.inv_view_proj * clip_pos;
    float3 world_pos = world_pos4.xyz / world_pos4.w;
    
    // Sample fog volumes
    float total_density = 0.0;
    float3 total_emission = float3(0.0);
    float total_scattering = 0.0;
    float3 total_color = float3(0.0);
    
    for (uint v = 0; v < volume_count; v++) {
        float density = eval_fog_volume_density(world_pos, volumes[v]);
        if (density > 0.0) {
            total_density += density;
            total_emission += volumes[v].emission * density;
            total_scattering += volumes[v].scattering * density;
            total_color += volumes[v].color * density;
        }
    }
    
    // Add global fog
    total_density += uniforms.density_scale;
    total_color += uniforms.fog_color * uniforms.density_scale;
    total_scattering += uniforms.density_scale * 0.1;
    
    // Normalize color
    if (total_density > 0.0) {
        total_color /= total_density;
    }
    
    // Calculate in-scattering from lights
    float3 view_dir = normalize(world_pos - uniforms.camera_pos);
    float3 inscattering = total_emission;
    
    for (uint i = 0; i < uniforms.light_count; i++) {
        constant Light& light = lights[i];
        
        float3 light_dir;
        float attenuation = 1.0;
        
        if (light.type == 0) {  // Directional
            light_dir = normalize(-light.direction);
        } else {  // Point or Spot
            float3 to_light = light.position - world_pos;
            float dist = length(to_light);
            light_dir = to_light / dist;
            
            // Distance attenuation
            attenuation = 1.0 - saturate(dist / light.range);
            attenuation *= attenuation;
            
            // Spot light cone
            if (light.type == 2) {
                float spot_factor = dot(light_dir, -light.direction);
                float inner = cos(light.spot_angles.x);
                float outer = cos(light.spot_angles.y);
                attenuation *= saturate((spot_factor - outer) / (inner - outer));
            }
        }
        
        if (attenuation > 0.0) {
            // Phase function
            float cos_theta = dot(view_dir, light_dir);
            float phase = hg_phase(cos_theta, uniforms.anisotropy);
            
            // Shadow sampling
            float shadow = sample_shadow(world_pos, light);
            
            // Accumulate in-scattering
            inscattering += light.color * light.intensity * total_scattering * phase * attenuation * shadow;
        }
    }
    
    // Write to froxel grid
    float4 scattering_value = float4(inscattering * total_color, total_density);
    froxel_scattering.write(scattering_value, id);
    froxel_density.write(float4(total_density), id);
}

// Kernel 2: Ray march integration
kernel void integrate_scattering(
    uint3 id [[thread_position_in_grid]],
    texture3d<float, access::read> froxel_scattering [[texture(0)]],
   texture3d<float, access::write> integrated_scattering [[texture(1)]],
    constant FogUniforms& uniforms [[buffer(0)]])
{
    if (any(id.xy >= uniforms.resolution.xy)) return;
    
    // Add temporal jitter
    float jitter = blue_noise(id.xy, uniforms.time);
    
    float3 accum_scattering = float3(0.0);
    float accum_transmittance = 1.0;
    
    // March through depth slices from front to back
    for (uint z = 0; z < uniforms.resolution.z; z++) {
        uint3 coord = uint3(id.xy, z);
        float4 sample = froxel_scattering.read(coord);
        
        float3 inscattering = sample.rgb;
        float extinction = sample.a;
        
        // Calculate step size in view space
        float z_near = froxel_z_to_view_z(z, uniforms.resolution.z, uniforms.near, uniforms.far);
        float z_far = froxel_z_to_view_z(z + 1, uniforms.resolution.z, uniforms.near, uniforms.far);
        float step_size = z_far - z_near;
        
        // Beer-Lambert law: transmittance = exp(-extinction * distance)
        float transmittance = exp(-extinction * step_size);
        
        // Integrate in-scattering
        float extinction_clamped = max(extinction, 0.0001);
        float visibility = (1.0 - transmittance) / extinction_clamped;
        
        accum_scattering += inscattering * accum_transmittance * visibility;
        accum_transmittance *= transmittance;
        
        // Write accumulated values for this depth
        integrated_scattering.write(float4(accum_scattering, accum_transmittance), coord);
        
        // Early exit if fully opaque
        if (accum_transmittance < 0.001) {
            break;
        }
    }
}

// Fragment shader: Apply volumetric fog to scene
fragment float4 apply_volumetric_fog(
    float4 position [[position]],
    texture2d<float> scene_color [[texture(0)]],
    texture2d<float> scene_depth [[texture(1)]],
    texture3d<float> integrated_fog [[texture(2)]],
    constant FogUniforms& uniforms [[buffer(0)]],
    sampler linear_sampler [[sampler(0)]])
{
    uint2 pixel_coord = uint2(position.xy);
    float4 scene = scene_color.read(pixel_coord);
    float depth = scene_depth.read(pixel_coord).r;
    
    // Convert depth to view space
    float linear_depth = uniforms.near * uniforms.far / (uniforms.far - depth * (uniforms.far - uniforms.near));
    
    // Convert to froxel Z coordinate
    float froxel_z = view_z_to_froxel_z(linear_depth, uniforms.resolution.z, uniforms.near, uniforms.far);
    froxel_z = clamp(froxel_z / float(uniforms.resolution.z), 0.0, 1.0);
    
    // Sample integrated fog at scene depth
    float3 uvw = float3(float2(pixel_coord) / uniforms.screen_size, froxel_z);
    float4 fog = integrated_fog.sample(linear_sampler, uvw);
    
    // Composite: scene_color * transmittance + inscattering
    float3 final_color = scene.rgb * fog.a + fog.rgb;
    
    return float4(final_color, scene.a);
}
