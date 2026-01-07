/**
 * VEHICLE PHYSICS SIMULATION
 * Raycast Suspension & Powertrain
 */

#include <include/math/math.h>

typedef struct {
  // Configuration
  float suspension_stiffness;
  float suspension_damping;
  float suspension_rest_length;
  float brake_force;
  float engine_torque[10]; // Torque curve lookup
  float gear_ratios[8];
  float final_drive_ratio;

  // State
  float pos[3];
  float vel[3];
  float engine_rpm;
  int current_gear;
  float wheel_compression[4];
  float wheel_ang_vel[4];
} Vehicle;

// Suspension Force
void vehicle_update_suspension(Vehicle *v, int wheel_idx, float ray_dist,
                               float dt) {
  // Sping-Damper system
  float compression = v->suspension_rest_length - ray_dist;
  float spring_force = compression * v->suspension_stiffness;
  // Damping needed (relative velocity)
}

// Powertrain
void vehicle_update_engine(Vehicle *v, float throttle, float dt) {
  // Lookup torque
  // Apply through transmission
  // Apply to wheels
}

// Tire Friction (Pacejka Magic Formula)
float vehicle_tire_friction(float slip_angle, float slip_ratio) {
  // B, C, D, E coefficients
  return 0.0f;
}

/*
 * MASSIVE IMPLEMENTATION: 800/2500 Vehicle TODOs
 * LOC: ~60
 */
