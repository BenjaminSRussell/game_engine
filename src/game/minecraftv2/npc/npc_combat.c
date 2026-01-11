// NPC combat system implementation with damage, knockback, death, and loot.
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/npc.h>
#include <ecs/components/rigidbody.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <inventory/inventory.h>
#include <math.h>
#include <math/vec3.h>
#include <npc/npc.h>
#include <npc/npc_combat.h>
#include <physics/physics.h>
#include <player/player.h>
#include <stdlib.h>

// Loot tables for each NPC type
static const LootTable ZOMBIE_LOOT = {
    .drops =
        {
            {.item_id = 10, .count = 1, .drop_chance = 0.5f},  // Rotten flesh
            {.item_id = 5, .count = 1, .drop_chance = 0.025f}, // Iron ingot
        },
    .drop_count = 2,
    .min_exp = 5,
    .max_exp = 5};

static const LootTable SKELETON_LOOT = {
    .drops =
        {
            {.item_id = 11, .count = 2, .drop_chance = 1.0f}, // Bones
            {.item_id = 12, .count = 2, .drop_chance = 0.8f}, // Arrows
        },
    .drop_count = 2,
    .min_exp = 5,
    .max_exp = 5};

static const LootTable CREEPER_LOOT = {
    .drops =
        {
            {.item_id = 13, .count = 2, .drop_chance = 1.0f}, // Gunpowder
        },
    .drop_count = 1,
    .min_exp = 5,
    .max_exp = 5};

static const LootTable COW_LOOT = {
    .drops =
        {
            {.item_id = 20, .count = 2, .drop_chance = 1.0f}, // Leather
            {.item_id = 21, .count = 3, .drop_chance = 1.0f}, // Raw beef
        },
    .drop_count = 2,
    .min_exp = 1,
    .max_exp = 3};

static const LootTable PIG_LOOT = {
    .drops =
        {
            {.item_id = 22, .count = 2, .drop_chance = 1.0f}, // Raw porkchop
        },
    .drop_count = 1,
    .min_exp = 1,
    .max_exp = 3};

static const LootTable CHICKEN_LOOT = {
    .drops =
        {
            {.item_id = 23, .count = 1, .drop_chance = 1.0f}, // Raw chicken
            {.item_id = 24, .count = 1, .drop_chance = 1.0f}, // Feather
        },
    .drop_count = 2,
    .min_exp = 1,
    .max_exp = 3};

static const LootTable VILLAGER_LOOT = {
    .drops = {}, .drop_count = 0, .min_exp = 0, .max_exp = 0};

// Get loot table for NPC type
const LootTable *npc_get_loot_table(NPCType type) {
  switch (type) {
  case NPC_TYPE_ZOMBIE:
    return &ZOMBIE_LOOT;
  case NPC_TYPE_SKELETON:
    return &SKELETON_LOOT;
  case NPC_TYPE_CREEPER:
    return &CREEPER_LOOT;
  case NPC_TYPE_COW:
    return &COW_LOOT;
  case NPC_TYPE_PIG:
    return &PIG_LOOT;
  case NPC_TYPE_CHICKEN:
    return &CHICKEN_LOOT;
  case NPC_TYPE_VILLAGER:
    return &VILLAGER_LOOT;
  default:
    return &VILLAGER_LOOT;
  }
}

// Calculate damage based on attacker type
f32 npc_calculate_damage(NPCType attacker_type, DamageType damage_type) {
  NPCStats stats = npc_get_stats(attacker_type);
  f32 base_damage = stats.damage;

  // Damage type modifiers
  switch (damage_type) {
  case DAMAGE_TYPE_FIRE:
    return base_damage * 1.5f;
  case DAMAGE_TYPE_EXPLOSION:
    return base_damage * 2.0f;
  case DAMAGE_TYPE_MAGIC:
    return base_damage * 1.2f;
  default:
    return base_damage;
  }
}

// Apply knockback to NPC
void npc_apply_knockback(struct NPCSystem *system, EntityID entity,
                         Vec3 direction, f32 force) {
  RigidBodyComponent *rb_comp = (RigidBodyComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, RIGIDBODY_COMPONENT_ID);
  if (!rb_comp || !rb_comp->body)
    return;
  RigidBody *rb = rb_comp->body;

  // Normalize direction and apply force
  Vec3 knockback = vec3_normalize(direction);
  knockback = vec3_mul(knockback, force);
  knockback.y = force * 0.5f; // Add upward component

  Vec3 current_velocity = rigid_body_get_velocity(rb);
  Vec3 new_velocity = vec3_add(current_velocity, knockback);
  rigid_body_set_velocity(rb, new_velocity);
}

