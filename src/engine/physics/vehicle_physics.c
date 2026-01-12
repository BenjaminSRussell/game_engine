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
#include <time.h>

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
    
    // Initialize wheel visual system
    vehicle_init_wheel_visuals(vehicle);
    
    // Initialize wheel effects system
    vehicle_init_wheel_effects(vehicle);
    
    // Initialize skid mark system
    vehicle_init_skid_mark_system();
    
    // Seed random number generator for particle effects
    srand((u32)time(NULL));
    
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
    
    // Update surface detection and friction coefficients
    vehicle_update_surface_detection(vehicle);
    
    // Update suspension system with force calculations
    vehicle_update_suspension_system(vehicle);
    
    // Update friction system with slip detection
    WheelFriction frictions[MAX_WHEELS];
    vehicle_calculate_friction_forces(vehicle, frictions);
    
    // Update wheel visual rotation system
    vehicle_update_wheel_visuals(vehicle, delta_time);
    
    // Update wheel dust and spray effects
    vehicle_update_wheel_effects(vehicle, delta_time);
    
    // Update skid mark system
    vehicle_update_skid_mark_system(vehicle, delta_time);
    
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
    
    // v =   r (cross product of angular velocity and position)
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
        // v =  * r, so  = v / r
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
    
    LOG_TRACE("Wheel %u friction: slip_ratio=%.3f, slip_angle=%.3f, long=%.3f, lat=%.3f",
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
    
    // Coulomb friction model: F =  * N
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
    
    LOG_TRACE("Pacejka: slip_ratio=%.3f, slip_angle=%.3f, Fx=%.1fN, Fy=%.1fN, reduction=%.3f",
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
    
    LOG_TRACE("Pacejka wheel %u: Fx=%.1fN, Fy=%.1fN, Mz=%.1fNm, slip_ratio=%.3f, slip_angle=%.3f",
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

// Wheel visual rotation system
typedef struct {
    f32 rotation_angle;        // Current rotation angle in radians
    f32 angular_velocity;      // Current angular velocity in rad/s
    f32 target_velocity;       // Target angular velocity for smooth transitions
    f32 steering_angle;        // Current steering angle
    f32 target_steering;       // Target steering angle
    f32 visual_scale;          // Scale factor for visual wheel size
    Vec3 visual_offset;        // Visual offset from physics position
    bool is_rotating;          // Is wheel currently rotating
    f32 rotation_damping;      // Damping factor for smooth rotation
} WheelVisualState;

static WheelVisualState g_wheel_visuals[MAX_WHEELS];

// Initialize wheel visual system
static void vehicle_init_wheel_visuals(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        WheelVisualState *visual = &g_wheel_visuals[i];
        const VehicleWheel *wheel = &vehicle->wheels[i];
        
        visual->rotation_angle = 0.0f;
        visual->angular_velocity = 0.0f;
        visual->target_velocity = 0.0f;
        visual->steering_angle = 0.0f;
        visual->target_steering = 0.0f;
        visual->visual_scale = 1.0f;
        visual->visual_offset = (Vec3){0.0f, 0.0f, 0.0f};
        visual->is_rotating = false;
        visual->rotation_damping = 5.0f;
        
        LOG_TRACE("Initialized visual state for wheel %u", i);
    }
}

// Calculate wheel angular velocity from vehicle motion
static f32 vehicle_calculate_wheel_angular_velocity(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return 0.0f;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded || wheel->radius <= 0.0f) {
        return 0.0f; // No rotation when not grounded or invalid radius
    }
    
    // Get wheel velocity at contact point
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    
    // Calculate forward direction
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    
    // Get forward velocity component
    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    
    // Angular velocity = linear velocity / radius
    f32 angular_velocity = -forward_velocity / wheel->radius; // Negative for correct rotation direction
    
    return angular_velocity;
}

// Update wheel visual rotation
static void vehicle_update_wheel_rotation(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return;
    }
    
    WheelVisualState *visual = &g_wheel_visuals[wheel_index];
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    // Calculate target angular velocity from vehicle motion
    visual->target_velocity = vehicle_calculate_wheel_angular_velocity(vehicle, wheel_index);
    
    // Smooth angular velocity transition
    f32 velocity_diff = visual->target_velocity - visual->angular_velocity;
    visual->angular_velocity += velocity_diff * (1.0f - expf(-visual->rotation_damping * delta_time));
    
    // Update rotation angle
    visual->rotation_angle += visual->angular_velocity * delta_time;
    
    // Keep angle in reasonable range (0 to 2)
    while (visual->rotation_angle > 2.0f * PI) {
        visual->rotation_angle -= 2.0f * PI;
    }
    while (visual->rotation_angle < 0.0f) {
        visual->rotation_angle += 2.0f * PI;
    }
    
    // Update steering angle (smooth transition)
    if (wheel->is_steered) {
        visual->target_steering = vehicle->steering * wheel->steering_angle;
        f32 steering_diff = visual->target_steering - visual->steering_angle;
        visual->steering_angle += steering_diff * (1.0f - expf(-visual->rotation_damping * delta_time));
    } else {
        visual->steering_angle = 0.0f;
        visual->target_steering = 0.0f;
    }
    
    // Check if wheel is rotating
    visual->is_rotating = fabsf(visual->angular_velocity) > 0.1f;
    
    // Update wheel physics rotation for consistency
    wheel->wheel_rotation = visual->rotation_angle;
    
    LOG_TRACE("Wheel %u visual: angle=%.3f, angular_vel=%.3f rad/s, steering=%.3f, rotating=%s",
             wheel_index, visual->rotation_angle * 180.0f / PI, visual->angular_velocity,
             visual->steering_angle * 180.0f / PI, visual->is_rotating ? "YES" : "NO");
}

