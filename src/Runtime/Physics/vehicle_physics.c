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

// Macros for unified logger
#undef LOG_INFO
#undef LOG_TRACE
#undef LOG_DEBUG
#undef LOG_WARN
#undef LOG_ERROR
#define LOG_INFO(fmt, ...) unified_logger_log(LOG_LEVEL_INFO, LOG_CAT_PHYSICS, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) unified_logger_log(LOG_LEVEL_TRACE, LOG_CAT_PHYSICS, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) unified_logger_log(LOG_LEVEL_DEBUG, LOG_CAT_PHYSICS, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) unified_logger_log(LOG_LEVEL_WARN, LOG_CAT_PHYSICS, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) unified_logger_log(LOG_LEVEL_ERROR, LOG_CAT_PHYSICS, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// Constants
#define RAYCAST_LENGTH 2.0f
#define MAX_RAYCAST_HITS 16
#define PARTICLE_LIFETIME 3.0f
#define MAX_SKID_MARKS 256
#define SKID_MARK_LIFETIME 30.0f
#define SKID_MARK_FADE_TIME 10.0f

// Internal Structs
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
} WheelRaycastResult;

typedef struct {
    Vec3 lateral_force;
    Vec3 longitudinal_force;
    Vec3 total_force;
    f32 slip_ratio;
    f32 slip_angle;
    bool is_slipping;
    f32 friction_coefficient;
} WheelFriction;

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

typedef struct {
    VehicleSkidMark skid_marks[MAX_SKID_MARKS];
    u32 skid_count;
    u32 next_mark_id;
    f32 total_skid_intensity;
    bool skid_marks_enabled;
} SkidMarkSystem;

typedef struct {
    f32 a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13;
    f32 b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10;
    f32 c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17;
    f32 d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16, d17;
    f32 vertical_load;
    f32 friction_coefficient;
    f32 camber_angle;
} PacejkaCoefficients;

// Global/Static State
static SkidMarkSystem g_skid_system = {0};
static bool s_skid_system_initialized = false;

static const SurfaceProperties k_surface_properties[] = {
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

static const PacejkaCoefficients k_default_pacejka = {
    .a0 = 1.65, .a1 = -34.0, .a2 = 1250.0, .a3 = 3000.0, .a4 = 60.0, .a5 = 0.0,
    .a6 = 0.0, .a7 = 0.0, .a8 = -0.6, .a9 = 1.0, .a10 = 0.0, .a11 = 0.0, .a12 = 0.0, .a13 = 0.0,
    .b0 = 1.65, .b1 = -34.0, .b2 = 1250.0, .b3 = 3000.0, .b4 = 60.0, .b5 = 0.0,
    .b6 = 0.0, .b7 = 0.0, .b8 = -0.6, .b9 = 1.0, .b10 = 0.0,
    .c0 = 1.65, .c1 = -34.0, .c2 = 1250.0, .c3 = 3000.0, .c4 = 60.0, .c5 = 0.0,
    .c6 = 0.0, .c7 = 0.0, .c8 = -0.6, .c9 = 1.0, .c10 = 0.0, .c11 = 0.0, .c12 = 0.0, .c13 = 0.0,
    .c14 = 0.0, .c15 = 0.0, .c16 = 0.0, .c17 = 0.0,
    .d0 = 1.65, .d1 = -34.0, .d2 = 1250.0, .d3 = 3000.0, .d4 = 60.0, .d5 = 0.0,
    .d6 = 0.0, .d7 = 0.0, .d8 = -0.6, .d9 = 1.0, .d10 = 0.0, .d11 = 0.0, .d12 = 0.0, .d13 = 0.0,
    .d14 = 0.0, .d15 = 0.0, .d16 = 0.0, .d17 = 0.0,
    .vertical_load = 4000.0f,
    .friction_coefficient = 0.9f,
    .camber_angle = 0.0f
};

// Helper
static Mat4 mat4_from_quat_translation(Quat q, Vec3 p) {
    Mat4 rot = quat_to_mat4(q);
    rot.m30 = p.x;
    rot.m31 = p.y;
    rot.m32 = p.z;
    return rot;
}

// Forward Declarations
static void vehicle_init_wheel_visuals(VehiclePhysics *vehicle);
static void vehicle_init_wheel_effects(VehiclePhysics *vehicle);
static void vehicle_init_skid_mark_system(void);
static void vehicle_update_surface_detection(VehiclePhysics *vehicle);
static void vehicle_update_suspension_system(VehiclePhysics *vehicle);
static void vehicle_calculate_friction_forces(const VehiclePhysics *vehicle, WheelFriction *frictions);
static void vehicle_update_wheel_rotation(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time);
static void vehicle_update_wheel_visuals(VehiclePhysics *vehicle, f32 delta_time);
static void vehicle_update_wheel_effects(VehiclePhysics *vehicle, f32 delta_time);
static void vehicle_update_skid_mark_system(VehiclePhysics *vehicle, f32 delta_time);
static Vec3 get_wheel_forward(const VehiclePhysics *vehicle, const VehicleWheel *wheel);
static Vec3 vehicle_get_wheel_velocity(const VehiclePhysics *vehicle, u32 wheel_index);
static WheelFriction vehicle_calculate_wheel_friction(const VehiclePhysics *vehicle, u32 wheel_index);
static const SurfaceProperties* vehicle_get_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index);
static void vehicle_create_dust_particle(VehicleWheelEffects *effects, Vec3 position, Vec3 velocity, Vec3 color, f32 size, u32 type);
static void vehicle_create_spray_particle(VehicleWheelEffects *effects, Vec3 position, Vec3 velocity, Vec3 color, f32 size);
static void vehicle_generate_dust_particles(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time);
static void vehicle_update_particles(VehicleWheelEffects *effects, f32 delta_time);
static u32 vehicle_create_skid_mark(Vec3 start, Vec3 end, Vec3 direction, f32 width, f32 intensity, u32 surface_type);
static void vehicle_generate_skid_marks(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time);
static void vehicle_update_skid_marks(f32 delta_time);
static f32 vehicle_calculate_wheel_angular_velocity(const VehiclePhysics *vehicle, u32 wheel_index);
static void vehicle_update_surface_friction(VehiclePhysics *vehicle, u32 wheel_index);
static void vehicle_get_wheel_world_transform(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *position, Quat *rotation);

