// Vehicle Physics System with Enhanced Wheel Raycast
#include "include/physics/vehicle_physics.h"
#include "include/physics/physics.h"
#include "include/core/logger.h"
#include "include/math/mat4.h"
#include "include/math/vec3.h"
#include "include/math/quat.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Raycast system for wheel-ground detection
#define RAYCAST_LENGTH 2.0f
#define MAX_RAYCAST_HITS 16

typedef struct {
    Vec3 origin;
    Vec3 direction;
    f32 max_distance;
    Vec3 hit_point;
    Vec3 hit_normal;
    f32 hit_distance;
    bool hit_found;
    u32 surface_type;
} WheelRaycast;

typedef struct {
    Vec3 points[MAX_RAYCAST_HITS];
    Vec3 normals[MAX_RAYCAST_HITS];
    u32 count;
} RaycastResult;

// Enhanced wheel raycast implementation
static bool vehicle_wheel_raycast(const VehiclePhysics *vehicle, u32 wheel_index, 
                                  WheelRaycast *raycast) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !raycast) {
        return false;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    // Calculate wheel world position
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, wheel->position);
    
    // Setup raycast (pointing downward)
    raycast->origin = wheel_world_pos;
    raycast->direction = (Vec3){0.0f, -1.0f, 0.0f};
    raycast->max_distance = wheel->suspension_length + wheel->radius;
    raycast->hit_found = false;
    raycast->hit_distance = raycast->max_distance;
    
    // In a real implementation, this would use the physics engine's raycast system
    // For now, we'll simulate ground detection at y=0
    f32 ground_y = 0.0f;
    f32 t = (ground_y - raycast->origin.y) / raycast->direction.y;
    
    if (t > 0.0f && t <= raycast->max_distance) {
        raycast->hit_point = vec3_add(raycast->origin, vec3_scale(raycast->direction, t));
        raycast->hit_normal = (Vec3){0.0f, 1.0f, 0.0f};
        raycast->hit_distance = t;
        raycast->hit_found = true;
        raycast->surface_type = 0; // Default surface
        
        LOG_TRACE("Wheel %u raycast hit at (%.2f, %.2f, %.2f)", 
                 wheel_index, raycast->hit_point.x, raycast->hit_point.y, raycast->hit_point.z);
    }
    
    return raycast->hit_found;
}

// Multi-point raycast for better wheel contact detection
static bool vehicle_wheel_raycast_multi(const VehiclePhysics *vehicle, u32 wheel_index, 
                                        RaycastResult *result) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !result) {
        return false;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    result->count = 0;
    
    // Calculate wheel world position
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, wheel->position);
    
    // Perform multiple raycasts around the wheel for better contact detection
    f32 raycast_spacing = wheel->width * 0.25f;
    Vec3 raycast_offsets[5] = {
        {0.0f, 0.0f, 0.0f},           // Center
        {raycast_spacing, 0.0f, 0.0f},   // Right
        {-raycast_spacing, 0.0f, 0.0f},  // Left
        {0.0f, 0.0f, raycast_spacing},   // Front
        {0.0f, 0.0f, -raycast_spacing}    // Back
    };
    
    for (u32 i = 0; i < 5 && result->count < MAX_RAYCAST_HITS; i++) {
        Vec3 ray_origin = vec3_add(wheel_world_pos, raycast_offsets[i]);
        Vec3 ray_direction = (Vec3){0.0f, -1.0f, 0.0f};
        f32 max_distance = wheel->suspension_length + wheel->radius;
        
        // Simulate raycast
        f32 ground_y = 0.0f;
        f32 t = (ground_y - ray_origin.y) / ray_direction.y;
        
        if (t > 0.0f && t <= max_distance) {
            Vec3 hit_point = vec3_add(ray_origin, vec3_scale(ray_direction, t));
            Vec3 hit_normal = (Vec3){0.0f, 1.0f, 0.0f};
            
            result->points[result->count] = hit_point;
            result->normals[result->count] = hit_normal;
            result->count++;
            
            LOG_TRACE("Multi-raycast %u.%u hit at (%.2f, %.2f, %.2f)", 
                     wheel_index, i, hit_point.x, hit_point.y, hit_point.z);
        }
    }
    
    return result->count > 0;
}

