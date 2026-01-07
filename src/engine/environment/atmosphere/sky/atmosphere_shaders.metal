#include <metal_stdlib>
#include "atmosphere_common.h"

using namespace metal;

// Constants
constant float PI = 3.14159265359;

// =============================================================================
// Helper Functions
// =============================================================================

// Ray-sphere intersection returning both distances
bool ray_sphere_intersect_full(float3 origin, float3 dir, float radius, thread float* t1, thread float* t2) {
    float b = dot(origin, dir);
    float c = dot(origin, origin) - radius * radius;
    float d = b * b - c;
    if (d < 0.0) return false;
    float sqrt_d = sqrt(d);
    *t1 = -b - sqrt_d;
    *t2 = -b + sqrt_d;
    return true;
}

// Ray-sphere intersection (simplified)
float ray_sphere_intersect(float3 origin, float3 dir, float radius) {
    float t1, t2;
    if (!ray_sphere_intersect_full(origin, dir, radius, &t1, &t2)) return -1.0;
    return (t2 > 0.0) ? t2 : t1;
}

// Horizon angle cosine
float horizon_angle_cos(float height, float planet_radius) {
    if (height <= planet_radius) return -1.0;
    float discriminant = height * height - planet_radius * planet_radius;
    return -sqrt(max(0.0, discriminant)) / height;
}

// Density functions
float3 get_density(float altitude, constant AtmosphereParams& params) {
    float rho_rayleigh = exp(-altitude / params.rayleigh_scale_height);
    float rho_mie = exp(-altitude / params.mie_scale_height);
    
    // Ozone layer (tent function centered at 25km)
    float ozone_altitude = altitude - 25.0;
    float rho_ozone = max(0.0, 1.0 - abs(ozone_altitude) / 15.0);
    
    return float3(rho_rayleigh, rho_mie, rho_ozone);
}

// =============================================================================
// LUT Parameterization
// =============================================================================

float2 transmittance_lut_encode(float height, float view_zenith_cos, constant AtmosphereParams& params) {
    float h = (height - params.planet_radius) / params.atmosphere_height;
    h = clamp(h, 0.0, 1.0);
    float u_height = sqrt(h); // Non-linear for better precision at low altitude
    
    float horizon_cos = horizon_angle_cos(height, params.planet_radius);
    float v_zenith;
    if (view_zenith_cos > horizon_cos) {
        v_zenith = 0.5 + 0.5 * (view_zenith_cos - horizon_cos) / (1.0 - horizon_cos);
    } else {
        v_zenith = 0.5 * (view_zenith_cos + 1.0) / (horizon_cos + 1.0);
    }
    
    return float2(u_height, v_zenith);
}

void transmittance_lut_decode(float2 uv, constant AtmosphereParams& params, 
                               thread float* height, thread float* view_zenith_cos) {
    float h = uv.x * uv.x; // Inverse of sqrt
    *height = params.planet_radius + h * params.atmosphere_height;
    
    float horizon_cos = horizon_angle_cos(*height, params.planet_radius);
    if (uv.y > 0.5) {
        float t = (uv.y - 0.5) * 2.0;
        *view_zenith_cos = horizon_cos + t * (1.0 - horizon_cos);
    } else {
        float t = uv.y * 2.0;
        *view_zenith_cos = -1.0 + t * (horizon_cos + 1.0);
    }
}

// =============================================================================
// Transmittance LUT (Improved)
// =============================================================================

