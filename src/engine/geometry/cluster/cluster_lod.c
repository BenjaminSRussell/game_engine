#include <simd/simd.h>
#include <include/math/math.h>

// Compute screen space error for a cluster
// projected_error = (lod_error * sphere_radius) / distance_to_camera * screen_height / fov_factor
// Simplified: error / dist

float compute_screen_error(float lod_error, simd_float3 bounds_center, float bounds_radius, simd_float3 view_pos, float screen_height, float fov) {
    float d = simd_distance(bounds_center, view_pos);
    // Avoid division by zero
    d = fmaxf(d, 0.001f);
    
    // Project error to screen pixels
    // simple projection: error_pixels = (error_world / dist) * (screen_height / (2 * tan(fov/2)))
    // Assuming lod_error is in object space units? Or screen space?
    // User comment says: "float lod_error; // Screen-space error threshold" - This implies it's a THRESHOLD.
    // AND "float screen_error = compute_screen_error(cluster.lod_error, ...)"
    // Typically `cluster.error` is the Max Deviation in object space.
    // We project that deviation to screen space and compare with a threshold (e.g. 1.0 pixel).
    
    // Let's assume lod_error in struct is Object Space Error.
    float cot_half_fov = 1.0f / tanf(fov * 0.5f);
    float projected_error = (lod_error * screen_height * cot_half_fov * 0.5f) / d;
    return projected_error;
}