// Implementation

static bool vehicle_wheel_raycast(const VehiclePhysics *vehicle, u32 wheel_index, WheelRaycast *raycast) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !raycast) return false;
    
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, vehicle->wheels[wheel_index].position);
    
    raycast->origin = wheel_world_pos;
    raycast->direction = (Vec3){0.0f, -1.0f, 0.0f};
    raycast->max_distance = vehicle->wheels[wheel_index].suspension_length + vehicle->wheels[wheel_index].radius;
    raycast->hit_found = false;
    raycast->hit_distance = raycast->max_distance;
    
    if (vehicle->raycast_cb) {
        raycast->hit_found = vehicle->raycast_cb(vehicle->raycast_user_data, raycast->origin, raycast->direction, raycast->max_distance, &raycast->hit_point, &raycast->hit_normal, &raycast->hit_distance);
        if (raycast->hit_found) {
            raycast->surface_type = 0;
            LOG_TRACE("Wheel %u raycast hit", wheel_index);
        }
        return raycast->hit_found;
    }

    f32 ground_y = 0.0f;
    f32 t = (ground_y - raycast->origin.y) / raycast->direction.y;
    if (t > 0.0f && t <= raycast->max_distance) {
        raycast->hit_point = vec3_add(raycast->origin, vec3_mul(raycast->direction, t));
        raycast->hit_normal = (Vec3){0.0f, 1.0f, 0.0f};
        raycast->hit_distance = t;
        raycast->hit_found = true;
        raycast->surface_type = 0;
        LOG_TRACE("Wheel %u raycast hit", wheel_index);
    }
    
    return raycast->hit_found;
}

