// Vehicle physics system
#ifndef VEHICLE_PHYSICS_H
#define VEHICLE_PHYSICS_H

#include "include/common.h"
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_WHEELS 8
#define MAX_DUST_PARTICLES 128
#define MAX_SPRAY_PARTICLES 64

// --- Enums & Structs ---

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
    Vec3 position;
    Vec3 velocity;
    Vec3 color;
    f32 size;
    f32 lifetime;
    f32 max_lifetime;
    bool active;
    u32 type; // 0 = dust, 1 = spray, 2 = mud
} VehicleParticle;

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
} VehicleSkidMark;

typedef struct {
    f32 rotation_angle;
    f32 angular_velocity;
    f32 target_velocity;
    f32 steering_angle;
    f32 target_steering;
    f32 visual_scale;
    Vec3 visual_offset;
    bool is_rotating;
    f32 rotation_damping;
} VehicleWheelVisuals;

typedef struct {
    VehicleParticle dust_particles[MAX_DUST_PARTICLES];
    VehicleParticle spray_particles[MAX_SPRAY_PARTICLES];
    u32 dust_count;
    u32 spray_count;
    f32 emission_timer;
    f32 spray_intensity;
    Vec3 wind_velocity;
    bool effects_enabled;
} VehicleWheelEffects;

// Callback for raycasting against the physics world
// Returns true if hit, populates output parameters
typedef bool (*VehicleRaycastCallback)(void *user_data, Vec3 origin, Vec3 direction, f32 max_distance, Vec3 *hit_point, Vec3 *hit_normal, f32 *hit_distance);

typedef struct {
  Vec3 position; // Local space offset from chassis
  f32 radius;
  f32 width;
  f32 suspension_length;
  f32 suspension_stiffness;
  f32 suspension_damping;
  f32 friction_slip;

  f32 compression;    // Current suspension compression
  f32 wheel_rotation; // Radians
  f32 steering_angle; // Current steering

  bool is_driven;  // Receives engine torque
  bool is_steered; // Affected by steering input

  Vec3 contact_point;
  Vec3 contact_normal;
  bool is_grounded;

} VehicleWheel;

typedef struct {
  // Chassis
  f32 mass;
  Vec3 center_of_mass;
  Mat4 inertia_tensor;

  Vec3 position;
  Quat rotation;
  Vec3 velocity;
  Vec3 angular_velocity;

  // Wheels
  VehicleWheel wheels[MAX_WHEELS];
  u32 wheel_count;

  // Engine
  f32 engine_torque;
  f32 max_engine_torque;
  f32 engine_rpm;
  f32 max_rpm;

  // Transmission
  u32 current_gear;
  u32 gear_count;
  f32 gear_ratios[8];
  f32 differential_ratio;

  // Control
  f32 throttle;  // 0-1
  f32 brake;     // 0-1
  f32 steering;  // -1 to 1
  f32 handbrake; // 0-1

  // Physics config
  f32 drag_coefficient;
  f32 downforce_coefficient;
  f32 slip_angle_limit;

  // Callbacks
  VehicleRaycastCallback raycast_cb;
  void *raycast_user_data;

  // Instance State (Visuals & Effects)
  VehicleWheelVisuals visuals[MAX_WHEELS];
  VehicleWheelEffects effects[MAX_WHEELS];

} VehiclePhysics;

#ifdef __cplusplus
extern "C" {
#endif

VehiclePhysics *vehicle_create(u32 wheel_count);
void vehicle_destroy(VehiclePhysics *vehicle);

void vehicle_update(VehiclePhysics *vehicle, f32 delta_time);
void vehicle_set_input(VehiclePhysics *vehicle, f32 throttle, f32 brake,
                       f32 steering, f32 handbrake);

void vehicle_add_wheel(VehiclePhysics *vehicle, Vec3 offset, f32 radius,
                       bool driven, bool steered);

// --- Extended API ---

// Raycast Setup
void vehicle_set_raycast_callback(VehiclePhysics *vehicle, VehicleRaycastCallback callback, void *user_data);

// Wheel Info
bool vehicle_get_wheel_contact(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *contact_point, Vec3 *contact_normal, bool *is_grounded);
f32 vehicle_get_wheel_compression(const VehiclePhysics *vehicle, u32 wheel_index);
bool vehicle_perform_wheel_raycast(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *hit_point, Vec3 *hit_normal, f32 *hit_distance);

// Suspension & Forces
f32 vehicle_get_suspension_force_magnitude(const VehiclePhysics *vehicle, u32 wheel_index);
void vehicle_get_suspension_force_vector(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *force);
f32 vehicle_get_total_suspension_force(const VehiclePhysics *vehicle);

// Friction & Slip
f32 vehicle_get_wheel_slip_ratio(const VehiclePhysics *vehicle, u32 wheel_index);
f32 vehicle_get_wheel_slip_angle(const VehiclePhysics *vehicle, u32 wheel_index);
void vehicle_get_wheel_friction_forces(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *lateral, Vec3 *longitudinal, Vec3 *total);
bool vehicle_is_wheel_slipping(const VehiclePhysics *vehicle, u32 wheel_index);

// Pacejka
void vehicle_set_pacejka_tire_model(VehiclePhysics *vehicle, bool enable);
void vehicle_get_pacejka_tire_forces(const VehiclePhysics *vehicle, u32 wheel_index, f32 *longitudinal, f32 *lateral, f32 *aligning_moment);

// Visuals
void vehicle_get_wheel_visual_state(const VehiclePhysics *vehicle, u32 wheel_index, f32 *rotation_angle, f32 *steering_angle, bool *is_rotating);
void vehicle_set_wheel_visual_scale(VehiclePhysics *vehicle, u32 wheel_index, f32 scale);
void vehicle_set_wheel_visual_offset(VehiclePhysics *vehicle, u32 wheel_index, Vec3 offset);
void vehicle_get_wheel_render_transform(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *position, Quat *rotation, f32 *scale);

// Surface
SurfaceType vehicle_get_wheel_surface_type(const VehiclePhysics *vehicle, u32 wheel_index);
void vehicle_get_wheel_surface_properties(const VehiclePhysics *vehicle, u32 wheel_index, f32 *friction, f32 *rolling_resistance, f32 *noise_factor);
bool vehicle_is_surface_deformable(const VehiclePhysics *vehicle, u32 wheel_index);
void vehicle_get_wheel_surface_color(const VehiclePhysics *vehicle, u32 wheel_index, Vec3 *color_tint);

// Effects
void vehicle_set_wheel_effects_enabled(VehiclePhysics *vehicle, bool enabled);
void vehicle_set_wind_velocity(VehiclePhysics *vehicle, Vec3 wind);
u32 vehicle_get_wheel_particle_count(const VehiclePhysics *vehicle, u32 wheel_index, u32 *dust_count, u32 *spray_count);
void vehicle_get_wheel_particles(const VehiclePhysics *vehicle, u32 wheel_index, VehicleParticle *dust_particles, u32 *dust_returned, VehicleParticle *spray_particles, u32 *spray_returned);

// Skid Marks
void vehicle_set_skid_marks_enabled(VehiclePhysics *vehicle, bool enabled);
u32 vehicle_get_skid_mark_count(const VehiclePhysics *vehicle);
void vehicle_get_skid_marks(const VehiclePhysics *vehicle, VehicleSkidMark *marks, u32 *count);
f32 vehicle_get_total_skid_intensity(const VehiclePhysics *vehicle);
void vehicle_clear_skid_marks(VehiclePhysics *vehicle);

#ifdef __cplusplus
}
#endif

#endif