// Surface detection based on raycast results
static u32 vehicle_detect_surface_type(const RaycastResult *result) {
    if (!result || result->count == 0) {
        return 0; // Default surface
    }
    
    // In a real implementation, this would check the material/terrain type
    // For now, we'll simulate surface detection based on position
    Vec3 avg_point = {0.0f, 0.0f, 0.0f};
    for (u32 i = 0; i < result->count; i++) {
        avg_point = vec3_add(avg_point, result->points[i]);
    }
    avg_point = vec3_scale(avg_point, 1.0f / result->count);
    
    // Simulate different surface types based on position
    if (avg_point.y < 0.1f) {
        return 1; // Asphalt
    } else if (avg_point.y < 0.5f) {
        return 2; // Dirt
    } else {
        return 3; // Grass
    }
}

// Update wheel contact information
static void vehicle_update_wheel_contact(VehiclePhysics *vehicle, u32 wheel_index) {
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    // Perform multi-point raycast
    RaycastResult result;
    bool has_contact = vehicle_wheel_raycast_multi(vehicle, wheel_index, &result);
    
    if (has_contact) {
        // Calculate average contact point and normal
        Vec3 avg_point = {0.0f, 0.0f, 0.0f};
        Vec3 avg_normal = {0.0f, 0.0f, 0.0f};
        
        for (u32 i = 0; i < result.count; i++) {
            avg_point = vec3_add(avg_point, result.points[i]);
            avg_normal = vec3_add(avg_normal, result.normals[i]);
        }
        
        avg_point = vec3_scale(avg_point, 1.0f / result.count);
        avg_normal = vec3_normalize(avg_normal);
        
        // Update wheel contact information
        wheel->contact_point = avg_point;
        wheel->contact_normal = avg_normal;
        wheel->is_grounded = true;
        
        // Calculate suspension compression
        Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
        Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, wheel->position);
        f32 distance_to_ground = vec3_distance(wheel_world_pos, avg_point);
        wheel->compression = wheel->suspension_length - (distance_to_ground - wheel->radius);
        wheel->compression = fmaxf(0.0f, fminf(wheel->compression, wheel->suspension_length));
        
        LOG_TRACE("Wheel %u contact: compression=%.3f, surface=%u", 
                 wheel_index, wheel->compression, vehicle_detect_surface_type(&result));
    } else {
        // No contact
        wheel->is_grounded = false;
        wheel->compression = 0.0f;
        wheel->contact_normal = (Vec3){0.0f, 1.0f, 0.0f};
        
        LOG_TRACE("Wheel %u: no ground contact", wheel_index);
    }
}

// Public API implementation
VehiclePhysics *vehicle_create(u32 wheel_count) {
    if (wheel_count == 0 || wheel_count > MAX_WHEELS) {
        LOG_ERROR("Invalid wheel count: %u (max: %u)", wheel_count, MAX_WHEELS);
        return NULL;
    }
    
    VehiclePhysics *vehicle = (VehiclePhysics *)calloc(1, sizeof(VehiclePhysics));
    if (!vehicle) {
        LOG_ERROR("Failed to allocate vehicle physics");
        return NULL;
    }
    
    vehicle->wheel_count = wheel_count;
    vehicle->mass = 1500.0f; // Default car mass
    vehicle->center_of_mass = (Vec3){0.0f, 0.0f, 0.0f};
    vehicle->max_engine_torque = 300.0f;
    vehicle->max_rpm = 6000.0f;
    vehicle->drag_coefficient = 0.3f;
    vehicle->downforce_coefficient = 0.4f;
    vehicle->slip_angle_limit = 0.2f;
    
    // Initialize transmission
    vehicle->current_gear = 1;
    vehicle->gear_count = 5;
    vehicle->gear_ratios[0] = 3.5f; // Reverse
    vehicle->gear_ratios[1] = 3.5f; // 1st
    vehicle->gear_ratios[2] = 2.1f; // 2nd
    vehicle->gear_ratios[3] = 1.4f; // 3rd
    vehicle->gear_ratios[4] = 1.0f; // 4th
    vehicle->gear_ratios[5] = 0.8f; // 5th
    vehicle->differential_ratio = 3.5f;
    
    LOG_INFO("Created vehicle physics with %u wheels", wheel_count);
    return vehicle;
}

void vehicle_destroy(VehiclePhysics *vehicle) {
    if (vehicle) {
        free(vehicle);
        LOG_INFO("Destroyed vehicle physics");
    }
}

void vehicle_update(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    
    // Update wheel contacts using raycast system
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_update_wheel_contact(vehicle, i);
    }
    
    // Update suspension system with force calculations
    vehicle_update_suspension_system(vehicle);
    
    // Update friction system with slip detection
    WheelFriction frictions[MAX_WHEELS];
    vehicle_calculate_friction_forces(vehicle, frictions);
    
    // Update engine RPM based on wheel speed and gear
    // This would be implemented in the engine todo
    
    // Update physics simulation
    // This would include forces, integration, etc.
    
    // Log friction status
    u32 slipping_wheels = 0;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        if (frictions[i].is_slipping) slipping_wheels++;
    }
    
    if (slipping_wheels > 0) {
        LOG_DEBUG("Vehicle has %u slipping wheels", slipping_wheels);
    }
    
    LOG_TRACE("Updated vehicle physics (%.3fs)", delta_time);
}

