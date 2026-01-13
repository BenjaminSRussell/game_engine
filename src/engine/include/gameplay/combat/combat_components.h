#ifndef COMBAT_COMPONENTS_H
#define COMBAT_COMPONENTS_H

#include "include/common.h"
#include "include/ecs/ecs.h"
#include "include/math/vec3.h"
#include "include/math/quat.h"

// Damage Flags
#ifndef DAMAGE_FLAG_AREA_OF_EFFECT
#define DAMAGE_FLAG_AREA_OF_EFFECT (1 << 8)
#endif
#ifndef DAMAGE_FLAG_MAGICAL
#define DAMAGE_FLAG_MAGICAL (1 << 9)
#endif

// Status Component
#define MAX_STATUS_EFFECTS 16

typedef struct {
    uint32_t status_id;
    Entity source_entity;
    float duration;
    float remaining_time;
    uint32_t stack_count;
    bool is_active;
} StatusEffect;

typedef struct {
    StatusEffect effects[MAX_STATUS_EFFECTS];
    uint32_t effect_count;
} StatusComponent;

// Ability Component
typedef struct {
    uint32_t ability_id;
    Entity caster_entity;
    float current_cooldown;
    bool is_channeling;
    float channel_time;
} AbilityComponent;

// Combo Component
typedef struct {
    uint32_t current_combo;
    float combo_timer;
    uint32_t combo_count;
    bool is_active;
} ComboComponent;

// Area Effect Component
typedef struct {
    float radius;
    float duration;
    float damage_per_second;
    uint32_t damage_type;
    Entity source_entity;
    bool affects_allies;
    bool affects_enemies;
} AreaEffectComponent;

#endif // COMBAT_COMPONENTS_H
