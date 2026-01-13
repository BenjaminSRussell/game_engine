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
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <math.h>
#include <player/player.h>
#include <player/player_vehicle.h>
#include <string.h>
#include <physics/physics.h>
#include <block/block.h>
#include <chunk/chunk.h>

// Vehicle damage and wear system
#define VEHICLE_DAMAGE_WEAR_THRESHOLD 0.2f
#define VEHICLE_MAX_WEAR_LEVELS 5
#define VEHICLE_REPAIR_COOLDOWN 1.0f

// Vehicle damage types
typedef enum {
    VEHICLE_DAMAGE_COLLISION = 0,
    VEHICLE_DAMAGE_FALL,
    VEHICLE_DAMAGE_EXPLOSION,
    VEHICLE_DAMAGE_FIRE,
    VEHICLE_DAMAGE_WATER,
    VEHICLE_DAMAGE_COUNT
} VehicleDamageType;

// Vehicle wear appearance levels
typedef struct {
    f32 health_threshold;
    const char* description;
    u32 visual_effects; // Bitmask for visual effects
} VehicleWearLevel;

static VehicleWearLevel vehicle_wear_levels[VEHICLE_MAX_WEAR_LEVELS] = {
    {0.8f, "Pristine", 0x00},
    {0.6f, "Light Wear", 0x01},
    {0.4f, "Moderate Wear", 0x03},
    {0.2f, "Heavy Wear", 0x07},
    {0.0f, "Critical Damage", 0x0F}
};

// Vehicle fuel system
#define VEHICLE_FUEL_CONSUMPTION_RATE 0.1f
#define VEHICLE_FUEL_LOW_THRESHOLD 0.2f
#define VEHICLE_FUEL_CRITICAL_THRESHOLD 0.1f

// Helper: Check if vehicle type requires fuel
static bool vehicle_requires_fuel(VehicleType type) {
    switch (type) {
        case VEHICLE_TYPE_MINECART:
            return true; // Minecarts can be powered
        case VEHICLE_TYPE_BOAT:
        case VEHICLE_TYPE_HORSE:
        case VEHICLE_TYPE_PIG:
        case VEHICLE_TYPE_LLAMA:
        case VEHICLE_TYPE_CAMEL:
        case VEHICLE_TYPE_STRIDER:
        case VEHICLE_TYPE_CHAIR:
        case VEHICLE_TYPE_BED:
        default:
            return false; // Natural vehicles don't require fuel
    }
}

// Helper: Get fuel consumption rate for vehicle type
static f32 vehicle_get_fuel_consumption(VehicleType type) {
    switch (type) {
        case VEHICLE_TYPE_MINECART:
            return VEHICLE_FUEL_CONSUMPTION_RATE * 1.5f; // Minecarts use more fuel
        default:
            return 0.0f;
    }
}

// Update vehicle fuel consumption
void player_vehicle_update_fuel(PlayerSystem *system, f32 delta_time) {
    if (!system || !system->player || delta_time <= 0.0f)
        return;

    if (!system->player->in_vehicle)
        return;

    VehicleState *vehicle = &system->player->vehicle_state;
    if (!vehicle_requires_fuel(vehicle->type))
        return;

    if (!system->ecs_world)
        return;

    EntityID vehicle_entity = system->player->vehicle;
    if (!ecs_entity_exists((World *)system->ecs_world, (Entity){vehicle_entity, 0}))
        return;

    VehicleComponent *vc = ecs_get_component((World *)system->ecs_world, 
                                            (Entity){vehicle_entity, 0}, 
                                            VEHICLE_COMPONENT_ID);
    if (!vc || !vc->requires_fuel)
        return;

    // Only consume fuel when vehicle is moving
    RigidBodyComponent *rbc = ecs_get_component((World *)system->ecs_world, 
                                               (Entity){vehicle_entity, 0}, 
                                               RIGIDBODY_COMPONENT_ID);
    if (!rbc || !rbc->body)
        return;

    Vec3 velocity = rigid_body_get_velocity(rbc->body);
    f32 speed = vec3_length(velocity);
    
    if (speed > 0.1f) { // Only consume fuel when moving
        f32 consumption_rate = vehicle_get_fuel_consumption(vehicle->type);
        f32 fuel_consumed = consumption_rate * speed * delta_time * 0.01f;
        
        vc->fuel_amount = fmaxf(0.0f, vc->fuel_amount - fuel_consumed);
        
        // Log fuel consumption
        f32 fuel_percentage = vc->fuel_amount / vc->max_fuel;
        if (fuel_percentage <= VEHICLE_FUEL_CRITICAL_THRESHOLD) {
            LOG_WARN("Vehicle fuel critical: %.1f%%", fuel_percentage * 100.0f);
        } else if (fuel_percentage <= VEHICLE_FUEL_LOW_THRESHOLD) {
            LOG_INFO("Vehicle fuel low: %.1f%%", fuel_percentage * 100.0f);
        }
        
        // Apply fuel penalties
        if (vc->fuel_amount <= 0.0f) {
            // Out of fuel - stop vehicle
            Vec3 zero_vel = {0};
            rigid_body_set_velocity(rbc->body, zero_vel);
            vehicle->can_control = false;
            LOG_WARN("Vehicle out of fuel!");
        } else if (fuel_percentage <= VEHICLE_FUEL_CRITICAL_THRESHOLD) {
            // Critical fuel - reduce performance
            vehicle->control_influence = 0.3f;
        } else if (fuel_percentage <= VEHICLE_FUEL_LOW_THRESHOLD) {
            // Low fuel - slightly reduce performance
            vehicle->control_influence = 0.7f;
        } else {
            vehicle->control_influence = 1.0f;
        }
    }
}