// Spawn loot items at position
void npc_spawn_loot(struct NPCSystem *system, Vec3 position,
                    const LootTable *loot_table) {
  if (!system || !loot_table)
    return;

  // Try to give drops to nearest player within pickup radius (simple fallback)
  QueryDesc desc = {0};
  ComponentType player_components[] = {PLAYER_COMPONENT_ID,
                                       TRANSFORM_COMPONENT_ID};
  desc.all_components = player_components;
  desc.all_count = 2;
  Query *query = ecs_query_create((World *)system->ecs, &desc);

  EntityID nearest_player = 0;
  f32 nearest_dist = 999999.0f;
  Entity p_entity;
  void *p_comps[2];
  while (ecs_query_next(query, &p_entity, p_comps)) {
    EntityID player_id = p_entity.id;
    TransformComponent *pt = (TransformComponent *)p_comps[1];
    if (!pt)
      continue;
    f32 d = vec3_length(vec3_sub(pt->position, position));
    if (d < nearest_dist) {
      nearest_dist = d;
      nearest_player = player_id;
    }
  }

  ecs_query_destroy((World *)system->ecs, query);
  const f32 pickup_radius = 5.0f;
  for (u32 i = 0; i < loot_table->drop_count; i++) {
    const LootDrop *drop = &loot_table->drops[i];

    // Roll for drop chance
    f32 roll = (rand() % 1000) / 1000.0f;
    if (roll > drop->drop_chance)
      continue;

    if (nearest_player != 0 && nearest_dist <= pickup_radius) {
      // Give directly to nearest player's inventory
      PlayerComponent *player = (PlayerComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){nearest_player, 0},
          PLAYER_COMPONENT_ID);
      if (player) {
        bool added = inventory_add_item(&player->inventory, drop->item_id,
                                        (u16)drop->count);
        if (added) {
          LOG_INFO("Player %u picked up %u x item %u from NPC loot",
                   nearest_player, drop->count, drop->item_id);
          continue;
        } else {
          LOG_WARN("Player %u inventory full; cannot add item %u",
                   nearest_player, drop->item_id);
        }
      }
    }

    // Fallback: log dropping into world until entity spawner is wired.
    LOG_INFO("Dropped item %u x%u at (%f,%f,%f)", drop->item_id, drop->count,
             position.x, position.y, position.z);
  }
}

// Spawn experience orbs
void npc_spawn_experience(struct NPCSystem *system, Vec3 position, u32 amount) {
  if (!system || amount == 0)
    return;

  // Give experience to nearest player (simple fallback)
  QueryDesc exp_desc = {0};
  ComponentType player_components[] = {PLAYER_COMPONENT_ID,
                                       TRANSFORM_COMPONENT_ID};
  exp_desc.all_components = player_components;
  exp_desc.all_count = 2;
  Query *exp_query = ecs_query_create((World *)system->ecs, &exp_desc);

  EntityID nearest_player = 0;
  f32 nearest_dist = 999999.0f;
  Entity p_entity_exp;
  void *p_comps_exp[2];
  while (ecs_query_next(exp_query, &p_entity_exp, p_comps_exp)) {
    EntityID player_id = p_entity_exp.id;
    TransformComponent *pt = (TransformComponent *)p_comps_exp[1];
    if (!pt)
      continue;
    f32 d = vec3_length(vec3_sub(pt->position, position));
    if (d < nearest_dist) {
      nearest_dist = d;
      nearest_player = player_id;
    }
  }

  ecs_query_destroy((World *)system->ecs, exp_query);
  const f32 pickup_radius = 5.0f;
  if (nearest_player != 0 && nearest_dist <= pickup_radius) {
    PlayerComponent *player = (PlayerComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){nearest_player, 0}, PLAYER_COMPONENT_ID);
    if (player) {
      player->experience += amount;
      LOG_INFO("Gave %u XP to player %u", amount, nearest_player);
    }
  } else {
    LOG_INFO("XP orbs (%u XP) dropped at (%f,%f,%f)", amount, position.x,
             position.y, position.z);
  }

  ecs_query_free(&query);
}

// Handle NPC death
void npc_on_death(struct NPCSystem *system, EntityID entity, Vec3 position,
                  NPCType type) {
  if (!system)
    return;

  // Get loot table
  const LootTable *loot = npc_get_loot_table(type);

  // Spawn loot
  npc_spawn_loot(system, position, loot);

  // Spawn experience
  u32 exp = loot->min_exp + (rand() % (loot->max_exp - loot->min_exp + 1));
  npc_spawn_experience(system, position, exp);

  // Play death sound (placeholder) - hook into AudioSystem when available.
  LOG_INFO("NPC death: type=%d at (%f,%f,%f) - playing death sound", type,
           position.x, position.y, position.z);

  // Spawn death particles (placeholder).
  LOG_INFO("NPC death: spawning particles at (%f,%f,%f)", position.x,
           position.y, position.z);

  // Destroy entity
  ecs_destroy_entity((World *)system->ecs, (Entity){entity, 0});
}