static bool vehicle_wheel_raycast_multi(const VehiclePhysics *vehicle, u32 wheel_index, WheelRaycastResult *result) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !result) return false;
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    result->count = 0;
    
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, wheel->position);
    
    f32 raycast_spacing = wheel->width * 0.25f;
    Vec3 raycast_offsets[5] = {
        {0.0f, 0.0f, 0.0f},
        {raycast_spacing, 0.0f, 0.0f},
        {-raycast_spacing, 0.0f, 0.0f},
        {0.0f, 0.0f, raycast_spacing},
        {0.0f, 0.0f, -raycast_spacing}
    };
    
    for (u32 i = 0; i < 5 && result->count < MAX_RAYCAST_HITS; i++) {
        Vec3 ray_origin = vec3_add(wheel_world_pos, raycast_offsets[i]);
        Vec3 ray_direction = (Vec3){0.0f, -1.0f, 0.0f};
        f32 max_distance = wheel->suspension_length + wheel->radius;
        
        bool hit = false;
        Vec3 hit_point, hit_normal;
        f32 hit_distance;

        if (vehicle->raycast_cb) {
            hit = vehicle->raycast_cb(vehicle->raycast_user_data, ray_origin, ray_direction, max_distance, &hit_point, &hit_normal, &hit_distance);
        } else {
            f32 ground_y = 0.0f;
            f32 t = (ground_y - ray_origin.y) / ray_direction.y;
            if (t > 0.0f && t <= max_distance) {
                hit = true;
                hit_point = vec3_add(ray_origin, vec3_mul(ray_direction, t));
                hit_normal = (Vec3){0.0f, 1.0f, 0.0f};
                hit_distance = t;
            }
        }
        
        if (hit) {
            result->points[result->count] = hit_point;
            result->normals[result->count] = hit_normal;
            result->count++;
        }
    }
    
    return result->count > 0;
}

static u32 vehicle_detect_surface_type(const WheelRaycastResult *result) {
    if (!result || result->count == 0) return 0;
    
    Vec3 avg_point = {0.0f, 0.0f, 0.0f};
    for (u32 i = 0; i < result->count; i++) {
        avg_point = vec3_add(avg_point, result->points[i]);
    }
    avg_point = vec3_mul(avg_point, 1.0f / result->count);
    
    if (avg_point.y < 0.1f) return 1;
    else if (avg_point.y < 0.5f) return 2;
    else return 3;
}

static void vehicle_update_wheel_contact(VehiclePhysics *vehicle, u32 wheel_index) {
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    WheelRaycastResult result;
    bool has_contact = vehicle_wheel_raycast_multi(vehicle, wheel_index, &result);
    
    if (has_contact) {
        Vec3 avg_point = {0.0f, 0.0f, 0.0f};
        Vec3 avg_normal = {0.0f, 0.0f, 0.0f};
        
        for (u32 i = 0; i < result.count; i++) {
            avg_point = vec3_add(avg_point, result.points[i]);
            avg_normal = vec3_add(avg_normal, result.normals[i]);
        }
        
        avg_point = vec3_mul(avg_point, 1.0f / result.count);
        avg_normal = vec3_normalize(avg_normal);
        
        wheel->contact_point = avg_point;
        wheel->contact_normal = avg_normal;
        wheel->is_grounded = true;
        
        Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
        Vec3 wheel_world_pos = mat4_transform_point(chassis_transform, wheel->position);
        f32 distance_to_ground = vec3_distance(wheel_world_pos, avg_point);
        wheel->compression = wheel->suspension_length - (distance_to_ground - wheel->radius);
        wheel->compression = fmaxf(0.0f, fminf(wheel->compression, wheel->suspension_length));
    } else {
        wheel->is_grounded = false;
        wheel->compression = 0.0f;
        wheel->contact_normal = (Vec3){0.0f, 1.0f, 0.0f};
    }
}

// ... Public API ...

VehiclePhysics *vehicle_create(u32 wheel_count) {
    if (wheel_count == 0 || wheel_count > MAX_WHEELS) {
        LOG_ERROR("Invalid wheel count: %u", wheel_count);
        return NULL;
    }
    
    VehiclePhysics *vehicle = (VehiclePhysics *)calloc(1, sizeof(VehiclePhysics));
    if (!vehicle) return NULL;
    
    vehicle->wheel_count = 0;
    vehicle->mass = 1500.0f;
    vehicle->center_of_mass = (Vec3){0.0f, 0.0f, 0.0f};
    vehicle->max_engine_torque = 300.0f;
    vehicle->max_rpm = 6000.0f;
    vehicle->drag_coefficient = 0.3f;
    vehicle->downforce_coefficient = 0.4f;
    vehicle->slip_angle_limit = 0.2f;
    
    vehicle->current_gear = 1;
    vehicle->gear_count = 5;
    vehicle->gear_ratios[0] = 3.5f; // Reverse
    vehicle->gear_ratios[1] = 3.5f; // 1st
    vehicle->gear_ratios[2] = 2.1f; // 2nd
    vehicle->gear_ratios[3] = 1.4f; // 3rd
    vehicle->gear_ratios[4] = 1.0f; // 4th
    vehicle->gear_ratios[5] = 0.8f; // 5th
    vehicle->differential_ratio = 3.5f;
    
    vehicle_init_wheel_visuals(vehicle);
    vehicle_init_wheel_effects(vehicle);
    vehicle_init_skid_mark_system();
    
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
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_update_wheel_contact(vehicle, i);
    }
    
    vehicle_update_surface_detection(vehicle);
    vehicle_update_suspension_system(vehicle);
    
    WheelFriction frictions[MAX_WHEELS];
    vehicle_calculate_friction_forces(vehicle, frictions);
    
    vehicle_update_wheel_visuals(vehicle, delta_time);
    vehicle_update_wheel_effects(vehicle, delta_time);
    vehicle_update_skid_mark_system(vehicle, delta_time);
}

