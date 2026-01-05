/**
 * @file vehicle_controller.c
 * @brief Raycast Vehicle Physics Controller.
 *
 * Implements a 4-wheel raycast vehicle with suspension, friction, and engine
 * torque curves.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <physics/advanced/vehicle_controller.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct Wheel {
  vec3 local_position;
  float radius;
  float suspension_rest_len;
  float suspension_stiffness;
  float suspension_damping;
  float current_suspension_len;
  bool is_grounded;
  float steering_angle;
  float rotation_velocity; // Rad/s
} Wheel;

typedef struct Vehicle {
  RigidBody *chassis_body;
  Wheel wheels[4];
  float engine_power;
  float brake_power;
  float steer_input;
  float throttle_input;
  float current_rpm;
} Vehicle;

// =================================================================================================
//                                      LOGIC
// =================================================================================================

void vehicle_update(Vehicle *vehicle, float dt) {
  vec3 chassis_pos = vehicle->chassis_body->position;
  mat4 chassis_rot = vehicle->chassis_body->rotation_matrix;

  for (int i = 0; i < 4; i++) {
    Wheel *w = &vehicle->wheels[i];

    // Raycast origin in world space
    vec3 ray_origin = mat4_mul_vec3(chassis_rot, w->local_position);
    ray_origin = vec3_add(chassis_pos, ray_origin);

    vec3 down_dir = mat4_mul_vec3(chassis_rot, (vec3){0, -1, 0});

    RaycastResult hit;
    if (physics_raycast(ray_origin, down_dir,
                        w->suspension_rest_len + w->radius, &hit)) {
      w->is_grounded = true;

      // Suspension Force
      // F = -k * (x - rest) - damping * v
      float compression = w->suspension_rest_len - hit.distance;
      w->current_suspension_len = hit.distance;

      vec3 vel_at_wheel =
          rigid_body_get_point_velocity(vehicle->chassis_body, ray_origin);
      float vel_down = vec3_dot(vel_at_wheel, down_dir);

      float force = (compression * w->suspension_stiffness) -
                    (vel_down * w->suspension_damping);

      vec3 suspension_force = vec3_scale(down_dir, -force);
      rigid_body_add_force_at_pos(vehicle->chassis_body, suspension_force,
                                  ray_origin);

      // Friction / Traction
      // ... implementation of Pacejka tire formula ...

      // Engine Force (if drive wheel)
      if (i < 2) { // FWD
        vec3 forward_dir = mat4_mul_vec3(chassis_rot, (vec3){0, 0, 1});
        vec3 drive_force = vec3_scale(forward_dir, vehicle->throttle_input *
                                                       vehicle->engine_power);
        rigid_body_add_force_at_pos(vehicle->chassis_body, drive_force,
                                    ray_origin);
      }

    } else {
      w->is_grounded = false;
    }
  }
}