kernel void compute_transmittance(
    texture2d<float, access::write> transmittance [[texture(0)]],
    constant AtmosphereParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= TRANSMITTANCE_WIDTH || gid.y >= TRANSMITTANCE_HEIGHT) return;

    float2 uv = (float2(gid) + 0.5) / float2(TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT);

    // Decode UV to height and view zenith
    float h, cos_theta;
    transmittance_lut_decode(uv, params, &h, &cos_theta);

    float3 origin = float3(0, h, 0);
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));
    float3 dir = float3(sin_theta, cos_theta, 0);

    // Calculate ray march distance
    float t_max = ray_sphere_intersect(origin, dir, params.planet_radius + params.atmosphere_height);
    
    // Check if ray hits ground
    float t_ground = ray_sphere_intersect(origin, dir, params.planet_radius);
    if (t_ground > 0.0 && t_ground < t_max) {
        t_max = t_ground;
    }
    
    if (t_max < 0.0) t_max = 0.0;

    // Integrate optical depth with more samples for accuracy
    float3 optical_depth = float3(0);
    int steps = 128; // Increased from 64
    float dt = t_max / float(steps);

    for (int i = 0; i < steps; i++) {
        float3 pos = origin + dir * (float(i) + 0.5) * dt;
        float altitude = length(pos) - params.planet_radius;
        
        if (altitude < 0.0 || altitude > params.atmosphere_height) continue;
        
        float3 density = get_density(altitude, params);
        
        // Rayleigh extinction
        optical_depth += params.rayleigh_coeff * density.x * dt;
        
        // Mie extinction  
        optical_depth += float3(params.mie_coeff) * density.y * dt;
        
        // Ozone absorption (approximation)
        float3 ozone_coeff = float3(0.00065, 0.00018, 0.00008); // km^-1
        optical_depth += ozone_coeff * density.z * dt;
    }

    float3 transmittance_value = exp(-optical_depth);
    transmittance.write(float4(transmittance_value, 1.0), gid);
}

// =============================================================================
// Phase Functions
// =============================================================================

float rayleigh_phase(float cos_theta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cos_theta * cos_theta);
}

float mie_phase_hg(float cos_theta, float g) {
    float g_sq = g * g;
    float denom = 1.0 + g_sq - 2.0 * g * cos_theta;
    return (1.0 - g_sq) / (4.0 * PI * pow(denom, 1.5));
}

// =============================================================================
// Single Scattering LUT (Improved)
// =============================================================================

kernel void compute_scattering(
    texture3d<float, access::write> scattering [[texture(0)]],
    texture2d<float, access::read> transmittance_lut [[texture(1)]],
    constant AtmosphereParams& params [[buffer(0)]],
    uint3 gid [[thread_position_in_grid]]
) {
    if (gid.x >= SCATTERING_WIDTH || gid.y >= SCATTERING_HEIGHT || gid.z >= SCATTERING_DEPTH) return;

    float3 uv = (float3(gid) + 0.5) / float3(SCATTERING_WIDTH, SCATTERING_HEIGHT, SCATTERING_DEPTH);

    // Decode LUT parameters
    float h = params.planet_radius + uv.y * params.atmosphere_height;
    float cos_view_zenith = uv.x * 2.0 - 1.0;
    float cos_sun_zenith = uv.z * 2.0 - 1.0;

    float3 origin = float3(0, h, 0);
    float sin_view = sqrt(max(0.0, 1.0 - cos_view_zenith * cos_view_zenith));
    float3 view_dir = float3(sin_view, cos_view_zenith, 0);
    
    float sin_sun = sqrt(max(0.0, 1.0 - cos_sun_zenith * cos_sun_zenith));
    float3 sun_dir = float3(sin_sun, cos_sun_zenith, 0);

    // Calculate scattering angle
    float cos_theta = dot(view_dir, sun_dir);
    
    // Phase functions
    float phase_r = rayleigh_phase(cos_theta);
    float phase_m = mie_phase_hg(cos_theta, params.mie_g);

    // Ray march distance
    float t_max = ray_sphere_intersect(origin, view_dir, params.planet_radius + params.atmosphere_height);
    float t_ground = ray_sphere_intersect(origin, view_dir, params.planet_radius);
    if (t_ground > 0.0 && t_ground < t_max) t_max = t_ground;
    if (t_max < 0.0) t_max = 0.0;

    float3 inscatter_rayleigh = float3(0);
    float3 inscatter_mie = float3(0);

    int steps = 32;
    float dt = t_max / float(steps);
    
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);

    for (int i = 0; i < steps; i++) {
        float t = (float(i) + 0.5) * dt;
        float3 sample_pos = origin + view_dir * t;
        float sample_height = length(sample_pos);
        float sample_altitude = sample_height - params.planet_radius;
        
        if (sample_altitude < 0.0 || sample_altitude > params.atmosphere_height) continue;
        
        float3 density = get_density(sample_altitude, params);
        
        // Sample transmittance from camera to sample point
        float2 trans_uv_view = transmittance_lut_encode(sample_height, cos_view_zenith, params);
        float3 trans_view = transmittance_lut.sample(s, trans_uv_view).rgb;
        
        // Sample transmittance from sample point to sun
        float cos_sun_at_sample = dot(normalize(sample_pos), sun_dir);
        float2 trans_uv_sun = transmittance_lut_encode(sample_height, cos_sun_at_sample, params);
        float3 trans_sun = transmittance_lut.sample(s, trans_uv_sun).rgb;
        
        // Accumulate in-scattering
        inscatter_rayleigh += density.x * trans_view * trans_sun * dt;
        inscatter_mie += density.y * trans_view * trans_sun * dt;
    }

    // Apply scattering coefficients and phase functions
    float3 final_inscatter = 
        params.rayleigh_coeff * inscatter_rayleigh * phase_r +
        float3(params.mie_coeff) * inscatter_mie * phase_m;

    scattering.write(float4(final_inscatter * params.sun_intensity, 1.0), gid);
}

