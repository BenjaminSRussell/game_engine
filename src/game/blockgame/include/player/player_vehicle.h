// Player Vehicle System Integration.
// Roadmap: docs/PLAYER_VEHICLE_ROADMAP.md.

#ifndef PLAYER_VEHICLE_H
#define PLAYER_VEHICLE_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

// Forward declare PlayerSystem to avoid including full player header here
typedef struct PlayerSystem PlayerSystem;

// Vehicle types
typedef enum {
  VEHICLE_TYPE_NONE = 0,
  VEHICLE_TYPE_BOAT,
  VEHICLE_TYPE_MINECART,
  VEHICLE_TYPE_HORSE,
  VEHICLE_TYPE_PIG,
  VEHICLE_TYPE_LLAMA,
  VEHICLE_TYPE_CAMEL,
  VEHICLE_TYPE_STRIDER,
  VEHICLE_TYPE_CHAIR,
  VEHICLE_TYPE_BED,
  VEHICLE_TYPE_COUNT
} VehicleType;

// Vehicle state
typedef struct {
  VehicleType type;
  EntityID vehicle_entity;
  f32 mount_timer;
  f32 dismount_timer;
  bool is_driver;
  bool can_control;
  Vec3 local_offset;     // Player offset relative to vehicle
  Vec3 seat_position;    // Seat position on vehicle
  f32 control_influence; // How much player can control vehicle (0-1)
} VehicleState;

// Vehicle component (attached to vehicle entities)
typedef struct {
  VehicleType type;
  EntityID rider; // Current rider (0 if none)
  EntityID owner; // Owner of vehicle (for persistent vehicles)
  f32 health;     // Vehicle health/durability
  f32 max_health;
  f32 speed;          // Base movement speed
  f32 turn_speed;     // Turning speed
  f32 acceleration;   // Acceleration rate
  bool can_fly;       // Can this vehicle fly?
  bool can_swim;      // Can this vehicle swim?
  bool requires_fuel; // Does this vehicle consume fuel?
  f32 fuel_amount;    // Current fuel (if applicable)
  f32 max_fuel;
  u32 inventory_slots; // Storage slots for vehicles with inventory
  RigidBody* physics_body;
} VehicleComponent;

// Initialize vehicle system
void player_vehicle_init(VehicleState *vehicle);

// Vehicle mounting/dismounting
bool player_mount_vehicle(PlayerSystem *system, EntityID vehicle_entity,
                          VehicleType type);
bool player_dismount_vehicle(PlayerSystem *system);
bool player_can_mount_vehicle(PlayerSystem *system, EntityID vehicle_entity);

// Vehicle control
void player_update_vehicle_control(PlayerSystem *system, f32 delta_time);
void player_vehicle_apply_input(PlayerSystem *system, Vec3 move_input,
                                bool jump, bool brake);

// Vehicle queries
bool player_is_in_vehicle(const PlayerSystem *system);
VehicleType player_get_vehicle_type(const PlayerSystem *system);
EntityID player_get_vehicle_entity(const PlayerSystem *system);

// Vehicle damage and repair
void player_vehicle_damage(PlayerSystem *system, f32 damage);
void player_vehicle_damage_typed(PlayerSystem *system, f32 damage, VehicleDamageType damage_type);
void player_vehicle_repair(PlayerSystem *system, f32 repair_amount);
bool player_vehicle_is_destroyed(const PlayerSystem *system);

// Vehicle fuel system
void player_vehicle_update_fuel(PlayerSystem *system, f32 delta_time);
bool player_vehicle_refuel(PlayerSystem *system, f32 fuel_amount);
f32 player_vehicle_get_fuel_percentage(const PlayerSystem *system);

// Vehicle damage types (for external use)
typedef enum {
    VEHICLE_DAMAGE_COLLISION = 0,
    VEHICLE_DAMAGE_FALL,
    VEHICLE_DAMAGE_EXPLOSION,
    VEHICLE_DAMAGE_FIRE,
    VEHICLE_DAMAGE_WATER,
    VEHICLE_DAMAGE_COUNT
} VehicleDamageType;

#endif // PLAYER_VEHICLE_H
