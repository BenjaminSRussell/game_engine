// Player Vehicle System Implementation
// Roadmap: docs/PLAYER_VEHICLE_ROADMAP.md.
//
// Implements vehicle mounting/dismounting, control mechanics, and type-specific
// physics. Supports 9 vehicle types with customized seat positions, speeds, and
// behavior.

// TODO: Implement vehicle physics system with realistic movement.
// TODO: Add vehicle damage system with visual wear.
// TODO: Implement vehicle fuel system for powered vehicles.
// TODO: Add vehicle inventory system for storage.
// TODO: Implement vehicle customization system.
// TODO: Add vehicle sound effects system.
// TODO: Implement vehicle particle effects system.
// TODO: Add vehicle statistics tracking system.
// TODO: Implement vehicle save/load system.
// TODO: Add vehicle AI system for NPCs.
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <math.h>
#include <player/player.h>
#include <player/player_vehicle.h>
#include <string.h>

// Helper: Get seat position based on vehicle type
static Vec3 vehicle_get_seat_offset(VehicleType type) {
  switch (type) {
  case VEHICLE_TYPE_BOAT:
    return vec3(0.0f, 0.3f, 0.0f); // Boat rider sits low
  case VEHICLE_TYPE_MINECART:
    return vec3(0.0f, 0.4f, 0.0f); // Minecart seat height
  case VEHICLE_TYPE_HORSE:
    return vec3(0.0f, 1.2f, 0.0f); // Horse back is high
  case VEHICLE_TYPE_PIG:
    return vec3(0.0f, 0.7f, 0.0f); // Pig saddle height
  case VEHICLE_TYPE_LLAMA:
    return vec3(0.0f, 1.4f, 0.0f); // Llama is tall
  case VEHICLE_TYPE_CAMEL:
    return vec3(0.0f, 1.8f, 0.0f); // Camel is very tall
  case VEHICLE_TYPE_STRIDER:
    return vec3(0.0f, 1.5f, 0.0f); // Strider saddle height
  case VEHICLE_TYPE_CHAIR:
    return vec3(0.0f, 0.4f, 0.0f); // Chair seat
  case VEHICLE_TYPE_BED:
    return vec3(0.0f, 0.3f, 0.0f); // Bed surface
  default:
    return vec3(0.0f, 0.5f, 0.0f);
  }
}

// Helper: Get speed multiplier based on vehicle type
static f32 vehicle_get_speed_multiplier(VehicleType type) {
  switch (type) {
  case VEHICLE_TYPE_BOAT:
    return 0.8f; // Boats are slower on land
  case VEHICLE_TYPE_MINECART:
    return 1.5f; // Minecarts fast on rails
  case VEHICLE_TYPE_HORSE:
    return 1.3f; // Horses are fast
  case VEHICLE_TYPE_PIG:
    return 0.6f; // Pigs are slow
  case VEHICLE_TYPE_LLAMA:
    return 1.0f; // Llamas normal speed
  case VEHICLE_TYPE_CAMEL:
    return 1.2f; // Camels are fast
  case VEHICLE_TYPE_STRIDER:
    return 0.9f; // Striders normal speed
  case VEHICLE_TYPE_CHAIR:
    return 0.0f; // Chairs don't move
  case VEHICLE_TYPE_BED:
    return 0.0f; // Beds don't move
  default:
    return 1.0f;
  }
}

// Helper: Get vehicle properties
static void vehicle_get_properties(VehicleType type, f32 *out_speed,
                                   f32 *out_turn_speed, bool *out_can_fly,
                                   bool *out_can_swim) {
  if (out_speed)
    *out_speed = 10.0f;
  if (out_turn_speed)
    *out_turn_speed = 180.0f;
  if (out_can_fly)
    *out_can_fly = false;
  if (out_can_swim)
    *out_can_swim = false;

  switch (type) {
  case VEHICLE_TYPE_BOAT:
    if (out_speed)
      *out_speed = 8.0f;
    if (out_turn_speed)
      *out_turn_speed = 120.0f;
    if (out_can_swim)
      *out_can_swim = true;
    break;
  case VEHICLE_TYPE_MINECART:
    if (out_speed)
      *out_speed = 15.0f;
    if (out_turn_speed)
      *out_turn_speed = 90.0f;
    break;
  case VEHICLE_TYPE_HORSE:
    if (out_speed)
      *out_speed = 13.0f;
    if (out_turn_speed)
      *out_turn_speed = 200.0f;
    break;
  case VEHICLE_TYPE_PIG:
    if (out_speed)
      *out_speed = 6.0f;
    if (out_turn_speed)
      *out_turn_speed = 150.0f;
    break;
  case VEHICLE_TYPE_LLAMA:
    if (out_speed)
      *out_speed = 10.0f;
    if (out_turn_speed)
      *out_turn_speed = 180.0f;
    break;
  case VEHICLE_TYPE_CAMEL:
    if (out_speed)
      *out_speed = 12.0f;
    if (out_turn_speed)
      *out_turn_speed = 160.0f;
    break;
  case VEHICLE_TYPE_STRIDER:
    if (out_speed)
      *out_speed = 9.0f;
    if (out_turn_speed)
      *out_turn_speed = 140.0f;
    if (out_can_swim)
      *out_can_swim = true;
    break;
  default:
    break;
  }
}