// =============================================================================
// Multi-Scattering LUT
// =============================================================================

kernel void compute_multiscattering(
    texture2d<float, access::write> multiscatter_lut [[texture(0)]],
    texture2d<float, access::read> transmittance_lut [[texture(1)]],
    constant AtmosphereParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= MULTISCATTER_WIDTH || gid.y >= MULTISCATTER_HEIGHT) return;
    
    float2 uv = (float2(gid) + 0.5) / float2(MULTISCATTER_WIDTH, MULTISCATTER_HEIGHT);
    
    // Decode UV to height and sun zenith
    float h = params.planet_radius + uv.x * params.atmosphere_height;
    float cos_sun_zenith = uv.y * 2.0 - 1.0;
    
    float3 origin = float3(0, h, 0);
    float3 sun_dir = float3(sqrt(max(0.0, 1.0 - cos_sun_zenith * cos_sun_zenith)), cos_sun_zenith, 0);
    
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    
    // Multiple scattering approximation using spherical integration
    // We integrate over all viewing directions to get average second-order scattering
    
    float3 multi_scatter = float3(0);
    int num_zenith_samples = 16;
    int num_azimuth_samples = 32;
    
    for (int i = 0; i < num_zenith_samples; i++) {
        float zenith_angle = (float(i) + 0.5) * PI / float(num_zenith_samples);
        float cos_zenith = cos(zenith_angle);
        float sin_zenith = sin(zenith_angle);
        
        for (int j = 0; j < num_azimuth_samples; j++) {
            float azimuth = (float(j) + 0.5) * 2.0 * PI / float(num_azimuth_samples);
            
            // View direction
            float3 view_dir = float3(
                sin_zenith * cos(azimuth),
                cos_zenith,
                sin_zenith * sin(azimuth)
            );
            
            // Ray march distance
            float t_max = ray_sphere_intersect(origin, view_dir, params.planet_radius + params.atmosphere_height);
            float t_ground = ray_sphere_intersect(origin, view_dir, params.planet_radius);
            if (t_ground > 0.0 && t_ground < t_max) t_max = t_ground;
            if (t_max < 0.0) continue;
            
            // Single-bounce scattering along this ray
            float3 inscatter_ray = float3(0);
            int steps = 16; // Reduced samples for performance
            float dt = t_max / float(steps);
            
            float cos_theta = dot(view_dir, sun_dir);
            float phase_r = rayleigh_phase(cos_theta);
            float phase_m = mie_phase_hg(cos_theta, params.mie_g);
            
            for (int k = 0; k < steps; k++) {
                float t = (float(k) + 0.5) * dt;
                float3 sample_pos = origin + view_dir * t;
                float sample_height = length(sample_pos);
                float sample_altitude = sample_height - params.planet_radius;
                
                if (sample_altitude < 0.0 || sample_altitude > params.atmosphere_height) continue;
                
                float3 density = get_density(sample_altitude, params);
                
                // Transmittance from view point to sample
                float2 trans_uv_view = transmittance_lut_encode(sample_height, cos_zenith, params);
                float3 trans_view = transmittance_lut.sample(s, trans_uv_view).rgb;
                
                // Transmittance from sample to sun
                float cos_sun_at_sample = dot(normalize(sample_pos), sun_dir);
                float2 trans_uv_sun = transmittance_lut_encode(sample_height, cos_sun_at_sample, params);
                float3 trans_sun = transmittance_lut.sample(s, trans_uv_sun).rgb;
                
                // Single scattering at this point
                float3 single_scatter = 
                    params.rayleigh_coeff * density.x * phase_r +
                    float3(params.mie_coeff) * density.y * phase_m;
                
                inscatter_ray += single_scatter * trans_view * trans_sun * dt;
            }
            
            // Accumulate for spherical integration
            // Weight by solid angle: sin(zenith) * d_zenith * d_azimuth
            float weight = sin_zenith;
            multi_scatter += inscatter_ray * weight;
        }
    }
    
    // Normalize by total solid angle (4π)
    multi_scatter *= (PI / float(num_zenith_samples)) * (2.0 * PI / float(num_azimuth_samples));
    
    // Second-order approximation: multiply by albedo factor
    // This represents light scattered twice before reaching the camera
    float3 albedo = params.rayleigh_coeff / (params.rayleigh_coeff + float3(params.mie_coeff) + 0.001);
    multi_scatter *= albedo * params.sun_intensity * 0.5; // 0.5 factor for energy conservation
    
    multiscatter_lut.write(float4(multi_scatter, 1.0), gid);
}