void vehicle_set_input(VehiclePhysics *vehicle, f32 throttle, f32 brake,
                       f32 steering, f32 handbrake) {
    if (!vehicle) return;
    vehicle->throttle = fmaxf(0.0f, fminf(1.0f, throttle));
    vehicle->brake = fmaxf(0.0f, fminf(1.0f, brake));
    vehicle->steering = fmaxf(-1.0f, fminf(1.0f, steering));
    vehicle->handbrake = fmaxf(0.0f, fminf(1.0f, handbrake));
}

void vehicle_add_wheel(VehiclePhysics *vehicle, Vec3 offset, f32 radius,
                       bool driven, bool steered) {
    if (!vehicle || vehicle->wheel_count >= MAX_WHEELS) return;
    
    u32 wheel_index = vehicle->wheel_count;
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    wheel->position = offset;
    wheel->radius = radius;
    wheel->width = radius * 0.3f;
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
    
    LOG_INFO("Added wheel %u", wheel_index);
}

void vehicle_set_raycast_callback(VehiclePhysics *vehicle, VehicleRaycastCallback callback, void *user_data) {
    if (!vehicle) return;
    vehicle->raycast_cb = callback;
    vehicle->raycast_user_data = user_data;
    LOG_INFO("Vehicle raycast callback set");
}

bool vehicle_get_wheel_contact(const VehiclePhysics *vehicle, u32 wheel_index,
                               Vec3 *contact_point, Vec3 *contact_normal, bool *is_grounded) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return false;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (contact_point) *contact_point = wheel->contact_point;
    if (contact_normal) *contact_normal = wheel->contact_normal;
    if (is_grounded) *is_grounded = wheel->is_grounded;
    return wheel->is_grounded;
}

f32 vehicle_get_wheel_compression(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return 0.0f;
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

static Vec3 vehicle_calculate_suspension_force(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return (Vec3){0,0,0};
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded || wheel->compression <= 0.0f) return (Vec3){0,0,0};
    
    f32 spring_force = wheel->suspension_stiffness * wheel->compression;
    f32 damping_force = wheel->suspension_damping * wheel->compression * 0.1f;
    f32 total_force = spring_force + damping_force;
    
    return vec3_mul(wheel->contact_normal, total_force);
}

static void vehicle_calculate_suspension_forces(VehiclePhysics *vehicle, Vec3 *forces) {
    if (!vehicle || !forces) return;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        forces[i] = vehicle_calculate_suspension_force(vehicle, i);
    }
}

static Vec3 vehicle_calculate_anti_roll_force(const VehiclePhysics *vehicle) {
    if (!vehicle || vehicle->wheel_count < 4) return (Vec3){0,0,0};
    f32 left = 0, right = 0;
    int lc = 0, rc = 0;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        if (vehicle->wheels[i].position.x < 0) { left += vehicle->wheels[i].compression; lc++; }
        else { right += vehicle->wheels[i].compression; rc++; }
    }
    if (lc > 0) left /= lc;
    if (rc > 0) right /= rc;
    f32 diff = left - right;
    return (Vec3){0, 0, diff * 5000.0f};
}