void vehicle_set_input(VehiclePhysics *vehicle, f32 throttle, f32 brake,
                       f32 steering, f32 handbrake) {
    if (!vehicle) return;
    
    vehicle->throttle = fmaxf(0.0f, fminf(1.0f, throttle));
    vehicle->brake = fmaxf(0.0f, fminf(1.0f, brake));
    vehicle->steering = fmaxf(-1.0f, fminf(1.0f, steering));
    vehicle->handbrake = fmaxf(0.0f, fminf(1.0f, handbrake));
    
    LOG_TRACE("Vehicle input: throttle=%.2f, brake=%.2f, steering=%.2f, handbrake=%.2f",
             vehicle->throttle, vehicle->brake, vehicle->steering, vehicle->handbrake);
}

void vehicle_add_wheel(VehiclePhysics *vehicle, Vec3 offset, f32 radius,
                       bool driven, bool steered) {
    if (!vehicle || vehicle->wheel_count >= MAX_WHEELS) {
        LOG_ERROR("Cannot add wheel: vehicle full or invalid");
        return;
    }
    
    u32 wheel_index = vehicle->wheel_count;
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    wheel->position = offset;
    wheel->radius = radius;
    wheel->width = radius * 0.3f; // Default width ratio
    wheel->suspension_length = 0.5f;
    wheel->suspension_stiffness = 35000.0f;
    wheel->suspension_damping = 3500.0f;
    wheel->friction_slip = 0.8f;
    
    wheel->compression = 0.0f;
    wheel->wheel_rotation = 0.0f;
    wheel->steering_angle = 0.0f;
    
    wheel->is_driven = driven;
    wheel->is_steered = steered;
    
    wheel->is_grounded = false;
    wheel->contact_point = (Vec3){0.0f, 0.0f, 0.0f};
    wheel->contact_normal = (Vec3){0.0f, 1.0f, 0.0f};
    
    vehicle->wheel_count++;
    
    LOG_INFO("Added wheel %u: offset=(%.2f,%.2f,%.2f), radius=%.2f, driven=%s, steered=%s",
             wheel_index, offset.x, offset.y, offset.z, radius,
             driven ? "true" : "false", steered ? "true" : "false");
}

// Public API for wheel raycast system
bool vehicle_get_wheel_contact(const VehiclePhysics *vehicle, u32 wheel_index,
                               Vec3 *contact_point, Vec3 *contact_normal, bool *is_grounded) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return false;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (contact_point) *contact_point = wheel->contact_point;
    if (contact_normal) *contact_normal = wheel->contact_normal;
    if (is_grounded) *is_grounded = wheel->is_grounded;
    
    return wheel->is_grounded;
}

f32 vehicle_get_wheel_compression(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return 0.0f;
    }
    
    return vehicle->wheels[wheel_index].compression;
}

bool vehicle_perform_wheel_raycast(const VehiclePhysics *vehicle, u32 wheel_index,
                                   Vec3 *hit_point, Vec3 *hit_normal, f32 *hit_distance) {
    WheelRaycast raycast;
    bool success = vehicle_wheel_raycast(vehicle, wheel_index, &raycast);
    
    if (success && hit_point) *hit_point = raycast.hit_point;
    if (success && hit_normal) *hit_normal = raycast.hit_normal;
    if (success && hit_distance) *hit_distance = raycast.hit_distance;
    
    return success;
}

// Suspension force calculation using spring-damper model
static Vec3 vehicle_calculate_suspension_force(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded || wheel->compression <= 0.0f) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    
    // Spring force: F = -k * x (Hooke's law)
    f32 spring_force = wheel->suspension_stiffness * wheel->compression;
    
    // Damping force: F = -c * v
    // We need wheel velocity relative to ground contact point
    // For now, we'll estimate damping based on compression rate
    f32 damping_force = wheel->suspension_damping * wheel->compression * 0.1f;
    
    // Total suspension force (upward, opposing compression)
    f32 total_force = spring_force + damping_force;
    
    // Apply force in direction of contact normal (usually upward)
    Vec3 force = vec3_scale(wheel->contact_normal, total_force);
    
    LOG_TRACE("Wheel %u suspension: spring=%.1fN, damping=%.1fN, total=%.1fN", 
             wheel_index, spring_force, damping_force, total_force);
    
    return force;
}