// =============================================================================
// Sky View LUT
// =============================================================================

kernel void compute_skyview_lut(
    texture2d<float, access::write> skyview_lut [[texture(0)]],
    texture2d<float, access::read> transmittance_lut [[texture(1)]],
    texture3d<float, access::read> scattering_lut [[texture(2)]],
    texture2d<float, access::read> multiscatter_lut [[texture(3)]],
    constant AtmosphereParams& params [[buffer(0)]],
    constant SkyUniforms& uniforms [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= SKYVIEW_WIDTH || gid.y >= SKYVIEW_HEIGHT) return;
    
    float2 uv = (float2(gid) + 0.5) / float2(SKYVIEW_WIDTH, SKYVIEW_HEIGHT);
    
    // Decode UV to view direction
    // U = view zenith angle, V = azimuth angle
    float camera_height = length(uniforms.camera_pos);
    float camera_altitude = camera_height - uniforms.planet_radius;
    
    float horizon_cos = horizon_angle_cos(camera_height, uniforms.planet_radius);
    
    // Decode zenith angle with horizon correction
    float cos_view_zenith;
    if (uv.y > 0.5) {
        // Above horizon
        float t = (uv.y - 0.5) * 2.0;
        t = sqrt(t); // Non-linear for better horizon detail
        cos_view_zenith = horizon_cos + t * (1.0 - horizon_cos);
    } else {
        // Below horizon (underground views)
        float t = uv.y * 2.0;
        t = sqrt(t);
        cos_view_zenith = horizon_cos - t * (horizon_cos + 1.0);
    }
    
    // Decode azimuth
    float azimuth = uv.x * 2.0 * PI;
    
    // Construct view direction
    float sin_view_zenith = sqrt(max(0.0, 1.0 - cos_view_zenith * cos_view_zenith));
    float3 view_dir = float3(
        sin_view_zenith * sin(azimuth),
        cos_view_zenith,
        sin_view_zenith * cos(azimuth)
    );
    
    // Ray march through atmosphere
    float3 origin = uniforms.camera_pos;
    float t_max = ray_sphere_intersect(origin, view_dir, uniforms.planet_radius + params.atmosphere_height);
    float t_ground = ray_sphere_intersect(origin, view_dir, uniforms.planet_radius);
    if (t_ground > 0.0 && t_ground < t_max) t_max = t_ground;
    
    if (t_max < 0.0) {
        // Looking into space
        skyview_lut.write(float4(0, 0, 0, 1), gid);
        return;
    }
    
    constexpr sampler samp(coord::normalized, address::clamp_to_edge, filter::linear);
    
    float3 sky_color = float3(0);
    int steps = 16; // Moderate sample count
    float dt = t_max / float(steps);
    
    float3 up_dir = normalize(uniforms.camera_pos);
    float cos_sun_zenith = dot(up_dir, uniforms.sun_direction);
    
    for (int i = 0; i < steps; i++) {
        float t = (float(i) + 0.5) * dt;
        float3 sample_pos = origin + view_dir * t;
        float sample_height = length(sample_pos);
        float sample_altitude = sample_height - uniforms.planet_radius;
        
        if (sample_altitude < 0.0) break; // Hit ground
        if (sample_altitude > params.atmosphere_height) continue;
        
        // Sample scattering LUT
        float u_view = (cos_view_zenith + 1.0) * 0.5;
        float v_height = sample_altitude / params.atmosphere_height;
        float w_sun = (cos_sun_zenith + 1.0) * 0.5;
        float3 scatter_uvw = float3(u_view, v_height, w_sun);
        
        float3 inscatter = scattering_lut.sample(samp, scatter_uvw).rgb;
        
        // Sample multi-scattering contribution
        float2 multi_uv = float2(v_height, (cos_sun_zenith + 1.0) * 0.5);
        float3 multi_inscatter = multiscatter_lut.sample(samp, multi_uv).rgb;
        
        // Sample transmittance to camera
        float2 trans_uv = transmittance_lut_encode(sample_height, cos_view_zenith, params);
        float3 transmittance = transmittance_lut.sample(samp, trans_uv).rgb;
        
        // Accumulate
        sky_color += (inscatter + multi_inscatter) * transmittance * dt;
    }
    
    skyview_lut.write(float4(sky_color, 1.0), gid);
}

