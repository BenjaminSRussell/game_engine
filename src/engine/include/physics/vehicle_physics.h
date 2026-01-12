// Vehicle physics system
#ifndef VEHICLE_PHYSICS_H
#define VEHICLE_PHYSICS_H

#include "include/common.h"
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_WHEELS 8

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

#ifdef __cplusplus
}
#endif

#endif