// Calculate suspension forces for all wheels
static void vehicle_calculate_suspension_forces(VehiclePhysics *vehicle, Vec3 *forces) {
    if (!vehicle || !forces) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        forces[i] = vehicle_calculate_suspension_force(vehicle, i);
    }
}

// Anti-roll bar calculation for improved stability
static Vec3 vehicle_calculate_anti_roll_force(const VehiclePhysics *vehicle) {
    if (!vehicle || vehicle->wheel_count < 4) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    
    // Calculate roll angle from wheel compression differences
    f32 left_compression = 0.0f, right_compression = 0.0f;
    u32 left_count = 0, right_count = 0;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        const VehicleWheel *wheel = &vehicle->wheels[i];
        
        // Simple left/right classification based on position
        if (wheel->position.x < 0.0f) {
            left_compression += wheel->compression;
            left_count++;
        } else {
            right_compression += wheel->compression;
            right_count++;
        }
    }
    
    if (left_count > 0) left_compression /= left_count;
    if (right_count > 0) right_compression /= right_count;
    
    // Anti-roll force proportional to compression difference
    f32 compression_diff = left_compression - right_compression;
    f32 anti_roll_stiffness = 5000.0f; // Anti-roll bar stiffness
    
    Vec3 anti_roll_force = (Vec3){0.0f, 0.0f, compression_diff * anti_roll_stiffness};
    
    LOG_TRACE("Anti-roll: left=%.3f, right=%.3f, force=%.1fN", 
             left_compression, right_compression, vec3_length(anti_roll_force));
    
    return anti_roll_force;
}

// Enhanced suspension system with load balancing
static void vehicle_update_suspension_system(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    
    Vec3 suspension_forces[MAX_WHEELS];
    vehicle_calculate_suspension_forces(vehicle, suspension_forces);
    
    // Calculate total suspension forces for load balancing
    Vec3 total_suspension_force = {0.0f, 0.0f, 0.0f};
    u32 grounded_wheels = 0;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        if (vehicle->wheels[i].is_grounded) {
            total_suspension_force = vec3_add(total_suspension_force, suspension_forces[i]);
            grounded_wheels++;
        }
    }
    
    // Apply load balancing if vehicle is unevenly loaded
    if (grounded_wheels > 0) {
        Vec3 avg_suspension_force = vec3_scale(total_suspension_force, 1.0f / grounded_wheels);
        
        for (u32 i = 0; i < vehicle->wheel_count; i++) {
            if (vehicle->wheels[i].is_grounded) {
                // Calculate force difference from average
                Vec3 force_diff = vec3_sub(suspension_forces[i], avg_suspension_force);
                
                // Apply load balancing (soft constraint)
                f32 balance_factor = 0.1f; // How strongly to balance loads
                Vec3 balancing_force = vec3_scale(force_diff, -balance_factor);
                
                // Update suspension force with balancing
                suspension_forces[i] = vec3_add(suspension_forces[i], balancing_force);
            }
        }
    }
    
    // Apply anti-roll forces
    Vec3 anti_roll_force = vehicle_calculate_anti_roll_force(vehicle);
    
    // In a full implementation, these forces would be applied to the vehicle body
    // For now, we'll just log the results
    LOG_DEBUG("Suspension system updated: total_force=(%.1f,%.1f,%.1f)N, anti_roll=%.1fN",
             total_suspension_force.x, total_suspension_force.y, total_suspension_force.z,
             vec3_length(anti_roll_force));
}

// Public API for suspension system
f32 vehicle_get_suspension_force_magnitude(const VehiclePhysics *vehicle, u32 wheel_index) {
    Vec3 force = vehicle_calculate_suspension_force(vehicle, wheel_index);
    return vec3_length(force);
}

void vehicle_get_suspension_force_vector(const VehiclePhysics *vehicle, u32 wheel_index,
                                          Vec3 *force) {
    if (force) {
        *force = vehicle_calculate_suspension_force(vehicle, wheel_index);
    }
}

f32 vehicle_get_total_suspension_force(const VehiclePhysics *vehicle) {
    if (!vehicle) return 0.0f;
    
    f32 total_force = 0.0f;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        Vec3 force = vehicle_calculate_suspension_force(vehicle, i);
        total_force += vec3_length(force);
    }
    
    return total_force;
}

// Wheel friction calculation with slip detection
typedef struct {
    Vec3 lateral_force;
    Vec3 longitudinal_force;
    Vec3 total_force;
    f32 slip_ratio;
    f32 slip_angle;
    bool is_slipping;
    f32 friction_coefficient;
} WheelFriction;

