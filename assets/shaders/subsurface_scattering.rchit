#version 460
#extension GL_EXT_ray_tracing : require

// Subsurface scattering shader - simulates light transport through translucent materials
// Used for materials like skin, wax, ice, leaves, and some Minecraft blocks

layout(location = 0) rayPayloadInEXT RayPayload payload;

// Material properties for subsurface scattering
layout(binding = 5, set = 0, std430) readonly buffer MaterialBuffer {
    struct Material {
        vec3 albedo;
        vec3 normal;
        float roughness;
        float metalness;
        float subsurface_strength;  // Strength of subsurface scattering
        float scatter_distance;    // Average scatter distance
        vec3 scatter_color;        // Color of scattered light
        float absorption_distance; // Average absorption distance
        vec3 absorption_color;    // Color of absorbed light
        uint material_type;
        uint texture_id;
    } materials[];
};

// Subsurface scattering parameters
layout(binding = 6, set = 0) uniform SubsurfaceParams {
    float max_scatter_distance;
    u32 scatter_samples;
    bool enable_approximation;
    bool enable_directional_scattering;
    float phase_function_g;       // Henyey-Greenstein phase function parameter
    bool enable_color_bleeding;
} subsurface_params;

// Random number generator
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

// Generate random direction in hemisphere
vec3 random_hemisphere_direction(vec3 normal, inout uint seed) {
    // Generate random point on unit sphere
    float u1 = rand_float(seed);
    float u2 = rand_float(seed);
    float r = sqrt(u1);
    float theta = 2.0 * 3.14159265 * u2;
    
    vec3 random_dir = vec3(
        r * cos(theta),
        r * sin(theta),
        sqrt(1.0 - u1)
    );
    
    // Align with normal
    vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
    if (length(tangent) < 0.001) {
        tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
    }
    vec3 bitangent = cross(normal, tangent);
    
    return tangent * random_dir.x + bitangent * random_dir.y + normal * random_dir.z;
}

// Henyey-Greenstein phase function for scattering
float henyey_greenstein_phase(float cos_theta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cos_theta, 1.5));
}

// Calculate subsurface scattering contribution
vec3 calculate_subsurface_scattering(vec3 hit_point, vec3 hit_normal, vec3 view_dir, 
                                   vec3 light_dir, Material material, uint material_id) {
    if (material.subsurface_strength <= 0.0) {
        return vec3(0.0);
    }
    
    vec3 total_scatter = vec3(0.0);
    uint rng_state = uint(gl_LaunchIDEXT.x + gl_LaunchIDEXT.y * 1000) + material_id;
    
    // Sample multiple scatter directions
    for (uint i = 0; i < subsurface_params.scatter_samples; ++i) {
        // Generate scatter direction
        vec3 scatter_dir = random_hemisphere_direction(hit_normal, rng_state);
        
        // Calculate phase function weight
        float cos_theta = dot(scatter_dir, -light_dir);
        float phase_weight = henyey_greenstein_phase(cos_theta, subsurface_params.phase_function_g);
        
        // Calculate distance-based attenuation
        float scatter_distance = material.scatter_distance;
        float distance_attenuation = exp(-scatter_distance / material.scatter_distance);
        
        // Calculate absorption
        float absorption = exp(-scatter_distance / material.absorption_distance);
        
        // Calculate scatter contribution
        vec3 scatter_color = material.scatter_color;
        vec3 absorption_color = material.absorption_color;
        
        // Apply color bleeding if enabled
        if (subsurface_params.enable_color_bleeding) {
            // Mix scatter color with material albedo
            scatter_color = mix(scatter_color, material.albedo, 0.3);
        }
        
        // Combine all factors
        vec3 scatter_contribution = scatter_color * phase_weight * distance_attenuation * absorption;
        scatter_contribution *= (1.0 - absorption_color); // Remove absorbed wavelengths
        
        // Apply material strength
        scatter_contribution *= material.subsurface_strength;
        
        total_scatter += scatter_contribution;
    }
    
    // Normalize by number of samples
    total_scatter /= float(subsurface_params.scatter_samples);
    
    // Apply directional scattering if enabled
    if (subspace_params.enable_directional_scattering) {
        // Add directional component based on view angle
        float view_angle = dot(view_dir, hit_normal);
        float directional_weight = max(view_angle, 0.0) * 0.5;
        total_scatter *= (1.0 + directional_weight);
    }
    
    return total_scatter;
}

// Approximate subsurface scattering using screen-space techniques
vec3 approximate_subsurface_scattering(vec3 hit_point, vec3 hit_normal, vec3 view_dir, 
                                        vec3 light_dir, Material material) {
    if (material.subsurface_strength <= 0.0) {
        return vec3(0.0);
    }
    
    // Simple approximation based on distance from surface
    float distance_factor = 1.0 - clamp(length(hit_point) / subsurface_params.max_scatter_distance, 0.0, 1.0);
    
    // Calculate scattering based on angle between normal and light
    float light_angle = max(dot(hit_normal, light_dir), 0.0);
    
    // Combine factors
    vec3 scatter_color = material.scatter_color;
    float scatter_strength = material.subsurface_strength * distance_factor * light_angle;
    
    return scatter_color * scatter_strength;
}

// Main subsurface scattering function
void main() {
    // Get hit information
    vec3 hit_point = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 hit_normal = normalize(gl_WorldRayDirectionEXT);
    vec3 view_dir = normalize(-gl_WorldRayDirectionEXT);
    
    // Get material ID from custom data
    uint material_id = gl_PrimitiveID;
    if (material_id >= materials.length()) {
        return;
    }
    
    Material material = materials[material_id];
    
    // Get light direction (simplified - would normally come from light buffer)
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    
    // Calculate subsurface scattering
    vec3 scatter_contribution = vec3(0.0);
    
    if (subsurface_params.enable_approximation) {
        scatter_contribution = approximate_subsurface_scattering(hit_point, hit_normal, view_dir, light_dir, material);
    } else {
        scatter_contribution = calculate_subsurface_scattering(hit_point, hit_normal, view_dir, light_dir, material, material_id);
    }
    
    // Add to existing payload radiance
    payload.radiance += scatter_contribution;
}