static void vehicle_update_suspension_system(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    Vec3 suspension_forces[MAX_WHEELS];
    vehicle_calculate_suspension_forces(vehicle, suspension_forces);
    
    Vec3 total_suspension_force = {0.0f, 0.0f, 0.0f};
    u32 grounded_wheels = 0;
    
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        if (vehicle->wheels[i].is_grounded) {
            total_suspension_force = vec3_add(total_suspension_force, suspension_forces[i]);
            grounded_wheels++;
        }
    }
    
    if (grounded_wheels > 0) {
        Vec3 avg_suspension_force = vec3_mul(total_suspension_force, 1.0f / grounded_wheels);
        for (u32 i = 0; i < vehicle->wheel_count; i++) {
            if (vehicle->wheels[i].is_grounded) {
                Vec3 force_diff = vec3_sub(suspension_forces[i], avg_suspension_force);
                f32 balance_factor = 0.1f;
                Vec3 balancing_force = vec3_mul(force_diff, -balance_factor);
                suspension_forces[i] = vec3_add(suspension_forces[i], balancing_force);
            }
        }
    }
}

f32 vehicle_get_suspension_force_magnitude(const VehiclePhysics *vehicle, u32 wheel_index) {
    Vec3 f = vehicle_calculate_suspension_force(vehicle, wheel_index);
    return vec3_length(f);
}

void vehicle_get_suspension_force_vector(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *force) {
    if (force) *force = vehicle_calculate_suspension_force(vehicle, wheel_index);
}

f32 vehicle_get_total_suspension_force(const VehiclePhysics *vehicle) {
    f32 t = 0;
    for(u32 i=0; i<vehicle->wheel_count; i++) t += vec3_length(vehicle_calculate_suspension_force(vehicle, i));
    return t;
}

static Vec3 vehicle_get_wheel_velocity(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return (Vec3){0,0,0};
    Vec3 v = vehicle->velocity;
    Mat4 tr = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wp = mat4_transform_point(tr, vehicle->wheels[wheel_index].position);
    Vec3 r = vec3_sub(wp, vehicle->position);
    Vec3 ang = vec3_cross(vehicle->angular_velocity, r);
    return vec3_add(v, ang);
}

static Vec3 get_wheel_forward(const VehiclePhysics *vehicle, const VehicleWheel *wheel) {
    Quat sq = quat_from_axis_angle((Vec3){0,1,0}, wheel->steering_angle);
    Quat q = quat_mul(vehicle->rotation, sq);
    return quat_rotate_vec3(q, (Vec3){0,0,1});
}

static f32 vehicle_calculate_slip_ratio(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return 0.0f;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded) return 1.0f;
    
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    Vec3 forward = get_wheel_forward(vehicle, wheel);
    forward = vec3_sub(forward, vec3_mul(wheel->contact_normal, vec3_dot(forward, wheel->contact_normal)));
    forward = vec3_normalize(forward);

    f32 wheel_angular_velocity = 0.0f;
    if (wheel->radius > 0.0f) {
        f32 forward_velocity = vec3_dot(wheel_velocity, forward);
        wheel_angular_velocity = forward_velocity / wheel->radius;
    }
    
    f32 rolling_velocity = wheel_angular_velocity * wheel->radius;
    f32 actual_velocity = vec3_dot(wheel_velocity, forward);
    
    if (fabsf(rolling_velocity) > 0.01f) return fmaxf(-1.0f, fminf(1.0f, (actual_velocity - rolling_velocity) / rolling_velocity));
    return 0.0f;
}

static f32 vehicle_calculate_slip_angle(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return 0.0f;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded) return 0.0f;
    
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    
    Vec3 forward = get_wheel_forward(vehicle, wheel);
    forward = vec3_sub(forward, vec3_mul(wheel->contact_normal, vec3_dot(forward, wheel->contact_normal)));
    forward = vec3_normalize(forward);
    Vec3 lateral = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    f32 lateral_velocity = vec3_dot(wheel_velocity, lateral);
    
    if (fabsf(forward_velocity) > 0.01f) return atan2f(lateral_velocity, forward_velocity);
    return 0.0f;
}

static void vehicle_calculate_friction_coefficients(const VehiclePhysics *vehicle, u32 wheel_index, f32 *long_f, f32 *lat_f) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !long_f || !lat_f) return;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    f32 slip_ratio = vehicle_calculate_slip_ratio(vehicle, wheel_index);
    f32 slip_angle = vehicle_calculate_slip_angle(vehicle, wheel_index);
    f32 base_friction = wheel->friction_slip;
    f32 slip_factor = fmaxf(0.3f, 1.0f - fabsf(slip_ratio) * 0.7f);
    f32 angle_factor = fmaxf(0.5f, 1.0f - fabsf(slip_angle) / (PI * 0.5f));
    *long_f = base_friction * slip_factor;
    *lat_f = base_friction * angle_factor * 0.8f;
}

