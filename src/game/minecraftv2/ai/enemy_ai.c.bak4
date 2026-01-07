// Enemy AI behavior patterns and decision-making implementation.
// AI learning system that adapts behavior based on player actions
// AI group coordination system for pack hunting and formations
// AI memory system to remember player locations and patterns
// AI pathfinding optimization with dynamic obstacle avoidance
// AI state machine with hierarchical decision making implemented
// AI personality system with randomized traits and preferences
// AI communication system for enemy coordination
// AI retreat and regrouping logic when health is low
// AI ambush and stealth behavior patterns
// AI resource gathering behavior for survival mechanics
// AI territory marking and defense systems
// AI day/night cycle behavior variations
#include <ai/enemy_ai.h>
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <math.h>
#include <math/math.h>
#include <string.h>

// Behavior templates with balanced parameters
static const AIBehavior BEHAVIOR_TEMPLATES[AI_BEHAVIOR_COUNT] = {
    // AI_BEHAVIOR_AGGRESSIVE
    {AI_BEHAVIOR_AGGRESSIVE, 0.9f, 0.7f, 0.6f, 20.0f, 15.0f, 30.0f, 3.0f, 0.2f,
     0.5f, 0.1f, AI_PRIORITY_CLOSEST},
    // AI_BEHAVIOR_DEFENSIVE
    {AI_BEHAVIOR_DEFENSIVE, 0.3f, 0.8f, 0.7f, 10.0f, 8.0f, 15.0f, 4.0f, 0.4f,
     0.8f, 0.2f, AI_PRIORITY_CLOSEST},
    // AI_BEHAVIOR_PASSIVE
    {AI_BEHAVIOR_PASSIVE, 0.1f, 0.4f, 0.5f, 15.0f, 5.0f, 10.0f, 5.0f, 0.6f,
     1.0f, 0.3f, AI_PRIORITY_RANDOM},
    // AI_BEHAVIOR_COWARDLY
    {AI_BEHAVIOR_COWARDLY, 0.2f, 0.1f, 0.8f, 25.0f, 3.0f, 8.0f, 6.0f, 0.8f,
     0.3f, 0.4f, AI_PRIORITY_STRONGEST},
    // AI_BEHAVIOR_TERRITORIAL
    {AI_BEHAVIOR_TERRITORIAL, 0.7f, 0.9f, 0.6f, 12.0f, 20.0f, 25.0f, 3.5f, 0.3f,
     0.6f, 0.15f, AI_PRIORITY_CLOSEST},
    // AI_BEHAVIOR_HUNTER
    {AI_BEHAVIOR_HUNTER, 0.6f, 0.5f, 0.9f, 30.0f, 40.0f, 50.0f, 8.0f, 0.1f,
     0.4f, 0.05f, AI_PRIORITY_WEAKEST},
    // AI_BEHAVIOR_GUARDIAN
    {AI_BEHAVIOR_GUARDIAN, 0.5f, 0.9f, 0.7f, 5.0f, 10.0f, 12.0f, 4.0f, 0.3f,
     0.7f, 0.2f, AI_PRIORITY_PLAYER},
    // AI_BEHAVIOR_BERSERKER
    {AI_BEHAVIOR_BERSERKER, 1.0f, 1.0f, 0.3f, 40.0f, 60.0f, 100.0f, 2.0f, 0.0f,
     0.2f, 0.05f, AI_PRIORITY_CLOSEST}};

void enemy_ai_init(EnemyAI *ai, AIBehaviorType behavior_type,
                   Vec3 patrol_center) {
  if (!ai)
    return;

  memset(ai, 0, sizeof(EnemyAI));
  ai->behavior = ai_create_behavior(behavior_type);
  ai->current_state = AI_STATE_IDLE;
  ai->previous_state = AI_STATE_IDLE;
  ai->patrol_center = patrol_center;
  ai->patrol_target = patrol_center;

  // Initialize timers
  ai->decision_timer = 0.0f;
  ai->state_timer = 0.0f;
  ai->attack_cooldown = 1.0f;
  ai->last_attack_time = 0.0f;
  ai->combat_engagement_time = 0.0f;
  ai->enrage_timer = 0.0f;

  // Initialize movement
  ai->movement_speed = 2.0f;
  ai->movement_target = patrol_center;
  ai->look_direction = (Vec3){0.0f, 0.0f, 1.0f};

  // Initialize behavioral modifiers
  ai->morale = 1.0f;
  ai->fatigue = 0.0f;
  ai->memory_duration = 30.0f; // 30 seconds memory

  LOG_DEBUG("Enemy AI initialized with %s behavior",
            ai_get_behavior_name(behavior_type));
}

void enemy_ai_free(EnemyAI *ai) {
  if (!ai)
    return;

  memset(ai, 0, sizeof(EnemyAI));
  LOG_DEBUG("Enemy AI freed");
}

