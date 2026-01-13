#ifndef HEALTH_COMPONENT_H
#define HEALTH_COMPONENT_H

#include "../../common.h"
#include "../../ecs/ecs.h"

typedef struct {
    f32 health;
    f32 max_health;
    bool is_alive;
    f32 last_damage_time;
    f32 regen_rate;
} HealthComponent;

#endif // HEALTH_COMPONENT_H