// Update all wheel visual states
static void vehicle_update_wheel_visuals(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_update_wheel_rotation(vehicle, i, delta_time);
    }
}

// Calculate wheel world transform for rendering
static void vehicle_get_wheel_world_transform(const VehiclePhysics *vehicle, u32 wheel_index,
                                            Vec3 *position, Quat *rotation) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !position || !rotation) {
        return;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    const WheelVisualState *visual = &g_wheel_visuals[wheel_index];
    
    // Calculate chassis transform
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    
    // Get wheel base position
    Vec3 wheel_base_pos = mat4_transform_point(chassis_transform, wheel->position);
    
    // Add visual offset
    Vec3 wheel_world_pos = vec3_add(wheel_base_pos, visual->visual_offset);
    
    // Calculate wheel rotation
    // Start with chassis rotation
    Quat wheel_rotation = vehicle->rotation;
    
    // Add steering rotation (around Y axis)
    if (wheel->is_steered) {
        Quat steering_quat = quat_from_axis_angle((Vec3){0.0f, 1.0f, 0.0f}, visual->steering_angle);
        wheel_rotation = quat_multiply(wheel_rotation, steering_quat);
    }
    
    // Add wheel rotation (around forward axis)
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    Quat wheel_spin_quat = quat_from_axis_angle(forward, visual->rotation_angle);
    wheel_rotation = quat_multiply(wheel_rotation, wheel_spin_quat);
    
    *position = wheel_world_pos;
    *rotation = wheel_rotation;
}

// Public API for wheel visual system
void vehicle_get_wheel_visual_state(const VehiclePhysics *vehicle, u32 wheel_index,
                                    f32 *rotation_angle, f32 *steering_angle, bool *is_rotating) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        if (rotation_angle) *rotation_angle = 0.0f;
        if (steering_angle) *steering_angle = 0.0f;
        if (is_rotating) *is_rotating = false;
        return;
    }
    
    const WheelVisualState *visual = &g_wheel_visuals[wheel_index];
    
    if (rotation_angle) *rotation_angle = visual->rotation_angle;
    if (steering_angle) *steering_angle = visual->steering_angle;
    if (is_rotating) *is_rotating = visual->is_rotating;
}

void vehicle_set_wheel_visual_scale(VehiclePhysics *vehicle, u32 wheel_index, f32 scale) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return;
    
    g_wheel_visuals[wheel_index].visual_scale = fmaxf(0.1f, scale);
    LOG_DEBUG("Wheel %u visual scale set to %.2f", wheel_index, scale);
}

void vehicle_set_wheel_visual_offset(VehiclePhysics *vehicle, u32 wheel_index, Vec3 offset) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return;
    
    g_wheel_visuals[wheel_index].visual_offset = offset;
    LOG_DEBUG("Wheel %u visual offset set to (%.2f,%.2f,%.2f)", 
             wheel_index, offset.x, offset.y, offset.z);
}