// Calculate wheel velocity for friction calculations
static Vec3 vehicle_get_wheel_velocity(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    
    // In a full implementation, this would calculate the actual wheel velocity
    // considering vehicle linear/angular velocity and wheel position
    // For now, we'll estimate based on vehicle velocity
    Vec3 wheel_velocity = vehicle->velocity;
    
    // Add rotational component from angular velocity
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, vehicle->wheels[wheel_index].position);
    
    // v = ω × r (cross product of angular velocity and position)
    Vec3 r = vec3_sub(wheel_world_pos, vehicle->position);
    Vec3 angular_velocity_contribution = vec3_cross(vehicle->angular_velocity, r);
    
    wheel_velocity = vec3_add(wheel_velocity, angular_velocity_contribution);
    
    return wheel_velocity;
}

// Calculate wheel slip ratio (longitudinal slip)
static f32 vehicle_calculate_slip_ratio(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return 0.0f;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded) {
        return 1.0f; // Maximum slip when not grounded
    }
    
    // Get wheel velocity
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    
    // Calculate wheel angular velocity (rotation speed)
    f32 wheel_angular_velocity = 0.0f;
    if (wheel->radius > 0.0f) {
        // v = ω * r, so ω = v / r
        Vec3 wheel_direction = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                                     vec3_cross(vehicle->rotation, wheel->position)));
        f32 forward_velocity = vec3_dot(wheel_velocity, wheel_direction);
        wheel_angular_velocity = forward_velocity / wheel->radius;
    }
    
    // Calculate theoretical rolling velocity
    f32 rolling_velocity = wheel_angular_velocity * wheel->radius;
    
    // Get actual forward velocity
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    f32 actual_velocity = vec3_dot(wheel_velocity, forward);
    
    // Slip ratio = (actual - rolling) / rolling
    if (fabsf(rolling_velocity) > 0.01f) {
        f32 slip_ratio = (actual_velocity - rolling_velocity) / rolling_velocity;
        return fmaxf(-1.0f, fminf(1.0f, slip_ratio));
    }
    
    return 0.0f;
}

// Calculate wheel slip angle (lateral slip)
static f32 vehicle_calculate_slip_angle(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return 0.0f;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded) {
        return 0.0f; // No slip angle when not grounded
    }
    
    // Get wheel velocity
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    
    // Calculate forward and lateral components
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    Vec3 lateral = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    f32 lateral_velocity = vec3_dot(wheel_velocity, lateral);
    
    // Slip angle = arctan(lateral / forward)
    if (fabsf(forward_velocity) > 0.01f) {
        f32 slip_angle = atan2f(lateral_velocity, forward_velocity);
        return slip_angle;
    }
    
    return 0.0f;
}

// Calculate friction coefficients based on slip
static void vehicle_calculate_friction_coefficients(const VehiclePhysics *vehicle, u32 wheel_index,
                                                   f32 *longitudinal_friction, f32 *lateral_friction) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || 
        !longitudinal_friction || !lateral_friction) {
        return;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    // Get slip values
    f32 slip_ratio = vehicle_calculate_slip_ratio(vehicle, wheel_index);
    f32 slip_angle = vehicle_calculate_slip_angle(vehicle, wheel_index);
    
    // Base friction coefficient
    f32 base_friction = wheel->friction_slip;
    
    // Reduce friction based on slip (Pacejka-like behavior)
    f32 slip_factor = fmaxf(0.3f, 1.0f - fabsf(slip_ratio) * 0.7f);
    f32 angle_factor = fmaxf(0.5f, 1.0f - fabsf(slip_angle) / (PI * 0.5f));
    
    *longitudinal_friction = base_friction * slip_factor;
    *lateral_friction = base_friction * angle_factor * 0.8f; // Lateral typically lower
    
    LOG_TRACE("Wheel %u friction: slip_ratio=%.3f, slip_angle=%.3f°, long=%.3f, lat=%.3f",
             wheel_index, slip_ratio, slip_angle * 180.0f / PI, 
             *longitudinal_friction, *lateral_friction);
}