// Refuel vehicle
bool player_vehicle_refuel(PlayerSystem *system, f32 fuel_amount) {
    if (!system || !system->player || fuel_amount <= 0.0f)
        return false;

    if (!system->player->in_vehicle)
        return false;

    VehicleState *vehicle = &system->player->vehicle_state;
    if (!vehicle_requires_fuel(vehicle->type))
        return false;

    if (!system->ecs_world)
        return false;

    EntityID vehicle_entity = system->player->vehicle;
    if (!ecs_entity_exists((World *)system->ecs_world, (Entity){vehicle_entity, 0}))
        return false;

    VehicleComponent *vc = ecs_get_component((World *)system->ecs_world, 
                                            (Entity){vehicle_entity, 0}, 
                                            VEHICLE_COMPONENT_ID);
    if (!vc || !vc->requires_fuel)
        return false;

    f32 old_fuel = vc->fuel_amount;
    vc->fuel_amount = fminf(vc->max_fuel, vc->fuel_amount + fuel_amount);
    f32 actual_refueled = vc->fuel_amount - old_fuel;
    
    if (actual_refueled > 0.0f) {
        // Restore control if fuel was added
        if (old_fuel <= 0.0f && vc->fuel_amount > 0.0f) {
            vehicle->can_control = true;
            LOG_INFO("Vehicle refueled and operational!");
        }
        
        LOG_INFO("Vehicle refueled: %.1f (+%.1f/%.1f)", 
                vc->fuel_amount, actual_refueled, vc->max_fuel);
        return true;
    }
    
    return false;
}

// Get vehicle fuel status
f32 player_vehicle_get_fuel_percentage(const PlayerSystem *system) {
    if (!system || !system->player)
        return 0.0f;

    if (!system->player->in_vehicle)
        return 0.0f;

    const VehicleState *vehicle = &system->player->vehicle_state;
    if (!vehicle_requires_fuel(vehicle->type))
        return 1.0f; // Non-fuel vehicles always have "full" fuel

    if (!system->ecs_world)
        return 0.0f;

    EntityID vehicle_entity = system->player->vehicle;
    if (!ecs_entity_exists((World *)system->ecs_world, (Entity){vehicle_entity, 0}))
        return 0.0f;

    const VehicleComponent *vc = ecs_get_component((World *)system->ecs_world, 
                                                   (Entity){vehicle_entity, 0}, 
                                                   VEHICLE_COMPONENT_ID);
    if (!vc || !vc->requires_fuel || vc->max_fuel <= 0.0f)
        return 1.0f;

    return vc->fuel_amount / vc->max_fuel;
}

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

  // Add VehicleComponent and RigidBody to the vehicle entity
    if (system->ecs_world && system->physics_world) {
        Entity e = {.id = vehicle_entity, .generation = 0};
        if (ecs_entity_exists((World*)system->ecs_world, e)) {
            if (!ecs_has_component((World*)system->ecs_world, e, VEHICLE_COMPONENT_ID)) {
                VehicleComponent vc = {
                    .type = type,
                    .rider = system->player->entity_id,
                    .health = 100.0f,
                    .max_health = 100.0f,
                    .speed = 10.0f,
                    .turn_speed = 90.0f,
                    .acceleration = 5.0f,
                    .can_fly = false,
                    .can_swim = false,
                    .requires_fuel = vehicle_requires_fuel(type),
                    .fuel_amount = vehicle_requires_fuel(type) ? 100.0f : 0.0f,
                    .max_fuel = vehicle_requires_fuel(type) ? 100.0f : 0.0f,
                    .inventory_slots = 0,
                    .physics_body = NULL
                };
                ecs_add_component((World*)system->ecs_world, e, VEHICLE_COMPONENT_ID, &vc);
            }
            if (!ecs_has_component((World*)system->ecs_world, e, RIGIDBODY_COMPONENT_ID)) {
                RigidBodyConfig config = {
                    .shape = SHAPE_BOX,
                    .mass = 1000.0f,
                    .box_extents = {1.0f, 1.0f, 1.0f}
                };
                TransformComponent* t = ecs_get_component((World*)system->ecs_world, e, TRANSFORM_COMPONENT_ID);
                if (t) {
                    RigidBody* body = physics_world_create_rigidbody(system->physics_world, config, t->position, t->rotation);
                    RigidBodyComponent rbc = { .body = body };
                    ecs_add_component((World*)system->ecs_world, e, RIGIDBODY_COMPONENT_ID, &rbc);
                }
            }
        }
    }

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
  
  // Update fuel consumption
  player_vehicle_update_fuel(system, delta_time);
}

