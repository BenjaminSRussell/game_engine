/**
 * =================================================================================================
 *                              VEHICLE PHYSICS SYSTEM
 *                               Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 *
 * Complete vehicle physics with wheeled, tracked, hovercraft, and boat support.
 *
 * =================================================================================================
 */

#ifndef VEHICLE_PHYSICS_H
#define VEHICLE_PHYSICS_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    VEHICLE TYPES
 * =================================================================================================
 */

typedef enum VehicleType {
  VEHICLE_CAR,
  VEHICLE_MOTORCYCLE,
  VEHICLE_TRUCK,
  VEHICLE_TANK,
  VEHICLE_HELICOPTER,
  VEHICLE_PLANE,
  VEHICLE_BOAT,
  VEHICLE_HOVERCRAFT,
  VEHICLE_MECH,
} VehicleType;

/* =================================================================================================
 *                                    WHEEL SYSTEM
 * =================================================================================================
 */

typedef struct WheelConfig {
  float radius;
  float width;
  float mass;
  float suspension_travel;
  float suspension_spring;
  float suspension_damper;
  float friction_slip;
  float friction_force;
  bool is_drive_wheel;
  bool is_steer_wheel;
  bool is_brake_wheel;
  float steer_angle_max;
  int32_t bone_index;
  float offset[3];
} WheelConfig;

typedef struct WheelState {
  float suspension_length;
  float suspension_velocity;
  float rotation_angle;
  float rotation_speed;
  float steer_angle;
  float slip_ratio;
  float slip_angle;
  bool is_grounded;
  float contact_point[3];
  float contact_normal[3];
  float ground_velocity[3];
  uint32_t surface_type;
} WheelState;

// TODO(AGENT_TEMPLATE_2): Implement wheel raycast [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement wheel suspension force [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement wheel friction calculation [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement wheel slip ratio [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement wheel slip angle [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement Pacejka tire model [Difficulty: 8]
// TODO(AGENT_TEMPLATE_2): Implement wheel visual rotation [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement wheel surface detection [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement wheel dust/spray effects [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement wheel skid marks [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement wheel audio [Difficulty: 5]

/* =================================================================================================
 *                                    ENGINE SYSTEM
 * =================================================================================================
 */

typedef struct EngineConfig {
  float max_rpm;
  float idle_rpm;
  float torque_curve[16];
  uint32_t torque_point_count;
  float inertia;
  float friction;
  float brake_torque;
} EngineConfig;

typedef struct GearboxConfig {
  float gear_ratios[8];
  uint32_t gear_count;
  float final_ratio;
  float shift_time;
  bool is_automatic;
  float auto_shift_rpm_up;
  float auto_shift_rpm_down;
} GearboxConfig;

typedef struct EngineState {
  float rpm;
  float throttle;
  float clutch;
  int32_t current_gear;
  bool is_shifting;
  float shift_timer;
  float angular_velocity;
} EngineState;

// TODO(AGENT_TEMPLATE_2): Implement engine torque calculation [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement engine RPM update [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement clutch simulation [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement automatic gear shifting [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement manual gear shifting [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement engine braking [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement engine audio pitch [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement turbo/boost [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement nitro/nos [Difficulty: 4]

/* =================================================================================================
 *                                    VEHICLE DEFINITION
 * =================================================================================================
 */

typedef struct VehicleDefinition {
  uint32_t id;
  char name[64];
  VehicleType type;

  // Physics
  float mass;
  float center_of_mass[3];
  float inertia_tensor[3];
  float drag_coefficient;
  float downforce_coefficient;

  // Wheels
  WheelConfig wheels[8];
  uint32_t wheel_count;

  // Powertrain
  EngineConfig engine;
  GearboxConfig gearbox;
  float drivetrain_efficiency;
  bool is_awd;
  float awd_front_bias;

  // Controls
  float max_steer_angle;
  float steer_speed;
  float brake_force;
  float handbrake_force;

  // Damage
  float health_max;
  float collision_damage_threshold;

  // Visuals
  char mesh_path[128];
  uint32_t damage_states;

  // Audio
  char engine_sound[128];
  char horn_sound[128];
  char collision_sound[128];
} VehicleDefinition;