void vehicle_get_wheel_render_transform(const VehiclePhysics *vehicle, u32 wheel_index,
                                        Vec3 *position, Quat *rotation, f32 *scale) {
    vehicle_get_wheel_world_transform(vehicle, wheel_index, position, rotation);
    
    if (scale) {
        *scale = g_wheel_visuals[wheel_index].visual_scale;
    }
}

// Enhanced wheel surface detection system
typedef enum {
    SURFACE_ASPHALT = 0,
    SURFACE_CONCRETE = 1,
    SURFACE_DIRT = 2,
    SURFACE_GRASS = 3,
    SURFACE_SAND = 4,
    SURFACE_GRAVEL = 5,
    SURFACE_MUD = 6,
    SURFACE_SNOW = 7,
    SURFACE_ICE = 8,
    SURFACE_WATER = 9,
    SURFACE_METAL = 10,
    SURFACE_WOOD = 11,
    SURFACE_ROCK = 12,
    SURFACE_UNKNOWN = 255
} SurfaceType;

typedef struct {
    SurfaceType type;
    f32 friction_coefficient;
    f32 rolling_resistance;
    f32 noise_factor;
    f32 dust_factor;
    f32 wetness;
    f32 temperature;
    Vec3 color_tint;
    bool is_deformable;
} SurfaceProperties;

// Surface properties database
static const SurfaceProperties k_surface_properties[] = {
    // Type, Friction, Rolling, Noise, Dust, Wetness, Temp, Color, Deformable
    {SURFACE_ASPHALT,   0.9f, 0.015f, 0.3f, 0.1f, 0.0f, 20.0f, {0.3f, 0.3f, 0.3f}, false},
    {SURFACE_CONCRETE,  0.8f, 0.020f, 0.4f, 0.1f, 0.0f, 20.0f, {0.4f, 0.4f, 0.4f}, false},
    {SURFACE_DIRT,     0.7f, 0.035f, 0.6f, 0.8f, 0.2f, 15.0f, {0.5f, 0.3f, 0.1f}, true},
    {SURFACE_GRASS,    0.6f, 0.040f, 0.5f, 0.6f, 0.3f, 10.0f, {0.2f, 0.5f, 0.1f}, true},
    {SURFACE_SAND,     0.5f, 0.050f, 0.7f, 1.0f, 0.0f, 25.0f, {0.8f, 0.7f, 0.4f}, true},
    {SURFACE_GRAVEL,   0.6f, 0.030f, 0.8f, 0.9f, 0.1f, 15.0f, {0.5f, 0.5f, 0.5f}, false},
    {SURFACE_MUD,      0.4f, 0.080f, 0.9f, 1.0f, 0.8f, 5.0f,  {0.3f, 0.2f, 0.1f}, true},
    {SURFACE_SNOW,     0.3f, 0.060f, 0.4f, 0.3f, 0.5f, -5.0f, {0.9f, 0.9f, 1.0f}, true},
    {SURFACE_ICE,      0.1f, 0.025f, 0.2f, 0.0f, 0.9f, -10.0f, {0.8f, 0.9f, 1.0f}, false},
    {SURFACE_WATER,    0.0f, 0.100f, 0.1f, 0.0f, 1.0f, 10.0f, {0.2f, 0.4f, 0.8f}, false},
    {SURFACE_METAL,    0.7f, 0.010f, 0.5f, 0.0f, 0.0f, 20.0f, {0.6f, 0.6f, 0.7f}, false},
    {SURFACE_WOOD,     0.5f, 0.025f, 0.6f, 0.4f, 0.1f, 15.0f, {0.4f, 0.2f, 0.1f}, false},
    {SURFACE_ROCK,     0.8f, 0.020f, 0.7f, 0.2f, 0.0f, 15.0f, {0.5f, 0.5f, 0.5f}, false},
    {SURFACE_UNKNOWN,  0.5f, 0.030f, 0.5f, 0.5f, 0.0f, 15.0f, {0.5f, 0.5f, 0.5f}, false}
};

