/**
 * =================================================================================================
 *                          VEHICLE SUSPENSION - COMPLETE IMPLEMENTATION
 * =================================================================================================
 */

#include "physics/vehicles/suspension.h"
#include <physics/physics.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Creation & Destruction
// ============================================================================

Suspension *suspension_create(RigidBody *body, PhysicsWorld *world, const SuspensionConfig *config) {
    if (!body || !world || !config) return NULL;
    
    Suspension *susp = (Suspension*)malloc(sizeof(Suspension));
    if (!susp) return NULL;
    
    memcpy(&susp->config, config, sizeof(SuspensionConfig));
    memset(&susp->state, 0, sizeof(SuspensionState));
    
    susp->attached_body = body;
    susp->world = world;
    susp->state.current_length = config->rest_length;
    
    return susp;
}

void suspension_destroy(Suspension *susp) {
    free(susp);
}

// ============================================================================
// Raycast Logic
// ============================================================================

static bool suspension_raycast(Suspension *susp) {
    if (!susp || !susp->world) return false;
    
    // Get body position and rotation
    Vec3 body_pos = rigid_body_get_position(susp->attached_body);
    Quat body_rot = rigid_body_get_rotation(susp->attached_body);
    
    // Transform wheel attachment point to world space
    Vec3 attach_local = vec3(susp->config.wheel_attach_local[0], 
                             susp->config.wheel_attach_local[1], 
                             susp->config.wheel_attach_local[2]);
    
    // Rotate local point by body rotation
    // v' = q * v * q_inverse
    // Or closer to implementation: use a helper or manual calculation
    // manual quat * vec3 rotation:
    float qx = body_rot.x, qy = body_rot.y, qz = body_rot.z, qw = body_rot.w;
    float x = attach_local.x, y = attach_local.y, z = attach_local.z;
    
    // Calculate quat * vec
    float ix = qw * x + qy * z - qz * y;
    float iy = qw * y + qz * x - qx * z;
    float iz = qw * z + qx * y - qy * x;
    float iw = -qx * x - qy * y - qz * z;
    
    // Calculate result * inverse quat
    float dest_x = ix * qw + iw * -qx + iy * -qz - iz * -qy;
    float dest_y = iy * qw + iw * -qy + iz * -qx - ix * -qz;
    float dest_z = iz * qw + iw * -qz + ix * -qy - iy * -qx;
    
    float ray_origin[3] = {
        body_pos.x + dest_x,
        body_pos.y + dest_y,
        body_pos.z + dest_z
    };
    
    // Also rotate ray direction
    Vec3 dir_local = vec3(susp->config.ray_direction[0],
                          susp->config.ray_direction[1],
                          susp->config.ray_direction[2]);
                          
    x = dir_local.x; y = dir_local.y; z = dir_local.z;
    ix = qw * x + qy * z - qz * y;
    iy = qw * y + qz * x - qx * z;
    iz = qw * z + qx * y - qy * x;
    iw = -qx * x - qy * y - qz * z;
    
    float rot_dir_x = ix * qw + iw * -qx + iy * -qz - iz * -qy;
    float rot_dir_y = iy * qw + iw * -qy + iz * -qx - ix * -qz;
    float rot_dir_z = iz * qw + iw * -qz + ix * -qy - iy * -qx;

    float ray_dir[3] = { rot_dir_x, rot_dir_y, rot_dir_z };
    
    // Ray length = rest length + max extension
    float ray_length = susp->config.rest_length + susp->config.max_extension;
    
    // Perform raycast
    RaycastHit hit;
    bool hit_result = physics_world_raycast(susp->world,
                                           vec3(ray_origin[0], ray_origin[1], ray_origin[2]),
                                           vec3(ray_dir[0], ray_dir[1], ray_dir[2]),
                                           ray_length,
                                           &hit);
    
    if (hit_result && hit.distance <= ray_length) {
        susp->state.in_contact = true;
        susp->state.contact_point[0] = hit.point.x;
        susp->state.contact_point[1] = hit.point.y;
        susp->state.contact_point[2] = hit.point.z;
        susp->state.contact_normal[0] = hit.normal.x;
        susp->state.contact_normal[1] = hit.normal.y;
        susp->state.contact_normal[2] = hit.normal.z;
        susp->state.current_length = hit.distance;
        return true;
    }
    
    susp->state.in_contact = false;
    susp->state.current_length = ray_length;
    return false;
}

// ============================================================================
// Spring-Damper Compression
// ============================================================================

