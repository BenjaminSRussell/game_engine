#include "include/gameplay/combat/player_combat.h"
#include "include/gameplay/combat/combat_system.h"
#include "include/ecs/ecs.h"
#include "include/ecs/component_ids.h"
#include "include/ecs/components/transform.h"
#include "include/ecs/components/health.h"
#include "include/core/logger.h"
#include "include/math/vec3.h"

// Default combat values
#define DEFAULT_MELEE_RANGE 2.5f
#define DEFAULT_MELEE_DAMAGE 10.0f

bool player_attack_entity(World *world, Entity player, Entity target) {
    if (!world) {
        LOG_ERROR(LOG_CAT_GAME, "player_attack_entity: World is NULL");
        return false;
    }

    if (player.id == 0 || target.id == 0) {
        LOG_WARN(LOG_CAT_GAME, "player_attack_entity: Invalid player or target entity");
        return false;
    }

    // Verify player component exists
    if (!ecs_has_component(world, player, PLAYER_COMPONENT_ID)) {
        LOG_WARN(LOG_CAT_GAME, "player_attack_entity: Entity %d is not a player", player.id);
        return false;
    }

    // Check if both entities are alive
    HealthComponent *player_health = ecs_get_component(world, player, HEALTH_COMPONENT_ID);
    HealthComponent *target_health = ecs_get_component(world, target, HEALTH_COMPONENT_ID);

    if (player_health && !player_health->is_alive) {
        LOG_DEBUG(LOG_CAT_GAME, "player_attack_entity: Player is dead, cannot attack");
        return false;
    }

    if (target_health && !target_health->is_alive) {
        LOG_DEBUG(LOG_CAT_GAME, "player_attack_entity: Target is already dead");
        return false;
    }

    // Get transforms to check range
    TransformComponent *player_transform = ecs_get_component(world, player, TRANSFORM_COMPONENT_ID);
    TransformComponent *target_transform = ecs_get_component(world, target, TRANSFORM_COMPONENT_ID);

    if (!player_transform || !target_transform) {
        LOG_WARN(LOG_CAT_GAME, "player_attack_entity: Missing transform components");
        return false;
    }

    // Calculate distance
    f32 distance = vec3_distance(player_transform->position, target_transform->position);

    // Check range
    if (distance > DEFAULT_MELEE_RANGE) {
        LOG_DEBUG(LOG_CAT_GAME, "player_attack_entity: Target out of range (dist: %.2f, range: %.2f)", distance, DEFAULT_MELEE_RANGE);
        return false;
    }

    // Calculate direction
    Vec3 direction = vec3_sub(target_transform->position, player_transform->position);
    direction = vec3_normalize(direction);

    // Create melee attack
    Entity attack = combat_create_melee_attack(world, player, player_transform->position, direction, DEFAULT_MELEE_DAMAGE, DEFAULT_MELEE_RANGE);

    if (attack.id != 0) {
        LOG_INFO(LOG_CAT_GAME, "Player %d attacked target %d", player.id, target.id);
        return true;
    } else {
        LOG_ERROR(LOG_CAT_GAME, "Failed to create melee attack");
        return false;
    }
}