// Enhanced surface detection using multiple criteria
static SurfaceType vehicle_detect_surface_type_enhanced(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return SURFACE_UNKNOWN;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded) {
        return SURFACE_UNKNOWN;
    }
    
    // Get contact point and analyze environment
    Vec3 contact_point = wheel->contact_point;
    
    // Surface detection based on height and position
    f32 height = contact_point.y;
    f32 x = contact_point.x;
    f32 z = contact_point.z;
    
    // Check for water (below certain height)
    if (height < -1.0f) {
        return SURFACE_WATER;
    }
    
    // Check for ice (very cold, low height)
    if (height < 0.0f && (sinf(x * 0.1f) + cosf(z * 0.1f)) > 0.5f) {
        return SURFACE_ICE;
    }
    
    // Check for snow (cold, moderate height)
    if (height < 0.5f && (sinf(x * 0.2f) + cosf(z * 0.2f)) > 0.3f) {
        return SURFACE_SNOW;
    }
    
    // Check for sand (dry, specific pattern)
    if (height < 0.2f && (sinf(x * 0.3f) * cosf(z * 0.3f)) > 0.2f) {
        return SURFACE_SAND;
    }
    
    // Check for mud (wet, low areas)
    if (height < 0.1f && (sinf(x * 0.15f) + cosf(z * 0.15f)) < -0.3f) {
        return SURFACE_MUD;
    }
    
    // Check for dirt/gravel (rough terrain)
    if (height < 0.3f && (fabsf(sinf(x * 0.25f)) + fabsf(cosf(z * 0.25f))) > 0.4f) {
        return (fabsf(sinf(x * 0.25f)) > 0.6f) ? SURFACE_DIRT : SURFACE_GRAVEL;
    }
    
    // Check for grass (vegetated areas)
    if (height < 0.4f && (sinf(x * 0.4f) + cosf(z * 0.4f)) > 0.1f) {
        return SURFACE_GRASS;
    }
    
    // Check for wood (special areas)
    if ((sinf(x * 0.5f) * cosf(z * 0.5f)) > 0.7f) {
        return SURFACE_WOOD;
    }
    
    // Check for metal (special areas)
    if ((cosf(x * 0.6f) * sinf(z * 0.6f)) > 0.8f) {
        return SURFACE_METAL;
    }
    
    // Check for rock (rough, high areas)
    if (height > 0.5f && (fabsf(sinf(x * 0.7f)) + fabsf(cosf(z * 0.7f))) > 0.6f) {
        return SURFACE_ROCK;
    }
    
    // Default to asphalt or concrete based on height
    return (height < 0.1f) ? SURFACE_ASPHALT : SURFACE_CONCRETE;
}

// Get surface properties for a wheel
static const SurfaceProperties* vehicle_get_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index) {
    SurfaceType surface_type = vehicle_detect_surface_type_enhanced(vehicle, wheel_index);
    
    for (u32 i = 0; i < sizeof(k_surface_properties) / sizeof(k_surface_properties[0]); i++) {
        if (k_surface_properties[i].type == surface_type) {
            return &k_surface_properties[i];
        }
    }
    
    return &k_surface_properties[13]; // Unknown surface
}

// Update wheel friction based on surface properties
static void vehicle_update_surface_friction(VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return;
    }
    
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    
    // Update wheel friction coefficient based on surface
    wheel->friction_slip = surface->friction_coefficient;
    
    LOG_TRACE("Wheel %u surface: type=%u, friction=%.3f, rolling=%.3f, noise=%.3f",
             wheel_index, surface->type, surface->friction_coefficient, 
             surface->rolling_resistance, surface->noise_factor);
}

// Update surface detection for all wheels
static void vehicle_update_surface_detection(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_update_surface_friction(vehicle, i);
    }
}

// Public API for surface detection
SurfaceType vehicle_get_wheel_surface_type(const VehiclePhysics *vehicle, u32 wheel_index) {
    return vehicle_detect_surface_type_enhanced(vehicle, wheel_index);
}

void vehicle_get_wheel_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index,
                                          f32 *friction, f32 *rolling_resistance, f32 *noise_factor) {
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    
    if (friction) *friction = surface->friction_coefficient;
    if (rolling_resistance) *rolling_resistance = surface->rolling_resistance;
    if (noise_factor) *noise_factor = surface->noise_factor;
}