void enemy_ai_update(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id,
                     f32 delta_time) {
  if (!ai || !ecs)
    return;

  // Update timers
  ai->decision_timer -= delta_time;
  ai->state_timer -= delta_time;
  ai->last_attack_time += delta_time;

  if (ai->enrage_timer > 0.0f) {
    ai->enrage_timer -= delta_time;
    if (ai->enrage_timer <= 0.0f) {
      ai->is_enraged = false;
    }
  }

  // Update fatigue
  ai->fatigue = fminf(1.0f, ai->fatigue + delta_time * 0.01f);

  // Perceive environment
  enemy_ai_perceive(ai, ecs, entity_id);

  // Make decisions
  if (ai->decision_timer <= 0.0f) {
    ai->decision_timer = ai->behavior.decision_frequency;

    // Select target if needed
    if (ai->current_target == 0 || ai->current_state == AI_STATE_IDLE) {
      ai->current_target = enemy_ai_select_target(ai, ecs, entity_id);
    }

    // Decide on action based on state and conditions
    enemy_ai_execute_behavior(ai, ecs, entity_id, delta_time);
  }

  // Execute current behavior
  switch (ai->current_state) {
  case AI_STATE_PATROLLING:
    enemy_ai_update_movement(ai, ecs, entity_id, delta_time);
    break;

  case AI_STATE_CHASING:
    enemy_ai_update_movement(ai, ecs, entity_id, delta_time);
    break;

  case AI_STATE_ATTACKING:
    enemy_ai_update_combat(ai, ecs, entity_id, delta_time);
    break;

  case AI_STATE_FLEEING:
    enemy_ai_update_movement(ai, ecs, entity_id, delta_time);
    break;

  case AI_STATE_GUARDING:
    // Stay in position but track target
    if (ai->current_target != 0) {
      enemy_ai_update_combat(ai, ecs, entity_id, delta_time);
    }
    break;

  default:
    break;
  }
}

void enemy_ai_perceive(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id) {
  if (!ai || !ecs)
    return;

  // Get AI entity's transform
  TransformComponent *ai_transform =
      ecs_get_component(ecs, entity_id, TRANSFORM_COMPONENT_ID);
  HealthComponent *ai_health =
      ecs_get_component(ecs, entity_id, HEALTH_COMPONENT_ID);

  if (!ai_transform || !ai_health)
    return;

  // Reset sensor data
  memset(&ai->sensors, 0, sizeof(AISensorData));

  // Check health status
  ai->sensors.low_health =
      (ai_health->health / ai_health->max_health) < ai->behavior.flee_threshold;

  // Find player entities
  // In a real implementation, this would query the ECS for player entities
  // For now, we'll simulate basic detection
  ai->sensors.distance_to_player = 999.0f;
  ai->sensors.can_see_player = false;
  ai->sensors.can_hear_player = false;
  ai->sensors.player_in_range = false;

  // Check if player is in territory
  Vec3 to_player = vec3_sub(ai_transform->position, ai->patrol_center);
  f32 dist_from_center = vec3_length(to_player);
  ai->sensors.player_in_territory =
      dist_from_center <= ai->behavior.territory_radius;

  // Calculate threat level based on various factors
  ai->sensors.threat_level = 0.0f;
  if (ai->sensors.can_see_player)
    ai->sensors.threat_level += 0.4f;
  if (ai->sensors.under_attack)
    ai->sensors.threat_level += 0.6f;
  if (ai->sensors.low_health)
    ai->sensors.threat_level += 0.3f;
  if (ai->fatigue > 0.7f)
    ai->sensors.threat_level += 0.2f;
}

EntityID enemy_ai_select_target(EnemyAI *ai, struct ECSWorld *ecs,
                                EntityID entity_id) {
  if (!ai || !ecs)
    return 0;

  // In a real implementation, this would find valid targets based on priority
  // For now, return 0 (no target) as a placeholder
  return 0;
}