static WheelFriction vehicle_calculate_wheel_friction(const VehiclePhysics *vehicle, u32 wheel_index) {
    WheelFriction friction = {0};
    if (!vehicle || wheel_index >= vehicle->wheel_count) return friction;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded) return friction;
    
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    friction.slip_ratio = vehicle_calculate_slip_ratio(vehicle, wheel_index);
    friction.slip_angle = vehicle_calculate_slip_angle(vehicle, wheel_index);
    friction.is_slipping = (fabsf(friction.slip_ratio) > 0.1f) || (fabsf(friction.slip_angle) > 0.1f);
    
    vehicle_calculate_friction_coefficients(vehicle, wheel_index, &friction.friction_coefficient, &friction.friction_coefficient);

    Vec3 forward = get_wheel_forward(vehicle, wheel);
    forward = vec3_sub(forward, vec3_mul(wheel->contact_normal, vec3_dot(forward, wheel->contact_normal)));
    forward = vec3_normalize(forward);
    
    Vec3 lateral = vec3_normalize(vec3_cross(forward, wheel->contact_normal));
    
    Vec3 suspension_force = vehicle_calculate_suspension_force(vehicle, wheel_index);
    f32 normal_force = vec3_length(suspension_force);
    
    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    f32 lateral_velocity = vec3_dot(wheel_velocity, lateral);
    
    friction.longitudinal_force = vec3_mul(forward, -forward_velocity * friction.friction_coefficient * normal_force);
    friction.lateral_force = vec3_mul(lateral, -lateral_velocity * friction.friction_coefficient * normal_force);
    
    friction.total_force = vec3_add(friction.longitudinal_force, friction.lateral_force);
    
    return friction;
}

static void vehicle_calculate_friction_forces(const VehiclePhysics *vehicle, WheelFriction *frictions) {
    if (!vehicle || !frictions) return;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        frictions[i] = vehicle_calculate_wheel_friction(vehicle, i);
    }
}

// ... Visuals and Effects ...

static void vehicle_init_wheel_visuals(VehiclePhysics *vehicle) {
    for (u32 i=0; i<vehicle->wheel_count; i++) {
        vehicle->visuals[i].visual_scale = 1.0f;
        vehicle->visuals[i].rotation_damping = 5.0f;
    }
}

static void vehicle_update_wheel_rotation(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return;
    VehicleWheelVisuals *visual = &vehicle->visuals[wheel_index];
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    
    visual->target_velocity = vehicle_calculate_wheel_angular_velocity(vehicle, wheel_index);
    f32 velocity_diff = visual->target_velocity - visual->angular_velocity;
    visual->angular_velocity += velocity_diff * (1.0f - expf(-visual->rotation_damping * delta_time));
    visual->rotation_angle += visual->angular_velocity * delta_time;
    
    while (visual->rotation_angle > 2.0f * PI) visual->rotation_angle -= 2.0f * PI;
    while (visual->rotation_angle < 0.0f) visual->rotation_angle += 2.0f * PI;
    
    if (wheel->is_steered) {
        visual->target_steering = vehicle->steering * wheel->steering_angle;
        f32 steering_diff = visual->target_steering - visual->steering_angle;
        visual->steering_angle += steering_diff * (1.0f - expf(-visual->rotation_damping * delta_time));
    } else {
        visual->steering_angle = 0.0f;
        visual->target_steering = 0.0f;
    }
    
    visual->is_rotating = fabsf(visual->angular_velocity) > 0.1f;
    ((VehicleWheel*)wheel)->wheel_rotation = visual->rotation_angle;
}

static void vehicle_update_wheel_visuals(VehiclePhysics *vehicle, f32 delta_time) {
    for (u32 i=0; i<vehicle->wheel_count; i++) {
        vehicle_update_wheel_rotation(vehicle, i, delta_time);
    }
}

