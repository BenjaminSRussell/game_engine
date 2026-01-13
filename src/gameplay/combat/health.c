#include "engine/include/common.h"
#include "engine/include/core/logger.h"
#include <ecs/components/health.h>
#include <ecs/ecs.h>
#include <stdlib.h>
#include <string.h>

static World *health_world = NULL;

bool health_system_init(World *world) {
  if (!world) {
    LOG_ERROR("Health system: NULL world provided");
    return false;
  }

  health_world = world;

  LOG_INFO("Health system initialized");
  return true;
}

void health_system_shutdown(void) {
  health_world = NULL;
  LOG_INFO("Health system shutdown");
}

HealthComponent *health_create_component(f32 max_health) {
  HealthComponent *health = calloc(1, sizeof(HealthComponent));
  if (!health) {
    LOG_ERROR("Failed to allocate health component");
    return NULL;
  }

  health->health = max_health;
  health->max_health = max_health;
  health->regeneration_rate = 0.0f;
  health->last_damage_time = 0.0f;
  health->is_alive = true;

  return health;
}

void health_destroy_component(HealthComponent *health) {
  if (health) {
    free(health);
  }
}

bool health_apply_damage(HealthComponent *health, f32 damage) {
  if (!health || !health->is_alive || damage <= 0.0f) {
    return false;
  }

  health->health -= damage;
  health->last_damage_time = 0.0f; // Would get actual time

  if (health->health <= 0.0f) {
    health->health = 0.0f;
    health->is_alive = false;
    LOG_DEBUG("Entity died");
  }

  return true;
}

bool health_apply_healing(HealthComponent *health, f32 healing) {
  if (!health || !health->is_alive || healing <= 0.0f) {
    return false;
  }

  health->health += healing;
  if (health->health > health->max_health) {
    health->health = health->max_health;
  }

  return true;
}

void health_update(HealthComponent *health, f32 delta_time) {
  if (!health || !health->is_alive)
    return;

  // Apply regeneration
  if (health->regeneration_rate > 0.0f) {
    health_apply_healing(health, health->regeneration_rate * delta_time);
  }
}

f32 health_get_percentage(const HealthComponent *health) {
  if (!health || health->max_health <= 0.0f) {
    return 0.0f;
  }

  return health->health / health->max_health;
}

bool health_is_alive(const HealthComponent *health) {
  return health ? health->is_alive : false;
}

bool health_is_at_full_health(const HealthComponent *health) {
  if (!health)
    return false;
  return health->health >= health->max_health;
}

void health_revive(HealthComponent *health) {
  if (!health)
    return;

  health->health = health->max_health;
  health->is_alive = true;
  health->last_damage_time = 0.0f;

  LOG_DEBUG("Entity revived");
}