// Calculate total friction forces on wheel
static WheelFriction vehicle_calculate_wheel_friction(const VehiclePhysics *vehicle, u32 wheel_index) {
    WheelFriction friction = {0};
    
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return friction;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded) {
        return friction; // No friction when not grounded
    }
    
    // Get wheel velocity
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    
    // Calculate slip values
    friction.slip_ratio = vehicle_calculate_slip_ratio(vehicle, wheel_index);
    friction.slip_angle = vehicle_calculate_slip_angle(vehicle, wheel_index);
    friction.is_slipping = (fabsf(friction.slip_ratio) > 0.1f) || (fabsf(friction.slip_angle) > 0.1f);
    
    // Calculate friction coefficients
    vehicle_calculate_friction_coefficients(vehicle, wheel_index, 
                                          &friction.friction_coefficient, 
                                          &friction.friction_coefficient);
    
    // Calculate force directions
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    Vec3 lateral = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    // Calculate normal force (from suspension)
    Vec3 suspension_force = vehicle_calculate_suspension_force(vehicle, wheel_index);
    f32 normal_force = vec3_length(suspension_force);
    
    // Calculate friction forces
    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    f32 lateral_velocity = vec3_dot(wheel_velocity, lateral);
    
    // Coulomb friction model: F = μ * N
    friction.longitudinal_force = vec3_scale(forward, -forward_velocity * friction.friction_coefficient * normal_force);
    friction.lateral_force = vec3_scale(lateral, -lateral_velocity * friction.friction_coefficient * normal_force);
    
    // Total friction force
    friction.total_force = vec3_add(friction.longitudinal_force, friction.lateral_force);
    
    LOG_TRACE("Wheel %u friction: F_lat=(%.1f,%.1f,%.1f)N, F_long=(%.1f,%.1f,%.1f)N, slip=%s",
             wheel_index, friction.lateral_force.x, friction.lateral_force.y, friction.lateral_force.z,
             friction.longitudinal_force.x, friction.longitudinal_force.y, friction.longitudinal_force.z,
             friction.is_slipping ? "YES" : "NO");
    
    return friction;
}

// Calculate friction forces for all wheels
static void vehicle_calculate_friction_forces(const VehiclePhysics *vehicle, WheelFriction *frictions) {
    if (!vehicle || !frictions) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        frictions[i] = vehicle_calculate_wheel_friction(vehicle, i);
    }
}

// Public API for friction system
f32 vehicle_get_wheel_slip_ratio(const VehiclePhysics *vehicle, u32 wheel_index) {
    return vehicle_calculate_slip_ratio(vehicle, wheel_index);
}

f32 vehicle_get_wheel_slip_angle(const VehiclePhysics *vehicle, u32 wheel_index) {
    return vehicle_calculate_slip_angle(vehicle, wheel_index);
}

void vehicle_get_wheel_friction_forces(const VehiclePhysics *vehicle, u32 wheel_index,
                                        Vec3 *lateral, Vec3 *longitudinal, Vec3 *total) {
    WheelFriction friction = vehicle_calculate_wheel_friction(vehicle, wheel_index);
    
    if (lateral) *lateral = friction.lateral_force;
    if (longitudinal) *longitudinal = friction.longitudinal_force;
    if (total) *total = friction.total_force;
}

bool vehicle_is_wheel_slipping(const VehiclePhysics *vehicle, u32 wheel_index) {
    WheelFriction friction = vehicle_calculate_wheel_friction(vehicle, wheel_index);
    return friction.is_slipping;
}

// Pacejka Magic Formula tire model implementation
typedef struct {
    // Longitudinal coefficients
    f32 a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13;
    
    // Lateral coefficients  
    f32 b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10;
    
    // Combined slip coefficients
    f32 c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17;
    
    // Aligning moment coefficients
    f32 d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16, d17;
    
    // Tire parameters
    f32 vertical_load;
    f32 friction_coefficient;
    f32 camber_angle;
} PacejkaCoefficients;

// Default Pacejka coefficients for a typical passenger car tire
static const PacejkaCoefficients k_default_pacejka = {
    // Longitudinal coefficients
    .a0 = 1.65, .a1 = -34.0, .a2 = 1250.0, .a3 = 3000.0, .a4 = 60.0, .a5 = 0.0,
    .a6 = 0.0, .a7 = 0.0, .a8 = -0.6, .a9 = 1.0, .a10 = 0.0, .a11 = 0.0, .a12 = 0.0, .a13 = 0.0,
    
    // Lateral coefficients
    .b0 = 1.65, .b1 = -34.0, .b2 = 1250.0, .b3 = 3000.0, .b4 = 60.0, .b5 = 0.0,
    .b6 = 0.0, .b7 = 0.0, .b8 = -0.6, .b9 = 1.0, .b10 = 0.0,
    
    // Combined slip coefficients
    .c0 = 1.65, .c1 = -34.0, .c2 = 1250.0, .c3 = 3000.0, .c4 = 60.0, .c5 = 0.0,
    .c6 = 0.0, .c7 = 0.0, .c8 = -0.6, .c9 = 1.0, .c10 = 0.0, .c11 = 0.0, .c12 = 0.0, .c13 = 0.0,
    .c14 = 0.0, .c15 = 0.0, .c16 = 0.0, .c17 = 0.0,
    
    // Aligning moment coefficients
    .d0 = 1.65, .d1 = -34.0, .d2 = 1250.0, .d3 = 3000.0, .d4 = 60.0, .d5 = 0.0,
    .d6 = 0.0, .d7 = 0.0, .d8 = -0.6, .d9 = 1.0, .d10 = 0.0, .d11 = 0.0, .d12 = 0.0, .d13 = 0.0,
    .d14 = 0.0, .d15 = 0.0, .d16 = 0.0, .d17 = 0.0,
    
    // Default tire parameters
    .vertical_load = 4000.0f, // 4000N (400kg) per wheel
    .friction_coefficient = 0.9f,
    .camber_angle = 0.0f
};

