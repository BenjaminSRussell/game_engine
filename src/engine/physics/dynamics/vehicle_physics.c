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
    
    // Update engine RPM based on wheel speed and gear
    // This would be implemented in the engine todo
    
    // Update physics simulation
    // This would include forces, integration, etc.
    
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