bool vehicle_is_surface_deformable(const VehiclePhysics *vehicle, u32 wheel_index) {
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    return surface->is_deformable;
}

void vehicle_get_wheel_surface_color(const VehiclePhysics *vehicle, u32 wheel_index,
                                      Vec3 *color_tint) {
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    if (color_tint) *color_tint = surface->color_tint;
}

// Wheel dust and spray effects system
#define MAX_DUST_PARTICLES 128
#define MAX_SPRAY_PARTICLES 64
#define PARTICLE_LIFETIME 3.0f

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 color;
    f32 size;
    f32 lifetime;
    f32 max_lifetime;
    bool active;
    u32 type; // 0 = dust, 1 = spray, 2 = mud
} Particle;

typedef struct {
    Particle dust_particles[MAX_DUST_PARTICLES];
    Particle spray_particles[MAX_SPRAY_PARTICLES];
    u32 dust_count;
    u32 spray_count;
    f32 emission_timer;
    f32 spray_intensity;
    Vec3 wind_velocity;
    bool effects_enabled;
} WheelEffects;

static WheelEffects g_wheel_effects[MAX_WHEELS];

// Initialize wheel effects system
static void vehicle_init_wheel_effects(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        WheelEffects *effects = &g_wheel_effects[i];
        
        memset(effects, 0, sizeof(WheelEffects));
        effects->emission_timer = 0.0f;
        effects->spray_intensity = 0.0f;
        effects->wind_velocity = (Vec3){0.0f, 0.0f, 0.0f};
        effects->effects_enabled = true;
        
        LOG_TRACE("Initialized effects for wheel %u", i);
    }
}

// Create dust particle
static void vehicle_create_dust_particle(WheelEffects *effects, Vec3 position, Vec3 velocity, 
                                          Vec3 color, f32 size, u32 type) {
    if (effects->dust_count >= MAX_DUST_PARTICLES) {
        return; // Particle limit reached
    }
    
    Particle *particle = &effects->dust_particles[effects->dust_count];
    particle->position = position;
    particle->velocity = velocity;
    particle->color = color;
    particle->size = size;
    particle->lifetime = 0.0f;
    particle->max_lifetime = PARTICLE_LIFETIME;
    particle->active = true;
    particle->type = type;
    
    effects->dust_count++;
}

// Create spray particle
static void vehicle_create_spray_particle(WheelEffects *effects, Vec3 position, Vec3 velocity, 
                                          Vec3 color, f32 size) {
    if (effects->spray_count >= MAX_SPRAY_PARTICLES) {
        return; // Particle limit reached
    }
    
    Particle *particle = &effects->spray_particles[effects->spray_count];
    particle->position = position;
    particle->velocity = velocity;
    particle->color = color;
    particle->size = size;
    particle->lifetime = 0.0f;
    particle->max_lifetime = PARTICLE_LIFETIME * 0.5f; // Spray particles live shorter
    particle->active = true;
    particle->type = 1; // Spray type
    
    effects->spray_count++;
}

// Generate dust particles based on wheel motion
static void vehicle_generate_dust_particles(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    WheelEffects *effects = &g_wheel_effects[wheel_index];
    
    if (!effects->effects_enabled || !wheel->is_grounded) {
        return;
    }
    
    // Get surface properties
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    
    // Check if surface generates dust
    if (surface->dust_factor <= 0.0f) {
        return;
    }
    
    // Get wheel velocity
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    f32 speed = vec3_length(wheel_velocity);
    
    // Generate particles based on speed and surface
    if (speed > 1.0f) {
        effects->emission_timer += delta_time;
        
        f32 emission_rate = surface->dust_factor * (speed / 10.0f); // Particles per second
        f32 emission_interval = 1.0f / emission_rate;
        
        while (effects->emission_timer >= emission_interval) {
            effects->emission_timer -= emission_interval;
            
            // Calculate particle spawn position
            Vec3 spawn_pos = wheel->contact_point;
            spawn_pos.y += wheel->radius * 0.1f; // Slightly above ground
            
            // Calculate particle velocity (random spread + wheel velocity)
            Vec3 random_spread = {
                (rand() / (f32)RAND_MAX - 0.5f) * 2.0f,
                (rand() / (f32)RAND_MAX) * 3.0f + 1.0f, // Upward bias
                (rand() / (f32)RAND_MAX - 0.5f) * 2.0f
            };
            
            Vec3 particle_velocity = vec3_add(vec3_scale(wheel_velocity, 0.1f), random_spread);
            particle_velocity = vec3_add(particle_velocity, effects->wind_velocity);
            
            // Particle color based on surface
            Vec3 particle_color = surface->color_tint;
            
            // Particle size variation
            f32 particle_size = 0.05f + (rand() / (f32)RAND_MAX) * 0.1f;
            
            // Determine particle type
            u32 particle_type = 0; // Default dust
            if (surface->wetness > 0.5f) {
                particle_type = 1; // Spray (wet surface)
            } else if (surface->type == SURFACE_MUD) {
                particle_type = 2; // Mud
            }
            
            if (particle_type == 1) {
                vehicle_create_spray_particle(effects, spawn_pos, particle_velocity, 
                                              particle_color, particle_size);
            } else {
                vehicle_create_dust_particle(effects, spawn_pos, particle_velocity, 
                                              particle_color, particle_size, particle_type);
            }
        }
    }
}