// Pacejka Magic Formula helper functions
static f32 pacejka_magic_formula(f32 B, f32 C, f32 D, f32 E, f32 S, f32 K, f32 x) {
    // F(x) = D * sin(C * arctan(B * x - E * (B * x - S - arctan(B * x)))) + K * x
    f32 bx = B * x;
    f32 bx_s = bx - S;
    f32 atan_bx_s = atanf(bx_s);
    f32 bx_s_atan = bx_s - atan_bx_s;
    f32 c_atan = C * atan_bx_s;
    f32 sin_c_atan = sinf(c_atan);
    f32 e_term = E * bx_s_atan;
    f32 argument = c_atan - e_term;
    f32 sin_argument = sinf(argument);
    
    return D * sin_argument + K * x;
}

// Calculate longitudinal tire force using Pacejka model
static f32 pacejka_longitudinal_force(const PacejkaCoefficients *coeff, f32 slip_ratio, f32 load) {
    // Normalize load
    f32 fz_n = load / coeff->vertical_load;
    
    // Shape factors
    f32 B = coeff->a3 * sinf(coeff->a2 * atanf(coeff->a1 * fz_n)) * (1.0f - coeff->a5 * fabsf(slip_ratio));
    f32 C = coeff->a4;
    f32 D = coeff->a1 * fz_n * coeff->friction_coefficient;
    f32 E = coeff->a6 * fz_n + coeff->a7;
    f32 S = coeff->a8 * fz_n + coeff->a9 + coeff->a10 * fz_n * fz_n;
    f32 K = coeff->a11 * fz_n + coeff->a12;
    
    return pacejka_magic_formula(B, C, D, E, S, K, slip_ratio);
}

// Calculate lateral tire force using Pacejka model
static f32 pacejka_lateral_force(const PacejkaCoefficients *coeff, f32 slip_angle, f32 load, f32 camber) {
    // Normalize load and convert slip angle to radians
    f32 fz_n = load / coeff->vertical_load;
    f32 alpha = slip_angle; // Already in radians
    
    // Shape factors
    f32 B = coeff->b3 * sinf(coeff->b2 * atanf(coeff->b1 * fz_n)) * (1.0f - coeff->b5 * fabsf(camber));
    f32 C = coeff->b4;
    f32 D = coeff->b1 * fz_n * coeff->friction_coefficient;
    f32 E = coeff->b6 * fz_n + coeff->b7;
    f32 S = coeff->b8 * fz_n + coeff->b9 + coeff->b10 * camber * fz_n;
    f32 K = coeff->b11 * fz_n + coeff->b12;
    
    return pacejka_magic_formula(B, C, D, E, S, K, alpha);
}

// Calculate combined slip forces (longitudinal + lateral)
static void pacejka_combined_forces(const PacejkaCoefficients *coeff, f32 slip_ratio, f32 slip_angle, 
                                    f32 load, f32 camber, f32 *longitudinal, f32 *lateral) {
    // Calculate pure forces
    f32 fx_pure = pacejka_longitudinal_force(coeff, slip_ratio, load);
    f32 fy_pure = pacejka_lateral_force(coeff, slip_angle, load, camber);
    
    // Combined slip reduction factors
    f32 slip_ratio_abs = fabsf(slip_ratio);
    f32 slip_angle_abs = fabsf(slip_angle);
    
    // Simplified combined slip model
    f32 reduction_factor = 1.0f;
    if (slip_ratio_abs > 0.01f && slip_angle_abs > 0.01f) {
        // Both longitudinal and lateral slip present
        f32 combined_slip = sqrtf(slip_ratio_abs * slip_ratio_abs + slip_angle_abs * slip_angle_abs);
        reduction_factor = fmaxf(0.5f, 1.0f - combined_slip * 0.3f);
    }
    
    *longitudinal = fx_pure * reduction_factor;
    *lateral = fy_pure * reduction_factor;
    
    LOG_TRACE("Pacejka: slip_ratio=%.3f, slip_angle=%.3f°, Fx=%.1fN, Fy=%.1fN, reduction=%.3f",
             slip_ratio, slip_angle * 180.0f / PI, *longitudinal, *lateral, reduction_factor);
}

