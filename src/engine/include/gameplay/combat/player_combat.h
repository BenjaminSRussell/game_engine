#ifndef PLAYER_COMBAT_H
#define PLAYER_COMBAT_H

#include <common.h>
#include <ecs/ecs.h>

/**
 * @brief Initiates an attack from a player entity towards a target entity.
 *
 * This function handles the logic for a player attacking a target entity.
 * It checks for validity of entities, range, and triggers the appropriate combat events.
 *
 * @param world The ECS world.
 * @param player The entity representing the player.
 * @param target The target entity to attack.
 * @return true if the attack was successfully initiated, false otherwise.
 */
bool player_attack_entity(World *world, Entity player, Entity target);

#endif // PLAYER_COMBAT_H