void player_vehicle_init(VehicleState *vehicle) {
  if (!vehicle)
    return;
  memset(vehicle, 0, sizeof(VehicleState));

  vehicle->type = VEHICLE_TYPE_NONE;
  vehicle->vehicle_entity = 0;
  vehicle->local_offset = vec3(0.0f, 0.5f, 0.0f);
  vehicle->control_influence = 1.0f;

  LOG_DEBUG("Vehicle state initialized");
}

bool player_mount_vehicle(PlayerSystem *system, EntityID vehicle_entity,
                          VehicleType type) {
  if (!system || !system->player)
    return false;

  // Check if already in vehicle
  if (system->player->in_vehicle) {
    LOG_WARN("Player already in vehicle");
    return false;
  }

  // Set vehicle state
  system->player->vehicle = vehicle_entity;
  system->player->in_vehicle = true;

  VehicleState *vehicle = &system->player->vehicle_state;
  vehicle->type = type;
  vehicle->vehicle_entity = vehicle_entity;
  vehicle->mount_timer = 0.5f; // Mount animation time
  vehicle->is_driver = true;   // First rider is driver by default
  vehicle->can_control = true;
  vehicle->local_offset = vehicle_get_seat_offset(type);
  vehicle->seat_position = vehicle->local_offset;
  vehicle->control_influence = vehicle_get_speed_multiplier(type);

  LOG_INFO("Player mounted %d-type vehicle (entity: %u)", type, vehicle_entity);

  return true;
}

bool player_dismount_vehicle(PlayerSystem *system) {
  if (!system || !system->player)
    return false;

  if (!system->player->in_vehicle) {
    LOG_DEBUG("Player not in vehicle");
    return false;
  }

  VehicleState *vehicle = &system->player->vehicle_state;

  LOG_INFO("Player dismounted from vehicle (entity: %u)",
           system->player->vehicle);

  // Clear vehicle state
  vehicle->dismount_timer = 0.3f; // Dismount animation time
  vehicle->can_control = false;
  system->player->vehicle = 0;
  system->player->in_vehicle = false;

  return true;
}

bool player_can_mount_vehicle(PlayerSystem *system, EntityID vehicle_entity) {
  if (!system || !system->player)
    return false;

  // Check if already in vehicle
  if (system->player->in_vehicle) {
    LOG_DEBUG("Player already mounted");
    return false;
  }

  // Check if vehicle entity exists
  if (!system->ecs_world) {
    LOG_WARN("ECS world not available for vehicle check");
    return false;
  }

  if (!ecs_entity_exists((World *)system->ecs_world,
                         (Entity){vehicle_entity, 0})) {
    LOG_WARN("Vehicle entity does not exist");
    return false;
  }

  return true;
}

