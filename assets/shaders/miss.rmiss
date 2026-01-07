#version 460
#extension GL_EXT_ray_tracing : require

// Miss shader - handles rays that don't hit any geometry (sky/background)
layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT ShadowPayload shadow_payload;

// Descriptor bindings
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

layout(set = 0, binding = 8) uniform samplerCube skybox;
layout(set = 0, binding = 9) uniform sampler2D atmosphere_lut;

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

// Atmospheric scattering constants
const float EARTH_RADIUS = 6371.0;
const float ATMOSPHERE_RADIUS = 6471.0;
const vec3 RAYLEIGH_SCATTERING = vec3(5.8e-6, 13.5e-6, 33.1e-6);
const vec3 MIE_SCATTERING = vec3(2.0e-5);
const float SCALE_HEIGHT_RAYLEIGH = 8.0;
const float SCALE_HEIGHT_MIE = 1.2;
const float MIE_G = 0.758;

// Calculate atmospheric scattering
vec3 calculate_atmospheric_scattering(vec3 ray_dir, vec3 sun_dir) {
    float height_above_ground = camera.position.y;
    
    // Simple approximation for Minecraft-style sky
    float sun_dot = max(dot(ray_dir, sun_dir), 0.0);
    
    // Rayleigh scattering (blue sky)
    vec3 rayleigh = RAYLEIGH_SCATTERING * 1e6;
    
    // Mie scattering (haze and sun disc)
    float mie_phase = (1.0 - MIE_G * MIE_G) / (4.0 * PI * pow(1.0 + MIE_G * MIE_G - 2.0 * MIE_G * sun_dot, 1.5));
    vec3 mie = MIE_SCATTERING * mie_phase * 1e4;
    
    // Combine scattering
    vec3 scattering = rayleigh + mie;
    
    // Add sun disc
    float sun_angle = acos(sun_dot);
    float sun_disc = smoothstep(0.02, 0.0, sun_angle);
    vec3 sun_color = vec3(1.0, 0.95, 0.8) * 5.0;
    scattering += sun_color * sun_disc;
    
    return scattering;
}

// Calculate procedural sky gradient
vec3 calculate_sky_gradient(vec3 ray_dir, float time_of_day) {
    // Time-based sky colors
    vec3 day_horizon = vec3(0.7, 0.8, 1.0);
    vec3 day_zenith = vec3(0.3, 0.5, 0.9);
    vec3 sunset_horizon = vec3(1.0, 0.6, 0.3);
    vec3 sunset_zenith = vec3(0.4, 0.2, 0.6);
    vec3 night_horizon = vec3(0.1, 0.1, 0.2);
    vec3 night_zenith = vec3(0.0, 0.0, 0.05);
    
    // Interpolate based on time of day (0-1, where 0.5 is noon)
    float t = clamp(time_of_day, 0.0, 1.0);
    
    // Smooth transitions
    float day_factor = smoothstep(0.2, 0.8, t) * (1.0 - smoothstep(0.8, 0.9, t));
    float sunset_factor = smoothstep(0.1, 0.3, t) * (1.0 - smoothstep(0.3, 0.4, t)) +
                         smoothstep(0.6, 0.8, t) * (1.0 - smoothstep(0.8, 0.9, t));
    float night_factor = smoothstep(0.9, 1.0, t) + (1.0 - smoothstep(0.0, 0.1, t));
    
    vec3 horizon = mix(night_horizon, day_horizon, day_factor);
    horizon = mix(horizon, sunset_horizon, sunset_factor);
    
    vec3 zenith = mix(night_zenith, day_zenith, day_factor);
    zenith = mix(zenith, sunset_zenith, sunset_factor);
    
    // Height-based gradient
    float height = clamp(ray_dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sky_color = mix(horizon, zenith, height);
    
    return sky_color;
}

// Calculate stars for night sky
vec3 calculate_stars(vec3 ray_dir, float time_of_day) {
    // Only show stars at night
    float night_factor = smoothstep(0.3, 0.1, time_of_day) * (1.0 - smoothstep(0.9, 1.0, time_of_day));
    
    if (night_factor <= 0.0) {
        return vec3(0.0);
    }
    
    // Simple star field using pseudo-random function
    vec3 star_dir = normalize(ray_dir);
    float star_seed = fract(sin(dot(star_dir.xy, vec2(12.9898, 78.233))) * 43758.5453);
    
    // Create star brightness
    float star_brightness = step(0.995, star_seed) * 2.0;
    
    // Add twinkling effect
    float twinkle = sin(camera.time * 3.0 + star_seed * 10.0) * 0.5 + 0.5;
    star_brightness *= mix(0.3, 1.0, twinkle);
    
    return vec3(star_brightness) * night_factor;
}

// Main miss shader
void main() {
    vec3 ray_dir = normalize(gl_WorldRayDirectionEXT);
    
    // Calculate time of day from camera time (0-1, where 0.5 is noon)
    float time_of_day = fract(camera.time / 24000.0); // Minecraft day is 24000 ticks
    
    // Sun direction (simplified)
    float sun_angle = time_of_day * 2.0 * PI - PI * 0.5;
    vec3 sun_dir = normalize(vec3(cos(sun_angle), sin(sun_angle), 0.0));
    
    // Calculate sky color
    vec3 sky_color = calculate_sky_gradient(ray_dir, time_of_day);
    
    // Add atmospheric scattering
    vec3 scattering = calculate_atmospheric_scattering(ray_dir, sun_dir);
    sky_color += scattering * 0.1;
    
    // Add stars at night
    vec3 stars = calculate_stars(ray_dir, time_of_day);
    sky_color += stars;
    
    // Sample skybox if available
    if (textureSize(skybox, 0).x > 0) {
        vec3 skybox_sample = texture(skybox, ray_dir).rgb;
        sky_color = mix(sky_color, skybox_sample, 0.5);
    }
    
    // Set payload for primary rays
    if (gl_LaunchIDEXT.z == 0) { // Primary ray payload
        payload.radiance = sky_color;
        payload.t = -1.0; // No hit
        payload.hit_id = 0xFFFFFFFF; // Invalid hit ID
        payload.material_id = 0xFFFFFFFF;
    } else { // Shadow payload
        shadow_payload.visibility = 1.0; // No shadow
    }
}