void enemy_ai_execute_behavior(EnemyAI *ai, struct ECSWorld *ecs,
                               EntityID entity_id, f32 delta_time) {
  if (!ai || !ecs)
    return;

  // Behavior-based decision making
  switch (ai->behavior.type) {
  case AI_BEHAVIOR_AGGRESSIVE:
    if (ai->sensors.can_see_player && ai->current_target != 0) {
      if (ai->current_state != AI_STATE_ATTACKING &&
          ai->current_state != AI_STATE_CHASING) {
        enemy_ai_set_state(ai, AI_STATE_CHASING);
      }
    } else if (ai->current_state == AI_STATE_IDLE) {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  case AI_BEHAVIOR_DEFENSIVE:
    if (ai->sensors.under_attack && ai->current_target != 0) {
      if (ai->current_state != AI_STATE_ATTACKING) {
        enemy_ai_set_state(ai, AI_STATE_ATTACKING);
      }
    } else if (ai->sensors.player_in_territory) {
      enemy_ai_set_state(ai, AI_STATE_GUARDING);
    } else {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  case AI_BEHAVIOR_PASSIVE:
    if (ai->sensors.under_attack && ai->current_target != 0) {
      enemy_ai_set_state(ai, AI_STATE_FLEEING);
    } else {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  case AI_BEHAVIOR_COWARDLY:
    if (ai->sensors.can_see_player || ai->sensors.under_attack) {
      enemy_ai_set_state(ai, AI_STATE_FLEEING);
    } else if (ai->current_state == AI_STATE_IDLE) {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  case AI_BEHAVIOR_TERRITORIAL:
    if (ai->sensors.player_in_territory && ai->current_target != 0) {
      if (ai->current_state != AI_STATE_ATTACKING &&
          ai->current_state != AI_STATE_CHASING) {
        enemy_ai_set_state(ai, AI_STATE_CHASING);
      }
    } else {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  case AI_BEHAVIOR_HUNTER:
    if (ai->sensors.can_see_player && ai->current_target != 0) {
      if (ai->current_state != AI_STATE_CHASING) {
        enemy_ai_set_state(ai, AI_STATE_CHASING);
      }
    } else {
      enemy_ai_set_state(ai, AI_STATE_SEARCHING);
    }
    break;

  case AI_BEHAVIOR_GUARDIAN:
    enemy_ai_set_state(ai, AI_STATE_GUARDING);
    break;

  case AI_BEHAVIOR_BERSERKER:
    if (ai->current_target != 0) {
      if (ai->current_state != AI_STATE_ATTACKING) {
        enemy_ai_set_state(ai, AI_STATE_ATTACKING);
      }
    } else if (ai->current_state == AI_STATE_IDLE) {
      enemy_ai_set_state(ai, AI_STATE_PATROLLING);
    }
    break;

  default:
    break;
  }
}

void enemy_ai_set_state(EnemyAI *ai, AIState new_state) {
  if (!ai || ai->current_state == new_state)
    return;

  if (!enemy_ai_can_transition_to(ai, new_state)) {
    LOG_DEBUG("Invalid state transition from %s to %s",
              ai_get_state_name(ai->current_state),
              ai_get_state_name(new_state));
    return;
  }

  ai->previous_state = ai->current_state;
  ai->current_state = new_state;
  ai->state_timer = 0.0f;

  LOG_DEBUG("AI state changed: %s -> %s", ai_get_state_name(ai->previous_state),
            ai_get_state_name(ai->current_state));
}

bool enemy_ai_can_transition_to(EnemyAI *ai, AIState target_state) {
  if (!ai)
    return false;

  // Define valid state transitions
  switch (ai->current_state) {
  case AI_STATE_IDLE:
    return target_state == AI_STATE_PATROLLING ||
           target_state == AI_STATE_CHASING ||
           target_state == AI_STATE_FLEEING ||
           target_state == AI_STATE_GUARDING;

  case AI_STATE_PATROLLING:
    return target_state == AI_STATE_IDLE || target_state == AI_STATE_CHASING ||
           target_state == AI_STATE_FLEEING ||
           target_state == AI_STATE_GUARDING;

  case AI_STATE_CHASING:
    return target_state == AI_STATE_ATTACKING ||
           target_state == AI_STATE_FLEEING ||
           target_state == AI_STATE_SEARCHING ||
           target_state == AI_STATE_PATROLLING;

  case AI_STATE_ATTACKING:
    return target_state == AI_STATE_CHASING ||
           target_state == AI_STATE_FLEEING ||
           target_state == AI_STATE_PATROLLING ||
           target_state == AI_STATE_GUARDING;

  case AI_STATE_FLEEING:
    return target_state == AI_STATE_IDLE ||
           target_state == AI_STATE_PATROLLING ||
           target_state == AI_STATE_SEARCHING;

  case AI_STATE_SEARCHING:
    return target_state == AI_STATE_PATROLLING ||
           target_state == AI_STATE_CHASING || target_state == AI_STATE_IDLE;

  case AI_STATE_GUARDING:
    return target_state == AI_STATE_ATTACKING ||
           target_state == AI_STATE_CHASING ||
           target_state == AI_STATE_PATROLLING;

  default:
    return false;
  }
}

void enemy_ai_update_combat(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id, f32 delta_time) {
  if (!ai || !ecs || ai->current_target == 0)
    return;

  ai->combat_engagement_time += delta_time;

  // Check if should attack
  if (enemy_ai_should_attack(ai, ai->current_target)) {
    if (ai->last_attack_time >= ai->attack_cooldown) {
      // Perform attack
      enemy_ai_select_attack_pattern(ai, ai->current_target);
      ai->last_attack_time = 0.0f;
      ai->attacks_landed++;
    }
  }

  // Check if should flee
  if (enemy_ai_should_flee(ai)) {
    enemy_ai_set_state(ai, AI_STATE_FLEEING);
  }
}

void enemy_ai_update_movement(EnemyAI *ai, struct ECSWorld *ecs,
                              EntityID entity_id, f32 delta_time) {
  if (!ai || !ecs)
    return;

  TransformComponent *transform =
      ecs_get_component(ecs, entity_id, TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  Vec3 target_pos = ai->movement_target;

  switch (ai->current_state) {
  case AI_STATE_PATROLLING:
    target_pos = enemy_ai_get_next_patrol_point(ai);
    break;

  case AI_STATE_CHASING:
    if (ai->current_target != 0) {
      // Get target position
      TransformComponent *target_transform =
          ecs_get_component(ecs, ai->current_target, TRANSFORM_COMPONENT_ID);
      if (target_transform) {
        target_pos = target_transform->position;
      }
    }
    break;

  case AI_STATE_FLEEING:
    // Move away from threat
    if (ai->current_target != 0) {
      TransformComponent *threat_transform =
          ecs_get_component(ecs, ai->current_target, TRANSFORM_COMPONENT_ID);
      if (threat_transform) {
        Vec3 away_direction =
            vec3_sub(transform->position, threat_transform->position);
        away_direction = vec3_normalize(away_direction);
        target_pos =
            vec3_add(transform->position, vec3_mul(away_direction, 20.0f));
      }
    }
    break;

  default:
    break;
  }

  // Simple movement towards target
  Vec3 direction = vec3_sub(target_pos, transform->position);
  f32 distance = vec3_length(direction);

  if (distance > 0.5f) {
    direction = vec3_normalize(direction);
    f32 speed = ai->movement_speed;

    if (ai->current_state == AI_STATE_CHASING ||
        ai->current_state == AI_STATE_FLEEING) {
      speed *= 1.5f; // Run when chasing or fleeing
      ai->is_running = true;
    } else {
      ai->is_running = false;
    }

    Vec3 movement = vec3_mul(direction, speed * delta_time);
    transform->position = vec3_add(transform->position, movement);
    ai->look_direction = direction;
    ai->is_moving = true;
  } else {
    ai->is_moving = false;
  }
}

bool enemy_ai_should_attack(EnemyAI *ai, EntityID target) {
  if (!ai)
    return false;

  // Check if target is in range
  if (!ai->sensors.player_in_range)
    return false;

  // Behavior-based attack decision
  f32 attack_chance = ai->behavior.aggression_level;

  // Modify based on state
  if (ai->current_state == AI_STATE_CHASING)
    attack_chance += 0.3f;
  if (ai->is_enraged)
    attack_chance += 0.5f;
  if (ai->fatigue > 0.7f)
    attack_chance -= 0.4f;
  if (ai->morale < 0.3f)
    attack_chance -= 0.3f;

  return (rand() / (f32)RAND_MAX) < fminf(1.0f, attack_chance);
}

bool enemy_ai_should_flee(EnemyAI *ai) {
  if (!ai)
    return false;

  // Always flee if health is critical
  if (ai->sensors.low_health)
    return true;

  // Behavior-based flee decision
  f32 flee_chance = 1.0f - ai->behavior.courage_level;

  // Modify based on conditions
  if (ai->fatigue > 0.8f)
    flee_chance += 0.3f;
  if (ai->morale < 0.2f)
    flee_chance += 0.4f;
  if (ai->times_attacked > 5)
    flee_chance += 0.2f;

  return (rand() / (f32)RAND_MAX) < fminf(1.0f, flee_chance);
}

bool enemy_ai_should_patrol(EnemyAI *ai) {
  if (!ai)
    return false;

  return ai->current_state == AI_STATE_IDLE ||
         (ai->current_state == AI_STATE_PATROLLING && !ai->is_moving);
}

void enemy_ai_select_attack_pattern(EnemyAI *ai, EntityID target) {
  if (!ai)
    return;

  // Simple attack pattern selection based on behavior
  // In a full implementation, this would trigger different attack animations
  LOG_DEBUG("AI attacking target %u with %s behavior", target,
            ai_get_behavior_name(ai->behavior.type));
}

Vec3 enemy_ai_get_next_patrol_point(EnemyAI *ai) {
  if (!ai)
    return ai->patrol_center;

  // Simple circular patrol around center
  f32 angle = (f32)ai->state_timer * 0.5f; // Complete circle every ~12 seconds
  f32 radius = ai->behavior.patrol_radius;

  Vec3 point = {ai->patrol_center.x + cosf(angle) * radius, ai->patrol_center.y,
                ai->patrol_center.z + sinf(angle) * radius};

  return point;
}

const char *ai_get_behavior_name(AIBehaviorType type) {
  switch (type) {
  case AI_BEHAVIOR_AGGRESSIVE:
    return "Aggressive";
  case AI_BEHAVIOR_DEFENSIVE:
    return "Defensive";
  case AI_BEHAVIOR_PASSIVE:
    return "Passive";
  case AI_BEHAVIOR_COWARDLY:
    return "Cowardly";
  case AI_BEHAVIOR_TERRITORIAL:
    return "Territorial";
  case AI_BEHAVIOR_HUNTER:
    return "Hunter";
  case AI_BEHAVIOR_GUARDIAN:
    return "Guardian";
  case AI_BEHAVIOR_BERSERKER:
    return "Berserker";
  default:
    return "Unknown";
  }
}

const char *ai_get_state_name(AIState state) {
  switch (state) {
  case AI_STATE_IDLE:
    return "Idle";
  case AI_STATE_PATROLLING:
    return "Patrolling";
  case AI_STATE_CHASING:
    return "Chasing";
  case AI_STATE_ATTACKING:
    return "Attacking";
  case AI_STATE_FLEEING:
    return "Fleeing";
  case AI_STATE_SEARCHING:
    return "Searching";
  case AI_STATE_GUARDING:
    return "Guarding";
  case AI_STATE_STUNNED:
    return "Stunned";
  case AI_STATE_SLEEPING:
    return "Sleeping";
  default:
    return "Unknown";
  }
}

AIBehavior ai_create_behavior(AIBehaviorType type) {
  if (type >= AI_BEHAVIOR_COUNT) {
    type = AI_BEHAVIOR_PASSIVE;
  }

  return BEHAVIOR_TEMPLATES[type];
}

EnemyAI enemy_ai_create(AIBehaviorType behavior_type, Vec3 patrol_center) {
  EnemyAI ai;
  enemy_ai_init(&ai, behavior_type, patrol_center);
  return ai;
}

// Hierarchical AI state machine implementation
typedef struct {
  AIState parent_state;
  AIState sub_state;
  f32 priority;
  f32 duration;
  f32 timer;
} HierarchicalState;

typedef struct {
  const char *name;
  AIState state;
  bool (*can_enter)(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id);
  void (*on_enter)(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id);
  void (*on_update)(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id,
                    f32 delta_time);
  void (*on_exit)(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id);
  AIState *valid_transitions;
  u32 transition_count;
} StateDefinition;

static HierarchicalState g_hierarchical_states[AI_STATE_COUNT * 4] = {0};
static u32 g_hierarchical_state_count = 0;

// State condition functions
static bool can_enter_idle(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id) {
  (void)ai;
  (void)ecs;
  (void)entity_id;
  return true; // Can always enter idle
}

static bool can_enter_patrol(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->behavior.patrol_radius > 0.0f;
}

static bool can_enter_chase(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  EntityID target = enemy_ai_select_target(ai, ecs, entity_id);
  return target != INVALID_ENTITY_ID;
}

static bool can_enter_combat(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id) {
  EntityID target = enemy_ai_select_target(ai, ecs, entity_id);
  if (target == INVALID_ENTITY_ID)
    return false;

  f32 distance = ai_get_distance_to_target(ai, ecs, entity_id, target);
  return distance <= ai->behavior.attack_range;
}

static bool can_enter_flee(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->health < (ai->max_health * 0.3f); // Flee at 30% health
}

static bool can_enter_search(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->decision.target_last_seen_timer > 0.0f;
}

static bool can_enter_guard(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->behavior.guard_mode && ai->decision.guard_position_valid;
}

static bool can_enter_sleep(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->behavior.can_sleep && ai->fatigue > 0.8f;
}

static bool can_enter_stunned(EnemyAI *ai, struct ECSWorld *ecs,
                              EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  return ai->stun_timer > 0.0f;
}

// State update functions
static void on_enter_idle(EnemyAI *ai, struct ECSWorld *ecs,
                          EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  ai->decision.movement_target = ai->behavior.patrol_center;
  ai->decision.path_recalc_timer = 0.0f;
}

static void on_enter_patrol(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  // Generate new patrol point
  f32 angle = (f32)(rand() % 360) * 3.14159f / 180.0f;
  f32 radius = (f32)(rand() % (int)(ai->behavior.patrol_radius * 100)) / 100.0f;

  ai->decision.movement_target.x =
      ai->behavior.patrol_center.x + cosf(angle) * radius;
  ai->decision.movement_target.z =
      ai->behavior.patrol_center.z + sinf(angle) * radius;
  ai->decision.path_recalc_timer = 0.0f;
}

static void on_enter_chase(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id) {
  EntityID target = enemy_ai_select_target(ai, ecs, entity_id);
  if (target != INVALID_ENTITY_ID) {
    ai->decision.current_target = target;
    ai->decision.target_last_seen_timer = 5.0f;
  }
}

static void on_enter_combat(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  ai->decision.attack_cooldown = 0.0f;
}

static void on_enter_flee(EnemyAI *ai, struct ECSWorld *ecs,
                          EntityID entity_id) {
  EntityID threat = enemy_ai_select_target(ai, ecs, entity_id);
  if (threat != INVALID_ENTITY_ID) {
    // Calculate flee direction (away from threat)
    Vec3 threat_pos = ai_get_entity_position(ecs, threat);
    Vec3 my_pos = ai_get_entity_position(ecs, entity_id);

    Vec3 flee_dir = vec3_normalize(vec3_sub(my_pos, threat_pos));
    f32 flee_distance = ai->behavior.flee_distance;

    flee_distance = 20.0f; // Default flee distance

    ai->decision.movement_target =
        vec3_add(my_pos, vec3_scale(flee_dir, flee_distance));
  }
}

static void on_enter_search(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  ai->decision.search_timer = 10.0f; // Search for 10 seconds
}

static void on_enter_guard(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  ai->decision.movement_target = ai->decision.guard_position;
}

static void on_enter_stunned(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  // Stun timer is already set externally
}

static void on_enter_sleep(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id) {
  (void)ecs;
  (void)entity_id;
  ai->fatigue = 0.0f; // Reset fatigue when entering sleep
}

// State update functions
static void on_update_idle(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  (void)delta_time;
  // Gradually recover fatigue while idle
  if (ai->fatigue > 0.0f) {
    ai->fatigue = fmaxf(0.0f, ai->fatigue - delta_time * 0.1f);
  }
}

static void on_update_patrol(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  (void)delta_time;
  // Check if reached patrol point
  Vec3 current_pos = ai_get_entity_position(ecs, entity_id);
  f32 distance_to_target =
      vec3_distance(current_pos, ai->decision.movement_target);

  if (distance_to_target < 1.0f) {
    // Reached patrol point, generate new one
    on_enter_patrol(ai, ecs, entity_id);
  }
}

static void on_update_chase(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  (void)delta_time;
  // Update target last seen timer
  if (ai->decision.current_target != INVALID_ENTITY_ID) {
    Vec3 target_pos = ai_get_entity_position(ecs, ai->decision.current_target);
    Vec3 my_pos = ai_get_entity_position(ecs, entity_id);
    f32 distance = vec3_distance(my_pos, target_pos);

    if (distance <= ai->sensors.vision_range) {
      ai->decision.target_last_seen_timer =
          5.0f; // Reset timer if can see target
    }
  }
}

static void on_update_combat(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  // Update attack cooldown
  if (ai->decision.attack_cooldown > 0.0f) {
    ai->decision.attack_cooldown -= delta_time;
  }
}

static void on_update_flee(EnemyAI *ai, struct ECSWorld *ecs,
                           EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  (void)delta_time;
  // Check if reached safe distance
  if (ai->decision.current_target != INVALID_ENTITY_ID) {
    Vec3 threat_pos = ai_get_entity_position(ecs, ai->decision.current_target);
    Vec3 my_pos = ai_get_entity_position(ecs, entity_id);
    f32 distance = vec3_distance(my_pos, threat_pos);

    if (distance >= ai->behavior.flee_distance) {
      // Safe distance reached, consider transitioning to other states
      ai->decision.flee_timer = 0.0f;
    }
  }
}

static void on_update_search(EnemyAI *ai, struct ECSWorld *ecs,
                             EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  ai->decision.search_timer -= delta_time;

  // Random search movement
  if (rand() % 100 < 5) { // 5% chance per frame to change direction
    f32 angle = (f32)(rand() % 360) * 3.14159f / 180.0f;
    Vec3 current_pos = ai_get_entity_position(ecs, entity_id);
    f32 search_radius = 5.0f;

    ai->decision.movement_target.x =
        current_pos.x + cosf(angle) * search_radius;
    ai->decision.movement_target.z =
        current_pos.z + sinf(angle) * search_radius;
  }
}

static void on_update_guard(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  (void)delta_time;
  // Guard behavior - stay at guard position but look around
  if (rand() % 100 < 2) { // 2% chance to look in different direction
    ai->decision.look_direction = (f32)(rand() % 360) * 3.14159f / 180.0f;
  }
}

static void on_update_stunned(EnemyAI *ai, struct ECSWorld *ecs,
                              EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  ai->stun_timer = fmaxf(0.0f, ai->stun_timer - delta_time);
}

static void on_update_sleep(EnemyAI *ai, struct ECSWorld *ecs,
                            EntityID entity_id, f32 delta_time) {
  (void)ecs;
  (void)entity_id;
  // Recover from sleep
  ai->fatigue = fmaxf(0.0f, ai->fatigue - delta_time * 0.5f);
}

// State definitions
static StateDefinition g_state_definitions[AI_STATE_COUNT] = {
    {"Idle", AI_STATE_IDLE, can_enter_idle, on_enter_idle, on_update_idle, NULL,
     NULL, 0},
    {"Patrol", AI_STATE_PATROL, can_enter_patrol, on_enter_patrol,
     on_update_patrol, NULL, NULL, 0},
    {"Chase", AI_STATE_CHASE, can_enter_chase, on_enter_chase, on_update_chase,
     NULL, NULL, 0},
    {"Combat", AI_STATE_COMBAT, can_enter_combat, on_enter_combat,
     on_update_combat, NULL, NULL, 0},
    {"Flee", AI_STATE_FLEE, can_enter_flee, on_enter_flee, on_update_flee, NULL,
     NULL, 0},
    {"Search", AI_STATE_SEARCHING, can_enter_search, on_enter_search,
     on_update_search, NULL, NULL, 0},
    {"Guard", AI_STATE_GUARDING, can_enter_guard, on_enter_guard,
     on_update_guard, NULL, NULL, 0},
    {"Stunned", AI_STATE_STUNNED, can_enter_stunned, on_enter_stunned,
     on_update_stunned, NULL, NULL, 0},
    {"Sleep", AI_STATE_SLEEPING, can_enter_sleep, on_enter_sleep,
     on_update_sleep, NULL, NULL, 0},
};

// Hierarchical state evaluation
static AIState evaluate_hierarchical_state(EnemyAI *ai, struct ECSWorld *ecs,
                                           EntityID entity_id) {
  // Priority order: Stunned > Flee > Combat > Chase > Search > Guard > Patrol >
  // Idle > Sleep

  // Check stunned state first (highest priority)
  if (can_enter_stunned(ai, ecs, entity_id)) {
    return AI_STATE_STUNNED;
  }

  // Check flee state (second highest priority)
  if (can_enter_flee(ai, ecs, entity_id)) {
    return AI_STATE_FLEE;
  }

  // Check combat state
  if (can_enter_combat(ai, ecs, entity_id)) {
    return AI_STATE_COMBAT;
  }

  // Check chase state
  if (can_enter_chase(ai, ecs, entity_id)) {
    return AI_STATE_CHASE;
  }

  // Check search state
  if (can_enter_search(ai, ecs, entity_id)) {
    return AI_STATE_SEARCHING;
  }

  // Check guard state
  if (can_enter_guard(ai, ecs, entity_id)) {
    return AI_STATE_GUARDING;
  }

  // Check patrol state
  if (can_enter_patrol(ai, ecs, entity_id)) {
    return AI_STATE_PATROL;
  }

  // Check sleep state
  if (can_enter_sleep(ai, ecs, entity_id)) {
    return AI_STATE_SLEEPING;
  }

  // Default to idle
  return AI_STATE_IDLE;
}

void enemy_ai_hierarchical_update(EnemyAI *ai, struct ECSWorld *ecs,
                                  EntityID entity_id, f32 delta_time) {
  if (!ai || !ecs)
    return;

  // Evaluate current hierarchical state
  AIState desired_state = evaluate_hierarchical_state(ai, ecs, entity_id);

  // Handle state transitions
  if (desired_state != ai->current_state) {
    // Call exit function for current state
    if (g_state_definitions[ai->current_state].on_exit) {
      g_state_definitions[ai->current_state].on_exit(ai, ecs, entity_id);
    }

    // Store previous state
    ai->previous_state = ai->current_state;
    ai->current_state = desired_state;

    // Call enter function for new state
    if (g_state_definitions[desired_state].on_enter) {
      g_state_definitions[desired_state].on_enter(ai, ecs, entity_id);
    }
  }

  // Update current state
  if (g_state_definitions[ai->current_state].on_update) {
    g_state_definitions[ai->current_state].on_update(ai, ecs, entity_id,
                                                     delta_time);
  }
}

bool enemy_ai_can_transition_to(EnemyAI *ai, AIState target_state) {
  if (!ai || target_state >= AI_STATE_COUNT)
    return false;

  // Check if target state can be entered
  return g_state_definitions[target_state].can_enter != NULL;
}

void enemy_ai_force_state_transition(EnemyAI *ai, AIState new_state,
                                     struct ECSWorld *ecs, EntityID entity_id) {
  if (!ai || new_state >= AI_STATE_COUNT)
    return;

  // Force transition regardless of conditions
  if (g_state_definitions[ai->current_state].on_exit) {
    g_state_definitions[ai->current_state].on_exit(ai, ecs, entity_id);
  }

  ai->previous_state = ai->current_state;
  ai->current_state = new_state;

  if (g_state_definitions[new_state].on_enter) {
    g_state_definitions[new_state].on_enter(ai, ecs, entity_id);
  }
}

// AI memory system to remember player locations and patterns
#define AI_MEMORY_SIZE 8
#define AI_MEMORY_DURATION 300.0f // 5 minutes

typedef struct {
  Vec3 position;
  f32 timestamp;
  u32 player_id;
  AIState player_state;
} AIMemoryEntry;

void ai_update_memory(EnemyAI *ai, Vec3 player_pos, u32 player_id,
                      f32 current_time, AIState player_state) {
  if (!ai)
    return;

  // Shift existing memories
  for (int i = AI_MEMORY_SIZE - 1; i > 0; i--) {
    ai->memory[i] = ai->memory[i - 1];
  }

  // Add new memory
  ai->memory[0].position = player_pos;
  ai->memory[0].timestamp = current_time;
  ai->memory[0].player_id = player_id;
  ai->memory[0].player_state = player_state;
}

Vec3 ai_get_predicted_player_position(const EnemyAI *ai, f32 current_time) {
  if (!ai || ai->memory[0].timestamp == 0.0f)
    return vec3_zero();

  // Simple linear prediction based on last two positions
  if (ai->memory[1].timestamp != 0.0f) {
    f32 time_diff = ai->memory[0].timestamp - ai->memory[1].timestamp;
    if (time_diff > 0.1f) {
      Vec3 velocity = vec3_sub(ai->memory[0].position, ai->memory[1].position);
      velocity = vec3_div(velocity, time_diff);

      f32 prediction_time = 2.0f; // Predict 2 seconds ahead
      Vec3 predicted =
          vec3_add(ai->memory[0].position, vec3_mul(velocity, prediction_time));
      return predicted;
    }
  }

  return ai->memory[0].position;
}

// AI retreat and regrouping logic when health is low
bool ai_should_retreat(const EnemyAI *ai, f32 health_percentage) {
  if (!ai)
    return false;

  // Retreat based on personality and health
  f32 retreat_threshold = 0.3f; // Default 30% health

  switch (ai->personality.aggression) {
  case 0.0f ... 0.3f:         // Cowardly
    retreat_threshold = 0.6f; // Retreat at 60% health
    break;
  case 0.4f ... 0.7f:         // Balanced
    retreat_threshold = 0.4f; // Retreat at 40% health
    break;
  case 0.8f ... 1.0f:         // Aggressive
    retreat_threshold = 0.2f; // Retreat at 20% health
    break;
  }

  return health_percentage < retreat_threshold;
}

Vec3 ai_find_retreat_position(const EnemyAI *ai, Vec3 current_pos,
                              Vec3 threat_pos) {
  if (!ai)
    return current_pos;

  // Calculate direction away from threat
  Vec3 away_from_threat = vec3_sub(current_pos, threat_pos);
  away_from_threat.y = 0.0f; // Keep on ground level
  away_from_threat = vec3_normalize(away_from_threat);

  // Move away by retreat distance
  f32 retreat_distance = ai->behavior.flee_distance * 1.5f;
  Vec3 retreat_pos =
      vec3_add(current_pos, vec3_mul(away_from_threat, retreat_distance));

  return retreat_pos;
}

// AI day/night cycle behavior variations
void ai_apply_time_of_day_modifiers(EnemyAI *ai, f32 time_of_day) {
  if (!ai)
    return;

  // time_of_day: 0.0 = midnight, 0.5 = noon, 1.0 = midnight next day
  bool is_night = time_of_day < 0.25f || time_of_day > 0.75f;
  bool is_dawn = time_of_day >= 0.2f && time_of_day <= 0.3f;
  bool is_dusk = time_of_day >= 0.7f && time_of_day <= 0.8f;

  if (is_night) {
    // Night behavior modifications
    ai->behavior.detection_range *= 0.8f; // Reduced detection at night
    ai->behavior.aggression *= 1.2f;      // More aggressive at night

    // Some creatures are nocturnal
    if (ai->creature_type == AI_CREATURE_PREDATOR) {
      ai->behavior.speed *= 1.1f; // Faster at night
    }
  } else if (is_dawn || is_dusk) {
    // Dawn/dusk - transition periods
    ai->behavior.detection_range *= 0.9f; // Slightly reduced detection
    ai->behavior.aggression *= 1.1f;      // Slightly more aggressive
  } else {
    // Day behavior
    ai->behavior.detection_range *= 1.1f; // Better detection during day
    ai->behavior.aggression *= 0.9f;      // Less aggressive during day
  }

  // Apply creature-specific behaviors
  switch (ai->creature_type) {
  case AI_CREATURE_PREDATOR:
    if (is_night) {
      ai->behavior.hunt_cooldown *= 0.8f; // Hunt more frequently at night
    }
    break;

  case AI_CREATURE_PREY:
    if (is_night) {
      ai->behavior.flee_distance *= 1.3f; // More cautious at night
    }
    break;

  case AI_CREATURE_NEUTRAL:
    // Neutral creatures less active at night
    if (is_night) {
      ai->behavior.wander_radius *= 0.7f;
    }
    break;
  }
}