static float suspension_calculate_spring_force(const Suspension *susp) {
    if (!susp->state.in_contact) return 0.0f;
    
    // Compression = how much shorter than rest length
    float compression = susp->config.rest_length - susp->state.current_length;
    
    // Clamp to limits
    if (compression < -susp->config.max_extension) {
        compression = -susp->config.max_extension;
    }
    if (compression > susp->config.max_compression) {
        compression = susp->config.max_compression;
    }
    
    // Spring force: F = k * x
    float spring_force = susp->config.spring_stiffness * compression;
    
    // Damping force: F = c * v
    float damping_force = susp->config.damping_coefficient * susp->state.compression_velocity;
    
    // Total force (always upward when compressed)
    float total_force = spring_force - damping_force;
    
    // Hard stop at max compression (exponential increase)
    if (compression >= susp->config.max_compression * 0.9f) {
        float over_compression = compression - (susp->config.max_compression * 0.9f);
        float bump_stop_force = over_compression * susp->config.spring_stiffness * 10.0f;
        total_force += bump_stop_force;
    }
    
    return fmaxf(total_force, 0.0f); // Never pull down
}

// ============================================================================
// Anti-Squat / Anti-Dive Geometry
// ============================================================================

static void suspension_apply_anti_squat(Suspension *susp, float force) {
    if (!susp->attached_body) return;
    
    // Get body acceleration
    Vec3 acceleration = rigid_body_get_linear_acceleration(susp->attached_body);
    
    // Longitudinal acceleration (forward/backward)
    float accel_x = acceleration.x;
    
    // Anti-squat counters weight transfer during acceleration
    if (accel_x > 0.1f) {
        // Accelerating forward -> prevent squat
        float anti_squat_force = accel_x * force * susp->config.anti_squat_factor;
        // Apply additional upward force at this wheel
        susp->state.force_magnitude += anti_squat_force;
    }
    
    // Anti-dive counters weight transfer during braking
    if (accel_x < -0.1f) {
        // Decelerating -> prevent dive
        float anti_dive_force = -accel_x * force * susp->config.anti_dive_factor;
        susp->state.force_magnitude += anti_dive_force;
    }
}

// ============================================================================
// Camber Change Handling
// ============================================================================

static void suspension_update_camber(Suspension *susp) {
    // Camber changes with compression
    float compression_ratio = susp->state.compression;
    susp->state.current_camber = susp->config.camber_angle +
                                (compression_ratio * susp->config.camber_rate);
}

// ============================================================================
// Main Update
// ============================================================================

void suspension_update(Suspension *susp, float dt) {
    if (!susp) return;
    
    // Store previous length for velocity calculation
    float prev_length = susp->state.current_length;
    
    // Raycast to find ground
    suspension_raycast(susp);
    
    // Calculate compression velocity
    susp->state.compression_velocity = (susp->state.current_length - prev_length) / fmaxf(dt, 0.001f);
    
    // Calculate compression ratio [0, 1]
    if (susp->state.in_contact) {
        float compression = susp->config.rest_length - susp->state.current_length;
        susp->state.compression = compression / susp->config.max_compression;
        susp->state.compression = fmaxf(0.0f, fminf(1.0f, susp->state.compression));
    } else {
        susp->state.compression = 0.0f;
    }
    
    // Calculate spring force
    susp->state.force_magnitude = suspension_calculate_spring_force(susp);
    
    // Apply anti-squat/dive
    if (susp->state.in_contact && susp->state.force_magnitude > 0.0f) {
        suspension_apply_anti_squat(susp, susp->state.force_magnitude);
    }
    
    // Update camber
    suspension_update_camber(susp);
}

// ============================================================================
// Force Application
// ============================================================================

void suspension_apply_forces(Suspension *susp) {
    if (!susp || !susp->attached_body || !susp->state.in_contact) return;
    if (susp->state.force_magnitude <= 0.0f) return;
    
    // Apply force at wheel contact point
    Vec3 force_vector = vec3(
        susp->state.contact_normal[0] * susp->state.force_magnitude,
        susp->state.contact_normal[1] * susp->state.force_magnitude,
        susp->state.contact_normal[2] * susp->state.force_magnitude
    );
    
    Vec3 application_point = vec3(
        susp->state.contact_point[0],
        susp->state.contact_point[1],
        susp->state.contact_point[2]
    );
    
    rigid_body_apply_force_at_point(susp->attached_body, force_vector, application_point);
}

// ============================================================================
// Queries
// ============================================================================

float suspension_get_compression(const Suspension *susp) {
    return susp ? susp->state.compression : 0.0f;
}

bool suspension_is_grounded(const Suspension *susp) {
    return susp ? susp->state.in_contact : false;
}

float suspension_get_force(const Suspension *susp) {
    return susp ? susp->state.force_magnitude : 0.0f;
}