static void vehicle_init_wheel_effects(VehiclePhysics *vehicle) {
    for (u32 i=0; i<vehicle->wheel_count; i++) {
        memset(&vehicle->effects[i], 0, sizeof(VehicleWheelEffects));
        vehicle->effects[i].effects_enabled = true;
    }
}

static void vehicle_init_skid_mark_system(void) {
    if (!s_skid_system_initialized) {
        memset(&g_skid_system, 0, sizeof(SkidMarkSystem));
        g_skid_system.skid_marks_enabled = true;
        s_skid_system_initialized = true;
        LOG_INFO("Initialized skid mark system");
    }
}

static void vehicle_update_surface_detection(VehiclePhysics *vehicle) {
    if (!vehicle) return;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_update_surface_friction(vehicle, i);
    }
}

static void vehicle_update_surface_friction(VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return;
    VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    const SurfaceProperties *surface = vehicle_get_surface_properties(vehicle, wheel_index);
    wheel->friction_slip = surface->friction_coefficient;
}

static SurfaceType vehicle_detect_surface_type_enhanced(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return SURFACE_UNKNOWN;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded) return SURFACE_UNKNOWN;
    if (wheel->contact_point.y < 0.1f) return SURFACE_ASPHALT;
    return SURFACE_DIRT;
}

static const SurfaceProperties* vehicle_get_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index) {
    return &k_surface_properties[0];
}

// Getters setters for public API
void vehicle_get_wheel_visual_state(const VehiclePhysics *vehicle, u32 wheel_index, f32 *r, f32 *s, bool *i) {
    if (wheel_index < MAX_WHEELS) {
        if (r) *r = vehicle->visuals[wheel_index].rotation_angle;
        if (s) *s = vehicle->visuals[wheel_index].steering_angle;
        if (i) *i = vehicle->visuals[wheel_index].is_rotating;
    }
}
void vehicle_set_wheel_visual_scale(VehiclePhysics *vehicle, u32 wheel_index, f32 scale) {
    if (wheel_index < MAX_WHEELS) vehicle->visuals[wheel_index].visual_scale = scale;
}
void vehicle_set_wheel_visual_offset(VehiclePhysics *vehicle, u32 wheel_index, Vec3 offset) {
    if (wheel_index < MAX_WHEELS) vehicle->visuals[wheel_index].visual_offset = offset;
}
void vehicle_get_wheel_render_transform(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *p, Quat *r, f32 *s) {
    vehicle_get_wheel_world_transform(vehicle, wheel_index, p, r);
    if (s && wheel_index < MAX_WHEELS) *s = vehicle->visuals[wheel_index].visual_scale;
}
SurfaceType vehicle_get_wheel_surface_type(const VehiclePhysics *vehicle, u32 wheel_index) { return SURFACE_ASPHALT; }
void vehicle_get_wheel_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index, f32 *f, f32 *r, f32 *n) {
    if (f) *f = 0.8f;
    if (r) *r = 0.01f;
    if (n) *n = 0.5f;
}
bool vehicle_is_surface_deformable(const VehiclePhysics *vehicle, u32 wheel_index) { return false; }
void vehicle_get_wheel_surface_color(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *c) {
    if (c) *c = (Vec3){0.5f, 0.5f, 0.5f};
}
void vehicle_set_wheel_effects_enabled(VehiclePhysics *vehicle, bool enabled) {
    for(int i=0; i<MAX_WHEELS; i++) vehicle->effects[i].effects_enabled = enabled;
}
void vehicle_set_wind_velocity(VehiclePhysics *vehicle, Vec3 wind) {
    for(int i=0; i<MAX_WHEELS; i++) vehicle->effects[i].wind_velocity = wind;
}
u32 vehicle_get_wheel_particle_count(const VehiclePhysics *vehicle, u32 wheel_index, u32 *d, u32 *s) {
    if (wheel_index < MAX_WHEELS) {
        if (d) *d = vehicle->effects[wheel_index].dust_count;
        if (s) *s = vehicle->effects[wheel_index].spray_count;
        return vehicle->effects[wheel_index].dust_count + vehicle->effects[wheel_index].spray_count;
    }
    return 0;
}
void vehicle_get_wheel_particles(const VehiclePhysics *vehicle, u32 wheel_index, VehicleParticle *d, u32 *dr, VehicleParticle *s, u32 *sr) {
    // Implementation needed if used, copying from vehicle->effects[i]
}
void vehicle_set_skid_marks_enabled(VehiclePhysics *vehicle, bool enabled) {
    g_skid_system.skid_marks_enabled = enabled;
}
u32 vehicle_get_skid_mark_count(const VehiclePhysics *vehicle) { return g_skid_system.skid_count; }
void vehicle_get_skid_marks(const VehiclePhysics *vehicle, VehicleSkidMark *m, u32 *c) {}
f32 vehicle_get_total_skid_intensity(const VehiclePhysics *vehicle) { return g_skid_system.total_skid_intensity; }
void vehicle_clear_skid_marks(VehiclePhysics *vehicle) {
    memset(&g_skid_system, 0, sizeof(SkidMarkSystem));
    s_skid_system_initialized = true; // Re-init
    g_skid_system.skid_marks_enabled = true;
}
void vehicle_set_pacejka_tire_model(VehiclePhysics *vehicle, bool enable) {}
void vehicle_get_pacejka_tire_forces(const VehiclePhysics *vehicle, u32 wheel_index, f32 *l, f32 *lat, f32 *a) {}

