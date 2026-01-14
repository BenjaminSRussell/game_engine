// include/ecs/component_ids.h
//
// Purpose: Defines a comprehensive enumeration of unique identifiers for all
// components used within the Entity-Component-System (ECS) architecture of the
// game engine. This standardization allows for efficient identification and
// retrieval of component types when interacting with entities.
//
// Public APIs:
// - `ComponentID`: An enumeration listing all distinct component types.
//   Examples include `TRANSFORM_COMPONENT_ID`, `RIGIDBODY_COMPONENT_ID`,
//   `NPC_COMPONENT_ID`, `HEALTH_COMPONENT_ID`, and `PLAYER_COMPONENT_ID`.
//   `MAX_COMPONENTS` is used to define the total number of unique component
//   types.
//
// Ownership: This file primarily defines constants and does not "own" any
// runtime data. It serves as a static registry for component types.
//
// Invariants:
// - Each `ComponentID` must be unique.
// - New components added to the ECS must be assigned a unique ID in this
// enumeration.
// - `MAX_COMPONENTS` must always be the last entry and correctly reflect the
// total count
//   of component types (excluding itself).
//
#ifndef COMPONENT_IDS_H
#define COMPONENT_IDS_H

typedef enum {
  TRANSFORM_COMPONENT_ID,
  RIGIDBODY_COMPONENT_ID,
  NPC_COMPONENT_ID,
  HEALTH_COMPONENT_ID,
  PLAYER_COMPONENT_ID,
  SHIELD_COMPONENT_ID,
  ENEMY_AI_COMPONENT_ID,
  HUNGER_COMPONENT_ID,
  EQUIPMENT_COMPONENT_ID,
  DIALOGUE_COMPONENT_ID,
  // Combat system components
  HITBOX_COMPONENT_ID,
  DAMAGE_COMPONENT_ID,
  RESISTANCE_COMPONENT_ID,
  PROJECTILE_COMPONENT_ID,
  STATUS_COMPONENT_ID,
  ABILITY_COMPONENT_ID,
  COMBO_COMPONENT_ID,
  AREA_EFFECT_COMPONENT_ID,
  // Inventory system components
  INVENTORY_COMPONENT_ID,
  ITEM_COMPONENT_ID,
  // Asset system components
  ASSET_INSTANCE_COMPONENT_ID,
  FALLING_BLOCK_COMPONENT_ID,
  // Add new component IDs here
  COMPONENT_ID_COUNT,
} ComponentID;

#endif // COMPONENT_IDS_H
