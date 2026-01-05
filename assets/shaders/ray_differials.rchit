#version 460
#extension GL_EXT_ray_tracing : require

// Ray differentials shader - calculates texture filtering differentials
// Used for proper mipmapping and anisotropic filtering in raytracing

layout(location = 0) rayPayloadInEXT RayPayload payload;

// Ray differential structure
struct RayDifferential {
    vec3 origin_dx;    // Partial derivative of ray origin with respect to x
    vec3 origin_dy;    // Partial derivative of ray origin with respect to y
    vec3 direction_dx; // Partial derivative of ray direction with respect to x
    vec3 direction_dy; // Partial derivative of ray direction with respect to y
};

// Camera data
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

// Screen data
layout(set = 0, binding = 10) uniform ScreenData {
    vec2 resolution;
    vec2 pixel_size;
    float screen_width;
    float screen_height;
} screen;

// Calculate ray differentials for texture filtering
RayDifferential calculate_ray_differentials(vec3 ray_origin, vec3 ray_direction, vec2 pixel_coord) {
    RayDifferential diff;
    
    // Calculate pixel size in world space
    float pixel_size_x = screen.pixel_size.x;
    float pixel_size_y = screen.pixel_size.y;
    
    // Calculate camera basis vectors
    vec3 right = normalize(cross(camera.direction, camera.up));
    vec3 camera_up = cross(right, camera.direction);
    
    // Calculate tangent vectors for ray direction changes
    float tan_half_fov = tan(camera.fov * 0.5);
    
    // Partial derivative of ray direction with respect to screen x
    vec3 direction_dx = right * tan_half_fov * camera.aspect * pixel_size_x;
    
    // Partial derivative of ray direction with respect to screen y  
    vec3 direction_dy = camera_up * tan_half_fov * pixel_size_y;
    
    // Partial derivative of ray origin (for perspective projection)
    // In perspective, the origin is constant (camera position)
    diff.origin_dx = vec3(0.0);
    diff.origin_dy = vec3(0.0);
    
    // For orthographic projection, origin would vary with screen position
    // But for ray tracing, we typically use perspective
    
    diff.direction_dx = direction_dx;
    diff.direction_dy = direction_dy;
    
    return diff;
}

// Calculate texture coordinate differentials at hit point
void calculate_texture_differentials(vec3 hit_point, vec3 hit_normal, vec3 ray_direction,
                                     RayDifferential ray_diff, out vec2 ddx, out vec2 ddy) {
    // Calculate distance to hit point
    float t = length(hit_point - camera.position);
    
    // Calculate how much the ray direction changes across pixels
    vec3 direction_change_x = ray_diff.direction_dx * t;
    vec3 direction_change_y = ray_diff.direction_dy * t;
    
    // Project these changes onto the surface tangent plane
    vec3 surface_tangent_x = direction_change_x - dot(direction_change_x, hit_normal) * hit_normal;
    vec3 surface_tangent_y = direction_change_y - dot(direction_change_y, hit_normal) * hit_normal;
    
    // Calculate texture coordinate differentials
    // This depends on the UV mapping of the surface
    // For a simple planar mapping:
    ddx = surface_tangent_x.xy;
    ddy = surface_tangent_y.xy;
    
    // For spherical mapping (like on spheres):
    if (abs(hit_normal.y) > 0.9) { // Top/bottom of sphere
        ddx = surface_tangent_x.xz;
        ddy = surface_tangent_y.xz;
    } else if (abs(hit_normal.x) > 0.9) { // Sides of sphere
        ddx = surface_tangent_x.yz;
        ddy = surface_tangent_y.yz;
    } else { // Front/back of sphere
        ddx = surface_tangent_x.xy;
        ddy = surface_tangent_y.xy;
    }
    
    // Scale by texture resolution if known
    // This would typically be passed in as uniform data
    float texture_scale = 1.0;
    ddx *= texture_scale;
    ddy *= texture_scale;
}

// Calculate level of detail (LOD) for mipmapping
float calculate_lod_level(vec2 ddx, vec2 ddy) {
    // Calculate the maximum change in texture coordinates
    float max_delta = max(length(ddx), length(ddy));
    
    // Convert to mip level (log2 based)
    float lod = log2(max_delta);
    
    // Clamp to reasonable range
    return clamp(lod, 0.0, 8.0);
}

// Calculate anisotropic filtering direction and ratio
void calculate_anisotropic_filtering(vec2 ddx, vec2 ddy, out vec2 aniso_dir, out float aniso_ratio) {
    // Calculate the major and minor axes of the texture coordinate change
    float len_x = length(ddx);
    float len_y = length(ddy);
    
    if (len_x > len_y) {
        aniso_dir = normalize(ddx);
        aniso_ratio = len_x / max(len_y, 0.001);
    } else {
        aniso_dir = normalize(ddy);
        aniso_ratio = len_y / max(len_x, 0.001);
    }
    
    // Clamp anisotropic ratio to hardware limits
    aniso_ratio = min(aniso_ratio, 16.0); // Typical max anisotropy
}

// Main closest hit shader with ray differentials
void main() {
    // Get hit information from built-in variables
    vec3 world_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 world_normal = normalize(gl_WorldRayDirectionEXT);
    
    // Calculate pixel coordinate from launch ID
    vec2 pixel_coord = vec2(gl_LaunchIDEXT.xy) + 0.5;
    
    // Calculate ray differentials
    RayDifferential ray_diff = calculate_ray_differentials(camera.position, gl_WorldRayDirectionEXT, pixel_coord);
    
    // Calculate texture coordinate differentials
    vec2 ddx, ddy;
    calculate_texture_differentials(world_pos, world_normal, gl_WorldRayDirectionEXT, ray_diff, ddx, ddy);
    
    // Calculate LOD level
    float lod_level = calculate_lod_level(ddx, ddy);
    
    // Calculate anisotropic filtering parameters
    vec2 aniso_dir;
    float aniso_ratio;
    calculate_anisotropic_filtering(ddx, ddy, aniso_dir, aniso_ratio);
    
    // Store differentials in payload for use in material evaluation
    payload.radiance = vec3(lod_level, aniso_ratio, length(ddx) + length(ddy));
    
    // Continue with normal shading
    // The actual material sampling would use these differentials
}