// Update particle physics
static void vehicle_update_particles(WheelEffects *effects, f32 delta_time) {
    // Update dust particles
    for (u32 i = 0; i < effects->dust_count; i++) {
        Particle *particle = &effects->dust_particles[i];
        
        if (!particle->active) continue;
        
        // Update lifetime
        particle->lifetime += delta_time;
        
        // Remove dead particles
        if (particle->lifetime >= particle->max_lifetime) {
            particle->active = false;
            continue;
        }
        
        // Update position
        particle->position = vec3_add(particle->position, vec3_scale(particle->velocity, delta_time));
        
        // Apply gravity
        particle->velocity.y -= 9.81f * delta_time;
        
        // Apply wind
        particle->velocity = vec3_add(particle->velocity, vec3_scale(effects->wind_velocity, delta_time));
        
        // Apply damping
        particle->velocity = vec3_scale(particle->velocity, 0.98f);
        
        // Fade out over lifetime
        f32 fade_factor = 1.0f - (particle->lifetime / particle->max_lifetime);
        particle->color = vec3_scale(particle->color, fade_factor);
    }
    
    // Update spray particles
    for (u32 i = 0; i < effects->spray_count; i++) {
        Particle *particle = &effects->spray_particles[i];
        
        if (!particle->active) continue;
        
        // Update lifetime
        particle->lifetime += delta_time;
        
        // Remove dead particles
        if (particle->lifetime >= particle->max_lifetime) {
            particle->active = false;
            continue;
        }
        
        // Update position
        particle->position = vec3_add(particle->position, vec3_scale(particle->velocity, delta_time));
        
        // Apply gravity (stronger for spray)
        particle->velocity.y -= 15.0f * delta_time;
        
        // Apply wind
        particle->velocity = vec3_add(particle->velocity, vec3_scale(effects->wind_velocity, delta_time * 2.0f));
        
        // Apply damping
        particle->velocity = vec3_scale(particle->velocity, 0.95f);
        
        // Fade out over lifetime
        f32 fade_factor = 1.0f - (particle->lifetime / particle->max_lifetime);
        particle->color = vec3_scale(particle->color, fade_factor);
    }
    
    // Remove inactive particles (compact arrays)
    u32 dust_write = 0;
    for (u32 i = 0; i < effects->dust_count; i++) {
        if (effects->dust_particles[i].active) {
            effects->dust_particles[dust_write] = effects->dust_particles[i];
            dust_write++;
        }
    }
    effects->dust_count = dust_write;
    
    u32 spray_write = 0;
    for (u32 i = 0; i < effects->spray_count; i++) {
        if (effects->spray_particles[i].active) {
            effects->spray_particles[spray_write] = effects->spray_particles[i];
            spray_write++;
        }
    }
    effects->spray_count = spray_write;
}

// Update all wheel effects
static void vehicle_update_wheel_effects(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    
    // Update wind (simple wind simulation)
    f32 wind_time = delta_time * 0.1f;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        g_wheel_effects[i].wind_velocity.x = sinf(wind_time) * 2.0f;
        g_wheel_effects[i].wind_velocity.z = cosf(wind_time * 0.7f) * 2.0f;
    }
    
    // Generate and update particles
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_generate_dust_particles(vehicle, i, delta_time);
        vehicle_update_particles(&g_wheel_effects[i], delta_time);
    }
}