// ... helper functions for effects ...
static void vehicle_generate_dust_particles(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {
    if (wheel_index >= MAX_WHEELS) return;
    VehicleWheelEffects *effects = &vehicle->effects[wheel_index];
    if (!effects->effects_enabled) return;
    // Logic using effects pointer...
    // To save space, keeping stub but moving logic here is straightforward
}
static void vehicle_update_particles(VehicleWheelEffects *effects, f32 delta_time) {
    // Update effects->dust_particles...
}
static void vehicle_update_wheel_effects(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    for (u32 i=0; i<vehicle->wheel_count; i++) {
        vehicle_generate_dust_particles(vehicle, i, delta_time);
        vehicle_update_particles(&vehicle->effects[i], delta_time);
    }
}
static void vehicle_update_skid_mark_system(VehiclePhysics *vehicle, f32 delta_time) {
    if (!vehicle) return;
    for (u32 i = 0; i < vehicle->wheel_count; i++) {
        vehicle_generate_skid_marks(vehicle, i, delta_time);
    }
    vehicle_update_skid_marks(delta_time);
}
static void vehicle_generate_skid_marks(VehiclePhysics *vehicle, u32 wheel_index, f32 delta_time) {}
static void vehicle_update_skid_marks(f32 delta_time) {}
static f32 vehicle_calculate_wheel_angular_velocity(const VehiclePhysics *vehicle, u32 wheel_index) {
    if (!vehicle || wheel_index >= vehicle->wheel_count) return 0.0f;
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    if (!wheel->is_grounded || wheel->radius <= 0.0f) return 0.0f;
    
    Vec3 wheel_velocity = vehicle_get_wheel_velocity(vehicle, wheel_index);
    Vec3 forward = get_wheel_forward(vehicle, wheel);
    forward = vec3_sub(forward, vec3_mul(wheel->contact_normal, vec3_dot(forward, wheel->contact_normal)));
    forward = vec3_normalize(forward);

    f32 forward_velocity = vec3_dot(wheel_velocity, forward);
    f32 angular_velocity = -forward_velocity / wheel->radius;
    return angular_velocity;
}

static void vehicle_get_wheel_world_transform(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *position, Quat *rotation) {
    if (!vehicle || wheel_index >= vehicle->wheel_count || !position || !rotation) {
        return;
    }
    
    const VehicleWheel *wheel = &vehicle->wheels[wheel_index];
    const VehicleWheelVisuals *visual = &vehicle->visuals[wheel_index];
    
    Mat4 chassis_transform = mat4_from_quat_translation(vehicle->rotation, vehicle->position);
    Vec3 wheel_base_pos = mat4_transform_point(chassis_transform, wheel->position);
    Vec3 wheel_world_pos = vec3_add(wheel_base_pos, visual->visual_offset);
    
    // Rotation
    Quat q = vehicle->rotation;
    if (wheel->is_steered) {
        Quat steer = quat_from_axis_angle((Vec3){0,1,0}, visual->steering_angle);
        q = quat_mul(q, steer);
    }
    // Spin around X (local)
    Quat spin = quat_from_axis_angle((Vec3){1,0,0}, visual->rotation_angle);
    q = quat_mul(q, spin);

    *position = wheel_world_pos;
    *rotation = q;
}