// =============================================================================
// Camera Volume LUT (Aerial Perspective)
// =============================================================================

kernel void compute_camera_volume_lut(
    texture3d<float, access::write> camera_volume_lut [[texture(0)]],
    texture2d<float, access::read> transmittance_lut [[texture(1)]],
    texture3d<float, access::read> scattering_lut [[texture(2)]],
    constant AtmosphereParams& params [[buffer(0)]],
    constant SkyUniforms& uniforms [[buffer(1)]],
    uint3 gid [[thread_position_in_grid]]
) {
    if (gid.x >= CAMERA_VOLUME_WIDTH || gid.y >= CAMERA_VOLUME_HEIGHT || gid.z >= CAMERA_VOLUME_DEPTH) return;
    
    // 1. Calculate world position of the froxel
    float2 screen_uv = (float2(gid.xy) + 0.5) / float2(CAMERA_VOLUME_WIDTH, CAMERA_VOLUME_HEIGHT);
    float w_depth = (float(gid.z) + 0.5) / float(CAMERA_VOLUME_DEPTH);
    
    // Exponential depth distribution
    float near_plane = 1.0; // 1 meter
    float far_plane = 100000.0; // 100 km
    float view_depth = near_plane * pow(far_plane / near_plane, w_depth);
    
    // Reconstruct world position
    float2 ndc = screen_uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    float4 clip = float4(ndc, 1.0, 1.0);
    float4 world_h = uniforms.inv_view_proj * clip;
    float3 world_dir = normalize(world_h.xyz / world_h.w - uniforms.camera_pos);
    
    // 2. Raymarch from camera to target
    float3 origin = uniforms.camera_pos;
    float t_max = view_depth;
    
    // Check ground intersection
    float t_ground = ray_sphere_intersect(origin, world_dir, uniforms.planet_radius);
    if (t_ground > 0.0 && t_ground < t_max) t_max = t_ground;
    
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    
    float3 accumulated_scattering = float3(0);
    float3 accumulated_transmittance = float3(1.0);
    
    int steps = 16;
    float dt = t_max / float(steps);
    
    for (int i = 0; i < steps; i++) {
        float t = (float(i) + 0.5) * dt;
        float3 sample_pos = origin + world_dir * t;
        float sample_height = length(sample_pos);
        float sample_altitude = sample_height - uniforms.planet_radius;
        
        if (sample_altitude < 0.0 || sample_altitude > params.atmosphere_height) continue;
        
        float3 density = get_density(sample_altitude, params);
        
        // Extinction at this point
        float3 extinction = 
            params.rayleigh_coeff * density.x +
            float3(params.mie_coeff) * density.y +
            float3(0.00065, 0.00018, 0.00008) * density.z;
            
        float3 trans_step = exp(-extinction * dt);
        
        // Sample transmittance from sample to sun
        float3 up = normalize(sample_pos);
        float cos_sun_zenith = dot(up, uniforms.sun_direction);
        float2 trans_uv_sun = transmittance_lut_encode(sample_height, cos_sun_zenith, params);
        float3 trans_sun = transmittance_lut.sample(s, trans_uv_sun).rgb;
        
        // Calculate single scattering
        float cos_theta = dot(world_dir, uniforms.sun_direction);
        float phase_r = rayleigh_phase(cos_theta);
        float phase_m = mie_phase_hg(cos_theta, params.mie_g);
        
        float3 single_scatter = 
            (params.rayleigh_coeff * density.x * phase_r + 
             float3(params.mie_coeff) * density.y * phase_m) * 
            trans_sun * uniforms.sun_intensity;
            
        float3 step_scatter = single_scatter * dt;
        accumulated_scattering += step_scatter * accumulated_transmittance;
        accumulated_transmittance *= trans_step;
    }
    
    camera_volume_lut.write(float4(accumulated_scattering, accumulated_transmittance.r), gid);
}