// Public API for wheel effects
void vehicle_set_wheel_effects_enabled(VehiclePhysics *vehicle, bool enabled) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        g_wheel_effects[i].effects_enabled = enabled;
    }
    
    LOG_INFO("Wheel effects %s", enabled ? "enabled" : "disabled");
}

void vehicle_set_wind_velocity(VehiclePhysics *vehicle, Vec3 wind) {
    if (!vehicle) return;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        g_wheel_effects[i].wind_velocity = wind;
    }
}

u32 vehicle_get_wheel_particle_count(const VehiclePhysics *vehicle, u32 wheel_index, 
                                     u32 *dust_count, u32 *spray_count) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        if (dust_count) *dust_count = 0;
        if (spray_count) *spray_count = 0;
        return 0;
    }
    
    const WheelEffects *effects = &g_wheel_effects[wheel_index];
    if (dust_count) *dust_count = effects->dust_count;
    if (spray_count) *spray_count = effects->spray_count;
    
    return effects->dust_count + effects->spray_count;
}

void vehicle_get_wheel_particles(const VehiclePhysics *vehicle, u32 wheel_index,
                                Particle *dust_particles, u32 *dust_returned,
                                Particle *spray_particles, u32 *spray_returned) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        if (dust_returned) *dust_returned = 0;
        if (spray_returned) *spray_returned = 0;
        return;
    }
    
    const WheelEffects *effects = &g_wheel_effects[wheel_index];
    
    if (dust_particles && dust_returned) {
        u32 count = fminf(*dust_returned, effects->dust_count);
        memcpy(dust_particles, effects->dust_particles, count * sizeof(Particle));
        *dust_returned = count;
    }
    
    if (spray_particles && spray_returned) {
        u32 count = fminf(*spray_returned, effects->spray_count);
        memcpy(spray_particles, effects->spray_particles, count * sizeof(Particle));
        *spray_returned = count;
    }
}

// Wheel skid marks system
#define MAX_SKID_MARKS 256
#define SKID_MARK_LIFETIME 30.0f
#define SKID_MARK_FADE_TIME 10.0f

typedef struct {
    Vec3 start_point;
    Vec3 end_point;
    Vec3 direction;
    f32 width;
    f32 intensity;
    f32 lifetime;
    f32 max_lifetime;
    u32 surface_type;
    bool active;
    u32 mark_id;
} SkidMark;

typedef struct {
    SkidMark skid_marks[MAX_SKID_MARKS];
    u32 skid_count;
    u32 next_mark_id;
    f32 total_skid_intensity;
    bool skid_marks_enabled;
} SkidMarkSystem;

static SkidMarkSystem g_skid_system = {0};

// Initialize skid mark system
static void vehicle_init_skid_mark_system(void) {
    memset(&g_skid_system, 0, sizeof(SkidMarkSystem));
    g_skid_system.next_mark_id = 1;
    g_skid_system.skid_marks_enabled = true;
    LOG_INFO("Initialized skid mark system");
}

// Create skid mark
static u32 vehicle_create_skid_mark(Vec3 start, Vec3 end, Vec3 direction, f32 width, 
                                 f32 intensity, u32 surface_type) {
    if (g_skid_system.skid_count >= MAX_SKID_MARKS) {
        return 0; // Mark limit reached
    }
    
    SkidMark *mark = &g_skid_system.skid_marks[g_skid_system.skid_count];
    
    mark->start_point = start;
    mark->end_point = end;
    mark->direction = direction;
    mark->width = width;
    mark->intensity = fminf(1.0f, intensity);
    mark->lifetime = 0.0f;
    mark->max_lifetime = SKID_MARK_LIFETIME;
    mark->surface_type = surface_type;
    mark->active = true;
    mark->mark_id = g_skid_system.next_mark_id++;
    
    g_skid_system.skid_count++;
    g_skid_system.total_skid_intensity += mark->intensity;
    
    LOG_TRACE("Created skid mark %u: intensity=%.2f, surface=%u", 
             mark->mark_id, mark->intensity, surface_type);
    
    return mark->mark_id;
}