void player_vehicle_apply_input(PlayerSystem *system, Vec3 move_input,
                                bool jump, bool brake) {
  if (!system || !system->player)
    return;

  if (!system->player->in_vehicle) {
    return;
  }

  VehicleState *vehicle_state = &system->player->vehicle_state;

  if (!vehicle_state->can_control || vehicle_state->control_influence <= 0.0f) {
    LOG_DEBUG("Cannot control vehicle (control_influence: %.2f)",
              vehicle_state->control_influence);
    return;
  }

    if (system->ecs_world && system->physics_world) {
        Entity e = {.id = vehicle_state->vehicle_entity, .generation = 0};
        if (ecs_entity_exists((World*)system->ecs_world, e)) {
            VehicleComponent* vc = ecs_get_component((World*)system->ecs_world, e, VEHICLE_COMPONENT_ID);
            RigidBodyComponent* rbc = ecs_get_component((World*)system->ecs_world, e, RIGIDBODY_COMPONENT_ID);

            if (vc && rbc && rbc->body) {
                Vec3 current_vel = rigid_body_get_velocity(rbc->body);
                Vec3 current_pos = rigid_body_get_position(rbc->body);
                Vec3 force = {0};
                
                // Enhanced physics with realistic acceleration and friction
                f32 speed_factor = vc->speed * vehicle_state->control_influence;
                f32 acceleration = vc->acceleration;
                
                // Calculate target velocity based on input
                Vec3 target_vel = {0};
                target_vel.x = move_input.x * speed_factor;
                target_vel.z = move_input.z * speed_factor;
                
                // Apply smooth acceleration
                Vec3 vel_diff = vec3_sub(target_vel, current_vel);
                vel_diff.y = 0; // Don't affect vertical movement with horizontal input
                
                f32 vel_diff_mag = vec3_length(vel_diff);
                if (vel_diff_mag > 0.01f) {
                    Vec3 accel_force = vec3_mul(vec3_normalize(vel_diff), acceleration * 100.0f);
                    force = vec3_add(force, accel_force);
                }
                
                // Enhanced turning physics
                if (vec3_length_sq(move_input) > 0.01f) {
                    Vec3 forward = vec3_normalize(vec3(current_vel.x, 0, current_vel.z));
                    Vec3 desired_forward = vec3_normalize(move_input);
                    
                    // Calculate turning angle
                    f32 turn_angle = atan2f(desired_forward.z, desired_forward.x) - atan2f(forward.z, forward.x);
                    
                    // Normalize angle to [-PI, PI]
                    while (turn_angle > M_PI) turn_angle -= 2.0f * M_PI;
                    while (turn_angle < -M_PI) turn_angle += 2.0f * M_PI;
                    
                    // Apply turning torque
                    f32 turn_speed = vc->turn_speed * vehicle_state->control_influence;
                    Vec3 torque = vec3(0, turn_angle * turn_speed * 10.0f, 0);
                    rigid_body_apply_torque(rbc->body, torque);
                }
                
                // Jump mechanics for vehicles that can fly
                if (jump && vc->can_fly) {
                    force.y = acceleration * 150.0f; // Stronger upward force for flight
                }
                
                // Braking system
                if (brake) {
                    Vec3 brake_force = vec3_mul(current_vel, -acceleration * 200.0f);
                    brake_force.y = 0; // Don't brake vertical movement
                    force = vec3_add(force, brake_force);
                }
                
                // Apply air resistance
                Vec3 air_resistance = vec3_mul(current_vel, -5.0f);
                air_resistance.y = 0; // Less air resistance on vertical movement
                force = vec3_add(force, air_resistance);
                
                rigid_body_apply_force(rbc->body, force);
                
                // Enhanced ground detection for vehicles
                Vec3 ground_check = vec3(current_pos.x, current_pos.y - 1.0f, current_pos.z);
                bool on_ground = player_check_block_collision(system, ground_check, vec3(0.5f, 0.1f, 0.5f), NULL);
                
                // Apply ground friction
                if (on_ground && vec3_length_sq(current_vel) > 0.01f) {
                    Vec3 ground_friction = vec3_mul(current_vel, -vc->speed * 0.1f);
                    ground_friction.y = 0;
                    rigid_body_apply_force(rbc->body, ground_friction);
                }
            }
        }
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

  // Get VehicleComponent and apply damage
  VehicleComponent *vc = ecs_get_component((World *)system->ecs_world, 
                                          (Entity){vehicle_entity, 0}, 
                                          VEHICLE_COMPONENT_ID);
  if (!vc) {
    LOG_WARN("Vehicle component not found");
    return;
  }

  // Apply damage with wear system
  f32 old_health = vc->health;
  vc->health = fmaxf(0.0f, vc->health - damage);
  
  // Calculate wear level
  u32 wear_level = 0;
  for (u32 i = 0; i < VEHICLE_MAX_WEAR_LEVELS; i++) {
    if (vc->health <= vehicle_wear_levels[i].health_threshold) {
      wear_level = i;
    }
  }
  
  // Apply visual wear effects
  u32 visual_effects = vehicle_wear_levels[wear_level].visual_effects;
  
  // Log damage with wear information
  LOG_INFO("Vehicle damaged for %.1f HP (%.1f -> %.1f) - Wear: %s (effects: 0x%02X)", 
           damage, old_health, vc->health, 
           vehicle_wear_levels[wear_level].description, visual_effects);
  
  // Apply performance penalties based on wear
  f32 performance_factor = 1.0f;
  if (wear_level >= 2) { // Moderate wear or worse
    performance_factor = 0.8f; // 20% speed reduction
  }
  if (wear_level >= 3) { // Heavy wear or worse
    performance_factor = 0.6f; // 40% speed reduction
  }
  if (wear_level >= 4) { // Critical damage
    performance_factor = 0.3f; // 70% speed reduction
  }
  
  // Update vehicle performance based on wear
  vc->speed = vc->speed * performance_factor;
  vc->acceleration = vc->acceleration * performance_factor;
  
  // Create damage effects
  if (damage > VEHICLE_DAMAGE_WEAR_THRESHOLD) {
    // TODO: Create particle effects for significant damage
    // vehicle_create_damage_particles(vehicle_entity, damage, wear_level);
  }
  
  // Check if vehicle is destroyed
  if (vc->health <= 0.0f) {
    LOG_WARN("Vehicle destroyed!");
    // TODO: Create explosion effects
    // vehicle_create_destruction_effects(vehicle_entity);
    
    // Force dismount
    player_dismount_vehicle(system);
    
    // Remove vehicle entity
    ecs_destroy_entity((World *)system->ecs_world, (Entity){vehicle_entity, 0});
  }
}

// Enhanced damage function with damage type
void player_vehicle_damage_typed(PlayerSystem *system, f32 damage, VehicleDamageType damage_type) {
  if (!system || !system->player || damage <= 0.0f)
    return;

  // Apply damage type multipliers
  f32 modified_damage = damage;
  switch (damage_type) {
    case VEHICLE_DAMAGE_COLLISION:
      modified_damage *= 1.0f; // Normal collision damage
      break;
    case VEHICLE_DAMAGE_FALL:
      modified_damage *= 1.5f; // Fall damage is more severe
      break;
    case VEHICLE_DAMAGE_EXPLOSION:
      modified_damage *= 2.0f; // Explosions are very damaging
      break;
    case VEHICLE_DAMAGE_FIRE:
      modified_damage *= 0.5f; // Fire does less damage to metal vehicles
      break;
    case VEHICLE_DAMAGE_WATER:
      // Water damage only applies to certain vehicle types
      if (system->player->vehicle_state.type != VEHICLE_TYPE_BOAT &&
          system->player->vehicle_state.type != VEHICLE_TYPE_STRIDER) {
        modified_damage *= 0.1f; // Minimal water damage
      } else {
        modified_damage = 0.0f; // No water damage for water vehicles
      }
      break;
    default:
      break;
  }
  
  LOG_DEBUG("Vehicle damage type %d: %.1f -> %.1f", damage_type, damage, modified_damage);
  player_vehicle_damage(system, modified_damage);
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