// =============================================================================
// Sky Rendering
// =============================================================================

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex VertexOut sky_vertex(uint vid [[vertex_id]]) {
    // Fullscreen triangle
    float2 grid = float2((vid << 1) & 2, vid & 2);
    VertexOut out;
    out.position = float4(grid * 2.0 - 1.0, 0.0, 1.0);
    out.uv = grid;
    return out;
}

fragment float4 sky_fragment(
    VertexOut in [[stage_in]],
    constant SkyUniforms& uniforms [[buffer(0)]],
    texture2d<float> transmittance_lut [[texture(0)]],
    texture3d<float> scattering_lut [[texture(1)]],
    texture2d<float> skyview_lut [[texture(2)]]
) {
    // Reconstruct view direction from screen UV
    float2 ndc = in.uv * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y for correct orientation
    float4 clip = float4(ndc, 1.0, 1.0);
    float4 world_h = uniforms.inv_view_proj * clip;
    float3 world_pos = world_h.xyz / world_h.w;
    float3 view_dir = normalize(world_pos - uniforms.camera_pos);
    
    // Camera altitude
    float camera_height = length(uniforms.camera_pos);
    // float camera_altitude = camera_height - uniforms.planet_radius;
    
    // Calculate view zenith angle
    float3 up_dir = normalize(uniforms.camera_pos);
    float cos_view_zenith = dot(view_dir, up_dir);
    float horizon_cos = horizon_angle_cos(camera_height, uniforms.planet_radius);
    
    // Sample Sky View LUT
    constexpr sampler samp(coord::normalized, address::clamp_to_edge, filter::linear);
    
    // Compute UV based on view direction (same encoding as compute shader)
    // Azimuth
    float azimuth = atan2(view_dir.x, view_dir.z); // Need basis relative to camera?
    // Simplified: Project view_dir onto tangent plane
    // For now assuming Y is up, world space generic
    // We used simple azimuth in compute shader: uv.x * 2PI
    // We need consistent mapping.
    // Let's assume standard mapping:
    float3 right = cross(up_dir, normalize(float3(0,0,1))); // Placeholder north?
    // Actually compute_skyview_lut iterated azimuth 0..2PI relative to some frame.
    // Ideally we align this with sun direction or just world.
    if (azimuth < 0.0) azimuth += 2.0 * PI;
    float u_azimuth = azimuth / (2.0 * PI);
    
    // Zenith
    float v_zenith;
    if (cos_view_zenith > horizon_cos) {
        float t = (cos_view_zenith - horizon_cos) / (1.0 - horizon_cos);
        v_zenith = 0.5 + 0.5 * sqrt(t);
    } else {
        float t = (horizon_cos - cos_view_zenith) / (horizon_cos + 1.0);
        v_zenith = 0.5 - 0.5 * sqrt(t);
    }
    
    float3 sky_color = skyview_lut.sample(samp, float2(u_azimuth, v_zenith)).rgb;
    
    // Add sun disk
    float cos_sun_angle = dot(view_dir, uniforms.sun_direction);
    float sun_angle = acos(clamp(cos_sun_angle, -1.0, 1.0));
   
    // Sun angular diameter: ~0.53 degrees = 0.0093 radians
    float sun_angular_radius = 0.00465;
    
    // Limb darkening effect
    float sun_disk_mask = smoothstep(sun_angular_radius, sun_angular_radius * 0.95, sun_angle);
    float limb_darkening = 1.0 - 0.6 * (1.0 - sun_disk_mask);
    
    // Sun intensity with limb darkening
    float3 sun_color = uniforms.sun_intensity * sun_disk_mask * limb_darkening * 20.0;
    
    // Sun corona (soft glow around sun)
    float corona_size = sun_angular_radius * 3.0;
    float corona_contribution = exp(-sun_angle * sun_angle / (corona_size * corona_size));
    float3 corona_color = uniforms.sun_intensity * corona_contribution * 0.5;
    
    sky_color += sun_color + corona_color;
    
    // Exposure adjustment for HDR
    sky_color = sky_color * 0.01; // Scale down for HDR tone mapping
    
    return float4(sky_color, 1.0);
}