// Generate skid marks based on wheel slip
static void vehicle_generate_skid_marks(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) {
        return;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    if (!wheel->is_grounded || !g_skid_system.skid_marks_enabled) {
        return;
    }
    
    // Check if wheel is slipping
    WheelFriction friction = vehicle_calculate_wheel_friction(vehicle, wheel_index);
    
    if (!friction.is_slipping || friction.slip_ratio < 0.1f) {
        return;
    }
    
    // Get surface properties
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    
    // Check if surface supports skid marks
    if (surface->friction_coefficient < 0.3f) {
        return; // Too slippery for skid marks
    }
    
    // Calculate skid mark parameters
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    f32 speed = vec3_length(wheel_velocity);
    
    // Skid mark intensity based on slip and speed
    f32 skid_intensity = fminf(1.0f, friction.slip_ratio * (speed / 20.0f));
    
    // Skid mark width based on wheel width
    f32 skid_width = wheel->width * 0.8f;
    
    // Calculate skid mark direction (perpendicular to wheel direction)
    Vec3 forward = vec3_normalize(vec3_cross(wheel->contact_normal, 
                                               vec3_cross(vehicle->rotation, wheel->position)));
    Vec3 lateral = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    // Skid mark start and end points
    Vec3 skid_start = wheel->contact_point;
    Vec3 skid_end = vec3_add(skid_start, vec3_scale(lateral, skid_width * 0.5f));
    
    // Create skid mark
    vehicle_create_skid_mark(skid_start, skid_end, lateral, skid_width, 
                            skid_intensity, surface->type);
}

// Update skid marks (fade out old marks)
static void vehicle_update_skid_marks(f32 delta_time) {
    for (u32 i = 0; i < g_skid_system.skid_count; i++) {
        SkidMark *mark = &g_skid_system.skid_marks[i];
        
        if (!mark->active) continue;
        
        // Update lifetime
        mark->lifetime += delta_time;
        
        // Remove expired marks
        if (mark->lifetime >= mark->max_lifetime) {
            mark->active = false;
            g_skid_system.total_skid_intensity -= mark->intensity;
            continue;
        }
        
        // Fade out over time
        if (mark->lifetime > SKID_MARK_LIFETIME - SKID_MARK_FADE_TIME) {
            f32 fade_progress = (mark->lifetime - (SKID_MARK_LIFETIME - SKID_MARK_FADE_TIME)) / SKID_MARK_FADE_TIME;
            mark->intensity *= (1.0f - fade_progress);
        }
    }
    
    // Remove inactive marks (compact array)
    u32 write_index = 0;
    for (u32 i = 0; i < g_skid_system.skid_count; i++) {
        if (g_skid_system.skid_marks[i].active) {
            g_skid_system.skid_marks[write_index] = g_skid_system.skid_marks[i];
            write_index++;
        }
    }
    g_skid_system.skid_count = write_index;
}

// Update all skid marks
static void vehicle_update_skid_mark_system(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    
    // Generate new skid marks
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_generate_skid_marks(vehicle, i, delta_time);
    }
    
    // Update existing skid marks
    vehicle_update_skid_marks(delta_time);
}

// Public API for skid mark system
void vehicle_set_skid_marks_enabled(VehiclePhysics *vehicle, bool enabled) {
    if (!vehicle) return;
    
    g_skid_system.skid_marks_enabled = enabled;
    LOG_INFO("Skid marks %s", enabled ? "enabled" : "disabled");
}

u32 vehicle_get_skid_mark_count(const VehiclePhysics *vehicle) {
    return g_skid_system.skid_count;
}

void vehicle_get_skid_marks(const VehiclePhysics *vehicle, SkidMark *marks, u32 *count) {
    if (!vehicle || !marks || !count) {
        if (count) *count = 0;
        return;
    }
    
    u32 return_count = fminf(*count, g_skid_system.skid_count);
    memcpy(marks, g_skid_marks, return_count * sizeof(SkidMark));
    *count = return_count;
}

f32 vehicle_get_total_skid_intensity(const VehiclePhysics *vehicle) {
    return g_skid_system.total_skid_intensity;
}

void vehicle_clear_skid_marks(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    
    memset(&g_skid_system, 0, sizeof(SkidMarkSystem));
    vehicle_init_skid_mark_system();
    
    LOG_INFO("Cleared all skid marks");
}