// TODO(AGENT_TEMPLATE_2): Implement vehicle_definition_load [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_definition_validate [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_definition_create_preset
// [Difficulty: 5]

/* =================================================================================================
 *                                    VEHICLE INSTANCE
 * =================================================================================================
 */

typedef struct VehicleInstance {
  uint32_t instance_id;
  uint32_t definition_id;

  // Transform
  float position[3];
  float rotation[4];
  float velocity[3];
  float angular_velocity[3];

  // Wheels
  WheelState *wheel_states;

  // Engine
  EngineState engine_state;

  // Input
  float input_throttle;
  float input_brake;
  float input_steer;
  bool input_handbrake;
  bool input_boost;

  // State
  float health;
  bool is_destroyed;
  bool is_flipped;
  float flip_timer;

  // Occupants
  uint32_t driver_id;
  uint32_t passenger_ids[8];
  uint32_t passenger_count;

  // Effects
  bool engine_running;
  bool headlights_on;
  float exhaust_rate;
} VehicleInstance;

// TODO(AGENT_TEMPLATE_2): Implement vehicle_spawn [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_despawn [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_update_physics [Difficulty: 7]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_apply_forces [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_update_wheels [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_update_engine [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_apply_input [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_enter [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_exit [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_damage [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_repair [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_flip [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_unflip [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_honk [Difficulty: 2]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_toggle_lights [Difficulty: 2]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_serialize [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_deserialize [Difficulty: 5]

/* =================================================================================================
 *                                    AIRCRAFT PHYSICS
 * =================================================================================================
 */

typedef struct AircraftConfig {
  float wing_area;
  float wing_span;
  float lift_coefficient;
  float drag_coefficient;
  float thrust_max;
  float pitch_authority;
  float roll_authority;
  float yaw_authority;
  float stall_angle;
  float max_altitude;
} AircraftConfig;

// TODO(AGENT_TEMPLATE_2): Implement aircraft lift calculation [Difficulty: 7]
// TODO(AGENT_TEMPLATE_2): Implement aircraft drag calculation [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement aircraft thrust [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement aircraft control surfaces [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement aircraft stall [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement helicopter rotor physics [Difficulty: 8]
// TODO(AGENT_TEMPLATE_2): Implement helicopter collective/cyclic [Difficulty:
// 7]

/* =================================================================================================
 *                                    BOAT PHYSICS
 * =================================================================================================
 */

typedef struct BoatConfig {
  float hull_length;
  float hull_width;
  float displacement;
  float buoyancy_points[8][3];
  uint32_t buoyancy_point_count;
  float drag_linear;
  float drag_angular;
  float propeller_force;
  float rudder_force;
} BoatConfig;

// TODO(AGENT_TEMPLATE_2): Implement boat buoyancy [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement boat wave response [Difficulty: 7]
// TODO(AGENT_TEMPLATE_2): Implement boat propeller [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement boat rudder [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement boat wake effects [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement boat water entry splash [Difficulty: 5]

/* =================================================================================================
 *                                    VEHICLE MANAGER
 * =================================================================================================
 */

typedef struct VehicleManager {
  VehicleDefinition *definitions;
  uint32_t definition_count;

  VehicleInstance *instances;
  uint32_t instance_count;
  uint32_t instance_capacity;

  float physics_timestep;
  uint32_t physics_substeps;
} VehicleManager;

// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_init [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_shutdown [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_update [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_spawn [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_despawn [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_find_nearby [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement vehicle_manager_raycast [Difficulty: 5]

#endif // VEHICLE_PHYSICS_H