// Aerial Perspective
fragment float4 aerial_perspective_fragment(
    VertexOut in [[stage_in]],
    constant SkyUniforms& uniforms [[buffer(0)]],
    texture3d<float> camera_volume_lut [[texture(0)]],
    depth2d<float> depth_tex [[texture(1)]]
) {
    constexpr sampler s(coord::normalized, address::clamp_to_edge, filter::linear);
    
    // 1. Reconstruct world position from depth
    float depth = depth_tex.sample(sampler(filter::nearest), in.uv);
    
    // Calculate linear depth for volume lookup assumption
    // We need to match the 'view_depth' calculation in compute shader
    // For standard projection, we can reconstruct w or linear z.
    // Assuming standard perspective projection:
    // z_ndc = clip.z / clip.w
    // clip.w = -view_z (for right handed view)
    
    float4 clip = float4(in.uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;
    float4 view_pos_h = uniforms.inv_view_proj * clip; // This might go to world directly if inv_view_proj is used
    // Wait, uniforms.inv_view_proj takes clip to world? Yes usually.
    // But for depth we need distance from camera.
    float3 world_pos = view_pos_h.xyz / view_pos_h.w;
    float dist = length(world_pos - uniforms.camera_pos);
    
    // 2. Map distance to W coordinate of Volume LUT
    // Matching compute shader: view_depth = near * (far/near)^w
    // w = log(view_depth / near) / log(far/near)
    float near_plane = 1.0;
    float far_plane = 100000.0;
    
    float w = log(max(near_plane, dist) / near_plane) / log(far_plane / near_plane);
    // Clamp w to avoid sampling artifacts at boundaries
    w = clamp(w, 0.0, 1.0); // Maybe slight offset?
    
    // 3. Sample Volume LUT
    float4 fog_data = camera_volume_lut.sample(s, float3(in.uv, w));
    
    // Output: rgb = inscatter, a = transmittance
    // This shader is likely drawn as a fullscreen pass over the scene.
    // The blending/composition implies: 
    // Final = Scene * Transmittance + Inscatter
    // We return this data. The pipeline blending state should be configured for:
    // SrcAlpha, OneMinusSrcAlpha? No.
    // If we want Scene * T + S:
    // We can use pre-multiplied alpha blending if Scene is dst.
    // dst.rgb = dst.rgb * src.a + src.rgb
    // Metal Blend:
    // SourceRGB = One
    // DestRGB = SourceAlpha
    // Alpha...
    
    return fog_data;
}
    float view_dist = length(view_vec);
    float3 view_dir = view_vec / view_dist;

    // 2. Compute aerial perspective (simplified fog)
    // In a real implementation: Raymarch or sample 3D LUT at (camera) and (target) to get inscatter/extinction
    float altitude = length(uniforms.camera_pos) - uniforms.planet_radius;
    float optical_depth = view_dist * 0.00001; // Extremely simplified constant density
    float3 transmittance = exp(-optical_depth * float3(0.1, 0.2, 0.4)); // Blue tint

    float3 inscatter = (1.0 - transmittance) * float3(0.5, 0.6, 0.8) * uniforms.sun_intensity.x * 0.01;

    // This output creates a fog overlay. Usually blended with scene color.
    // For this pass, we might output (Inscatter, Transmittance.r) to blend.
    return float4(inscatter, transmittance.r);
}
