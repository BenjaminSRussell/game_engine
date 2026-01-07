#include "ecs/components/health.h"
#include "gameplay/combat/damage.h"
#include <core/memory.h>
#include <math.h>

// Enhanced health system extending the base HealthComponent

// ============================================================================
// HEALTH MANAGEMENT
// ============================================================================

void health_take_damage(HealthComponent *health, f32 damage) {
  if (!health)
    return;

  health->health -= damage;
  if (health->health < 0.0f) {
    health->health = 0.0f;
  }
}

void health_heal(HealthComponent *health, f32 amount) {
  if (!health)
    return;

  health->health += amount;
  if (health->health > health->max_health) {
    health->health = health->max_health;
  }
}

void health_set_max(HealthComponent *health, f32 max_health) {
  if (!health)
    return;

  health->max_health = max_health;
  if (health->health > max_health) {
    health->health = max_health;
  }
}

bool health_is_alive(const HealthComponent *health) {
  return health && health->health > 0.0f;
}

bool health_is_dead(const HealthComponent *health) {
  return health && health->health <= 0.0f;
}

f32 health_get_percentage(const HealthComponent *health) {
  if (!health || health->max_health <= 0.0f)
    return 0.0f;
  return health->health / health->max_health;
}

void health_kill(HealthComponent *health) {
  if (!health)
    return;
  health->health = 0.0f;
}

void health_revive(HealthComponent *health, f32 amount) {
  if (!health)
    return;

  health->health = amount;
  if (health->health > health->max_health) {
    health->health = health->max_health;
  }
}