// Calculate aligning moment (self-aligning torque)
static f32 pacejka_aligning_moment(const PacejkaCoefficients *coeff, f32 slip_angle, f32 load, f32 camber) {
    // Normalize load
    f32 fz_n = load / coeff->vertical_load;
    f32 alpha = slip_angle; // Already in radians
    
    // Shape factors
    f32 B = coeff->d3 * sinf(coeff->d2 * atanf(coeff->d1 * fz_n)) * (1.0f - coeff->d5 * fabsf(camber));
    f32 C = coeff->d4;
    f32 D = coeff->d1 * fz_n * coeff->friction_coefficient * coeff->vertical_load * 0.1f; // Scale for moment
    f32 E = coeff->d6 * fz_n + coeff->d7;
    f32 S = coeff->d8 * fz_n + coeff->d9 + coeff->d10 * camber * fz_n;
    f32 K = coeff->d11 * fz_n + coeff->d12;
    
    return pacejka_magic_formula(B, C, D, E, S, K, alpha);
}

// Enhanced wheel friction using Pacejka model
static WheelFriction vehicle_calculate_pacejka_friction(const VehiclePhysics *vehicle, u32 wheel_index) {
    WheelFriction friction = {0};
    
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return friction;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded) {
        return friction; // No friction when not grounded
    }
    
    // Get slip values
    friction.slip_ratio = vehicle_calculate_slip_ratio(vehicle, wheel_index);
    friction.slip_angle = vehicle_calculate_slip_angle(vehicle, wheel_index);
    friction.is_slipping = (fabsf(friction.slip_ratio) > 0.1f) || (fabsf(friction.slip_angle) > 0.1f);
    
    // Get normal force (from suspension)
    Vec3 suspension_force = vehicle_calculate_suspension_force(vehicle, wheel_index);
    f32 normal_force = vec3_length(suspension_force);
    
    // Use Pacejka model for force calculation
    f32 longitudinal_force, lateral_force;
    pacejka_combined_forces(&k_default_pacejka, friction.slip_ratio, friction.slip_angle,
                            normal_force, 0.0f, &longitudinal_force, &lateral_force);
    
    // Calculate force directions
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    Vec3 lateral_dir = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    // Apply forces in correct directions
    friction.longitudinal_force = vec3_scale(forward, longitudinal_force);
    friction.lateral_force = vec3_scale(lateral_dir, lateral_force);
    friction.total_force = vec3_add(friction.longitudinal_force, friction.lateral_force);
    friction.friction_coefficient = k_default_pacejka.friction_coefficient;
    
    // Calculate aligning moment (for steering feel)
    f32 aligning_moment = pacejka_aligning_moment(&k_default_pacejka, friction.slip_angle, 
                                                   normal_force, 0.0f);
    
    LOG_TRACE("Pacejka wheel %u: Fx=%.1fN, Fy=%.1fN, Mz=%.1fNm, slip_ratio=%.3f, slip_angle=%.3f°",
             wheel_index, longitudinal_force, lateral_force, aligning_moment,
             friction.slip_ratio, friction.slip_angle * 180.0f / PI);
    
    return friction;
}

// Public API for Pacejka tire model
void vehicle_set_pacejka_tire_model(VehiclePhysics *vehicle, bool enable) {
    // In a full implementation, this would switch between simple and Pacejka models
    LOG_INFO("Pacejka tire model %s", enable ? "enabled" : "disabled");
}

void vehicle_get_pacejka_tire_forces(const VehiclePhysics *vehicle, u32 wheel_index,
                                      f32 *longitudinal, f32 *lateral, f32 *aligning_moment) {
    WheelFriction friction = vehicle_calculate_pacejka_friction(vehicle, wheel_index);
    
    if (longitudinal) *longitudinal = vec3_length(friction.longitudinal_force);
    if (lateral) *lateral = vec3_length(friction.lateral_force);
    
    if (aligning_moment) {
        Vec3 suspension_force = vehicle_calculate_suspension_force(vehicle, wheel_index);
        f32 normal_force = vec3_length(suspension_force);
        *aligning_moment = pacejka_aligning_moment(&k_default_pacejka, friction.slip_angle, 
                                                   normal_force, 0.0f);
    }
}