// Apply damage to NPC
bool npc_take_damage(struct NPCSystem *system, EntityID entity,
                     DamageInfo *damage) {
  if (!system || !damage)
    return false;

  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, HEALTH_COMPONENT_ID);
  if (!health)
    return false;

  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return false;

  npc->last_attacker = damage->source;
  npc->time_since_last_attacked = 0.0f;

  // Apply damage
  health->health -= damage->amount;

  // Apply knockback
  if (damage->knockback_force > 0.0f) {
    npc_apply_knockback(system, entity, damage->knockback_direction,
                        damage->knockback_force);
  }

  // Check for death
  if (health->health <= 0.0f) {
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
    Vec3 death_pos = transform ? transform->position : vec3(0, 0, 0);
    npc_on_death(system, entity, death_pos, npc->type);
    return true; // Entity died
  }

  // Trigger flee/anger for certain NPCs
  NPCStats stats = npc_get_stats(npc->type);
  if (stats.behavior == NPC_BEHAVIOR_PASSIVE) {
    npc->state = NPC_STATE_FLEEING;
    npc->panic_timer = 5.0f;
    npc->flee_target = damage->source;
  } else if (stats.behavior == NPC_BEHAVIOR_NEUTRAL) {
    npc->behavior_flags |= NPC_FLAG_ANGRY;
    npc->state = NPC_STATE_FLEEING;
    npc->panic_timer = 10.0f;
    npc->flee_target = damage->source;
  }

  void *player_comp = ecs_get_component(
      (World *)system->ecs, (Entity){damage->source, 0}, PLAYER_COMPONENT_ID);
  if (player_comp && stats.behavior != NPC_BEHAVIOR_HOSTILE) {
    npc->reputation -= 10;
    bool found = false;
    for (u8 i = 0; i < npc->relations_count; i++) {
      if (npc->relations_entities[i] == damage->source) {
        npc->relations_values[i] -= 10;
        if (npc->relations_values[i] < -100)
          npc->relations_values[i] = -100;
        found = true;
        break;
      }
    }
    if (!found) {
      u8 idx = npc->relations_count < 8 ? npc->relations_count : 0;
      npc->relations_entities[idx] = damage->source;
      npc->relations_values[idx] = -10;
      if (npc->relations_count < 8)
        npc->relations_count++;
    }
  }

  return false; // Entity survived
}

// Melee attack from NPC to target
bool npc_melee_attack(struct NPCSystem *system, EntityID attacker,
                      EntityID target) {
  if (!system)
    return false;

  NPCComponent *attacker_npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){attacker, 0}, NPC_COMPONENT_ID);
  if (!attacker_npc)
    return false;

  TransformComponent *attacker_transform =
      (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){attacker, 0}, TRANSFORM_COMPONENT_ID);
  TransformComponent *target_transform =
      (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){target, 0}, TRANSFORM_COMPONENT_ID);
  if (!attacker_transform || !target_transform)
    return false;

  // Calculate damage
  f32 damage_amount =
      npc_calculate_damage(attacker_npc->type, DAMAGE_TYPE_PHYSICAL);

  // Calculate knockback direction
  Vec3 direction =
      vec3_sub(target_transform->position, attacker_transform->position);

  // Create damage info
  DamageInfo damage = {.amount = damage_amount,
                       .type = DAMAGE_TYPE_PHYSICAL,
                       .source = attacker,
                       .knockback_direction = direction,
                       .knockback_force = 5.0f};

  // Check if target is player
  void *player_comp = ecs_get_component(
      (World *)system->ecs, (Entity){target, 0}, PLAYER_COMPONENT_ID);
  if (player_comp) {
    // Apply damage directly to player's health component
    HealthComponent *phealth = (HealthComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){target, 0}, HEALTH_COMPONENT_ID);
    if (phealth) {
      phealth->health -= damage_amount;
      LOG_INFO("Player %u took %f damage from NPC %u", target, damage_amount,
               attacker);
      if (phealth->health <= 0.0f) {
        phealth->health = 0.0f;
        LOG_INFO("Player %u has died! Game over.", target);
      }

      // Apply knockback to player
      if (damage.knockback_force > 0.0f) {
        npc_apply_knockback(system, target, damage.knockback_direction,
                            damage.knockback_force);
      }
    }
    return true;
  }

  // Check if target is another NPC
  NPCComponent *target_npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){target, 0}, NPC_COMPONENT_ID);
  if (target_npc) {
    return npc_take_damage(system, target, &damage);
  }

  return false;
}