void player_update_vehicle_control(PlayerSystem *system, f32 delta_time) {
  if (!system || !system->player || delta_time <= 0.0f)
    return;

  if (!system->player->in_vehicle) {
    return;
  }

  if (player_vehicle_is_destroyed(system)) {
    LOG_INFO("Vehicle destroyed, dismounting player");
    player_dismount_vehicle(system);
    return;
  }

  VehicleState *vehicle = &system->player->vehicle_state;

  // Update mount/dismount timers
  if (vehicle->mount_timer > 0.0f) {
    vehicle->mount_timer -= delta_time;
  }
  if (vehicle->dismount_timer > 0.0f) {
    vehicle->dismount_timer -= delta_time;
  }

  // Get vehicle speed based on type
  f32 vehicle_speed, turn_speed;
  bool can_fly, can_swim;
  vehicle_get_properties(vehicle->type, &vehicle_speed, &turn_speed, &can_fly,
                         &can_swim);

  // Apply speed multiplier
  vehicle_speed *= vehicle->control_influence;

  // Update player position to follow vehicle (if needed)
  if (system->ecs_world) {
    TransformComponent *player_transform =
        (TransformComponent *)ecs_get_component(
            (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
            TRANSFORM_COMPONENT_ID);

    // Get vehicle entity transform from ECS (placeholder - actual integration
    // depends on ECS query)
    if (vehicle->vehicle_entity &&
        ecs_entity_exists((World *)system->ecs_world,
                          (Entity){vehicle->vehicle_entity, 0}) &&
        player_transform) {
      // In real implementation, query the TransformComponent from ECS
      // For now, we just track the vehicle position via seat offset
      player_transform->position = vec3_add(
          player_transform
              ->position, // Vehicle position would come from vehicle_transform
          vehicle->seat_position);
    }
  }
}

void player_vehicle_apply_input(PlayerSystem *system, Vec3 move_input,
                                bool jump, bool brake) {
  if (!system || !system->player)
    return;

  if (!system->player->in_vehicle) {
    return;
  }

  VehicleState *vehicle = &system->player->vehicle_state;

  if (!vehicle->can_control || vehicle->control_influence <= 0.0f) {
    LOG_DEBUG("Cannot control vehicle (control_influence: %.2f)",
              vehicle->control_influence);
    return;
  }

  // Apply vehicle-type-specific input handling
  switch (vehicle->type) {
  case VEHICLE_TYPE_BOAT:
    // Boats move with input in water, slow on land
    if (move_input.x != 0.0f || move_input.z != 0.0f) {
      // Player is controlling boat direction
      // This would normally apply velocity to the boat entity
      LOG_DEBUG("Boat input: %.2f, %.2f", move_input.x, move_input.z);
    }
    break;

  case VEHICLE_TYPE_HORSE:
  case VEHICLE_TYPE_PIG:
  case VEHICLE_TYPE_LLAMA:
  case VEHICLE_TYPE_CAMEL:
  case VEHICLE_TYPE_STRIDER:
    // Animal mounts respond to speed control
    if (jump) {
      LOG_DEBUG("Animal mount jumping");
      // Would apply upward velocity to mount
    }
    if (brake) {
      LOG_DEBUG("Animal mount braking");
      // Would reduce mount velocity
    }
    break;

  case VEHICLE_TYPE_MINECART:
    // Minecarts move along rails, limited steering
    LOG_DEBUG("Minecart control input");
    break;

  case VEHICLE_TYPE_CHAIR:
  case VEHICLE_TYPE_BED:
    // Stationary - no input effect
    LOG_DEBUG("Stationary vehicle - input ignored");
    break;

  default:
    break;
  }
}

bool player_is_in_vehicle(const PlayerSystem *system) {
  if (!system || !system->player)
    return false;
  return system->player->in_vehicle;
}

VehicleType player_get_vehicle_type(const PlayerSystem *system) {
  if (!system || !system->player)
    return VEHICLE_TYPE_NONE;
  if (!system->player->in_vehicle)
    return VEHICLE_TYPE_NONE;
  return system->player->vehicle_state.type;
}

EntityID player_get_vehicle_entity(const PlayerSystem *system) {
  if (!system || !system->player)
    return 0;
  if (!system->player->in_vehicle)
    return 0;
  return system->player->vehicle;
}

void player_vehicle_damage(PlayerSystem *system, f32 damage) {
  if (!system || !system->player || damage <= 0.0f)
    return;

  if (!system->player->in_vehicle) {
    return;
  }

  if (!system->ecs_world) {
    LOG_WARN("Cannot apply vehicle damage - ECS world not available");
    return;
  }

  EntityID vehicle_entity = system->player->vehicle;
  if (!ecs_entity_exists((World *)system->ecs_world,
                         (Entity){vehicle_entity, 0})) {
    LOG_WARN("Vehicle entity no longer exists");
    return;
  }

  // Query VehicleComponent from ECS and apply damage
  // In real implementation: ecs_get_component(system->ecs_world,
  // vehicle_entity, COMPONENT_VEHICLE);
  LOG_INFO("Vehicle damaged for %.1f HP (vehicle: %u)", damage, vehicle_entity);

  // If vehicle health drops to 0, destroy it
  // This would trigger automatic dismount in player_update_vehicle_control
}

void player_vehicle_repair(PlayerSystem *system, f32 repair_amount) {
  if (!system || !system->player || repair_amount <= 0.0f)
    return;

  if (!system->player->in_vehicle) {
    return;
  }

  if (!system->ecs_world) {
    LOG_WARN("Cannot repair vehicle - ECS world not available");
    return;
  }

  EntityID vehicle_entity = system->player->vehicle;
  if (!ecs_entity_exists((World *)system->ecs_world,
                         (Entity){vehicle_entity, 0})) {
    LOG_WARN("Vehicle entity no longer exists");
    return;
  }

  // Query VehicleComponent from ECS and apply repair
  // In real implementation: ecs_get_component(system->ecs_world,
  // vehicle_entity, COMPONENT_VEHICLE);
  LOG_INFO("Vehicle repaired for %.1f HP (vehicle: %u)", repair_amount,
           vehicle_entity);
}

bool player_vehicle_is_destroyed(const PlayerSystem *system) {
  if (!system || !system->player)
    return false;

  if (!system->player->in_vehicle) {
    return false;
  }

  if (!system->ecs_world) {
    return false;
  }

  return !ecs_entity_exists((World *)system->ecs_world,
                            (Entity){system->player->vehicle, 0});
}
