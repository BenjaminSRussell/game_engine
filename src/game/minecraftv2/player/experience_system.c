#include <block/block.h>
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <inventory/inventory.h>
#include <player/experience_system.h>

#ifndef EXPERIENCE_SYSTEM_TEST
#include <audio/audio_system.h>
#include <core/logger.h>
#include <core/memory.h>
#include <math/math.h>
#include <rendering/renderer.h>
#else
#include <stdio.h>
#include <stdlib.h>
#define LOG_INFO(...)                                                          \
  do {                                                                         \
  } while (0)
#define LOG_DEBUG(...)                                                         \
  do {                                                                         \
  } while (0)
#define LOG_WARN(...)                                                          \
  do {                                                                         \
  } while (0)
#define core_alloc malloc
#define core_free free
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global experience system instance
static ExperienceSystem g_experience_system = {0};

// Experience source names
static const char *EXPERIENCE_SOURCE_NAMES[EXP_SOURCE_COUNT] = {
    "Mining",  "Crafting",   "Combat",  "Exploration", "Smelting", "Fishing",
    "Farming", "Enchanting", "Brewing", "Building",    "Trade",    "Quest"};

// Skill names
static const char *SKILL_NAMES[SKILL_COUNT] = {"Sword Mastery",
                                               "Axe Mastery",
                                               "Archery",
                                               "Unarmed Combat",
                                               "Shield Mastery",
                                               "Critical Strikes",
                                               "Mining Speed",
                                               "Mining Fortune",
                                               "Excavation",
                                               "Gem Finding",
                                               "Crafting Efficiency",
                                               "Smithing",
                                               "Enchanting Proficiency",
                                               "Alchemy",
                                               "Harvesting",
                                               "Crop Yield",
                                               "Animal Husbandry",
                                               "Spell Power",
                                               "Mana Efficiency",
                                               "Summoning",
                                               "Elemental Mastery",
                                               "Endurance",
                                               "Foraging",
                                               "Stealth",
                                               "First Aid"};

// Skill category names
static const char *SKILL_CATEGORY_NAMES[SKILL_CATEGORY_COUNT] = {
    "Combat", "Mining", "Crafting", "Farming", "Magic", "Archery", "Survival"};

// Base experience amounts for different sources
static const float EXPERIENCE_BASE_AMOUNTS[EXP_SOURCE_COUNT] = {
    1.0f, // Mining
    2.0f, // Crafting
    5.0f, // Combat
    3.0f, // Exploration
    1.5f, // Smelting
    2.5f, // Fishing
    1.0f, // Farming
    3.0f, // Enchanting
    2.0f, // Brewing
    0.5f, // Building
    4.0f, // Trade
    10.0f // Quest
};

// Experience required for levels (Minecraft-style formula)
static float experience_required_for_level(int level) {
  if (level <= 0)
    return 0.0f;
  if (level <= 16) {
    return level * level + 6 * level;
  } else if (level <= 31) {
    return 2.5f * level * level - 40.5f * level + 360.0f;
  } else {
    return 4.5f * level * level - 162.5f * level + 2220.0f;
  }
}

bool experience_system_init(ExperienceSystem *system, World *ecs_world) {
  if (!system || !ecs_world)
    return false;

  memset(system, 0, sizeof(ExperienceSystem));
  system->ecs_world = ecs_world;

  // Initialize experience orbs array
  system->max_orbs = 256;
  system->experience_orbs =
      (ExperienceOrb *)core_alloc(system->max_orbs * sizeof(ExperienceOrb));
  if (!system->experience_orbs)
    return false;
  memset(system->experience_orbs, 0, system->max_orbs * sizeof(ExperienceOrb));

  // Initialize skill trees
  for (int i = 0; i < SKILL_CATEGORY_COUNT; i++) {
    skill_tree_init(&system->skill_trees[i], (SkillCategory)i);
  }

  // Set default options
  system->enable_orb_effects = true;
  system->enable_level_up_effects = true;
  system->enable_skill_notifications = true;
  system->orb_attraction_range = 8.0f;
  system->orb_collection_speed = 15.0f;

  // Reset statistics
  memset(&system->stats, 0, sizeof(ExperienceStats));
  system->stats.session_start_time = (uint64_t)(time(NULL) * 1000);

  LOG_INFO("Experience system initialized");
  return true;
}

void experience_system_cleanup(ExperienceSystem *system) {
  if (!system)
    return;

  if (system->experience_orbs) {
    core_free(system->experience_orbs);
    system->experience_orbs = NULL;
  }

  // Cleanup skill trees
  for (int i = 0; i < SKILL_CATEGORY_COUNT; i++) {
    skill_tree_cleanup(&system->skill_trees[i]);
  }

  memset(system, 0, sizeof(ExperienceSystem));
  LOG_INFO("Experience system cleaned up");
}

void experience_system_update(ExperienceSystem *system, float delta_time) {
  if (!system)
    return;

  // Update experience orbs
  experience_orb_update(system, delta_time);

  // Update total play time
  system->stats.total_play_time += (uint64_t)(delta_time * 1000);
}

/* Simple per-player experience tracking (in-memory). This allows the experience
   system to operate without requiring the full ECS player component to exist.
 */

static ExperiencePlayerEntry *g_player_entries = NULL;
static u32 g_player_entry_count = 0;

static ExperiencePlayerEntry *get_or_create_player_entry(EntityID entity) {
  for (u32 i = 0; i < g_player_entry_count; i++) {
    if (g_player_entries[i].entity == entity)
      return &g_player_entries[i];
  }
  ExperiencePlayerEntry *tmp = (ExperiencePlayerEntry *)realloc(
      g_player_entries,
      (g_player_entry_count + 1) * sizeof(ExperiencePlayerEntry));
  if (!tmp)
    return NULL;
  g_player_entries = tmp;
  g_player_entries[g_player_entry_count].entity = entity;
  g_player_entries[g_player_entry_count].experience = 0.0f;
  g_player_entries[g_player_entry_count].level = 0;
  g_player_entries[g_player_entry_count].skill_points = 0;
  g_player_entry_count++;
  return &g_player_entries[g_player_entry_count - 1];
}

bool experience_add_experience_to_player(ExperienceSystem *system,
                                         EntityID entity, float amount) {
  if (!system || amount <= 0.0f)
    return false;
  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry)
    return false;

  entry->experience += amount;

  int new_level = experience_get_level_for_experience(entry->experience);
  if (new_level > entry->level) {
    int gained = new_level - entry->level;
    entry->skill_points += gained;
    entry->level = new_level;
    system->stats.levels_gained += gained;
    system->stats.skill_points_earned += gained;
    // Trigger level-up effects for entity
    if (system->enable_level_up_effects) {
      experience_trigger_level_up_effects(system, entity);
    }
  }
  return true;
}

ExperiencePlayerEntry *experience_get_player_entry(ExperienceSystem *system,
                                                   EntityID entity) {
  (void)system;
  for (u32 i = 0; i < g_player_entry_count; i++) {
    if (g_player_entries[i].entity == entity)
      return &g_player_entries[i];
  }
  return NULL;
}

bool experience_add_experience(ExperienceSystem *system, EntityID entity,
                               float amount, ExperienceSource source) {
  if (!system || amount <= 0.0f || source >= EXP_SOURCE_COUNT)
    return false;

  if (!experience_is_valid_amount(amount)) {
    return false;
  }

  if (!experience_add_experience_to_player(system, entity, amount)) {
    return false;
  }

  // Update statistics
  system->stats.experience_gained[source] += amount;

  // Spawn experience orbs
  if (system->enable_orb_effects) {
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        system->ecs_world, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
    if (transform) {
      experience_orb_spawn_multiple(system, transform->position, amount,
                                    (uint32_t)(amount / 10.0f) + 1);
    }
  }

  LOG_DEBUG("Added %.1f experience from %s", amount,
            experience_source_get_name(source));
  return true;
}

bool experience_level_up(ExperienceSystem *system, EntityID entity) {
  if (!system)
    return false;

  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry)
    return false;

  entry->level += 1;
  entry->skill_points += 1;
  system->stats.levels_gained++;
  system->stats.skill_points_earned++;

  if (system->enable_level_up_effects) {
    experience_trigger_level_up_effects(system, entity);
  }

  LOG_INFO("Player leveled up to %d", entry->level);
  return true;
}

float experience_get_experience_for_level(int level) {
  return experience_required_for_level(level);
}

bool experience_set_level(ExperienceSystem *system, EntityID entity,
                          int level) {
  if (!system)
    return false;
  if (level < 0)
    level = 0;

  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry)
    return false;

  float total_exp = 0.0f;
  for (int i = 1; i <= level; i++) {
    total_exp += experience_required_for_level(i);
  }

  int gained = level - entry->level;
  if (gained > 0) {
    entry->skill_points += gained;
    system->stats.levels_gained += gained;
    system->stats.skill_points_earned += gained;
    if (system->enable_level_up_effects) {
      experience_trigger_level_up_effects(system, entity);
    }
  } else if (gained < 0 && entry->skill_points > 0) {
    int reduce = -gained;
    entry->skill_points =
        (entry->skill_points > reduce) ? (entry->skill_points - reduce) : 0;
  }

  entry->level = level;
  entry->experience = total_exp;
  return true;
}

int experience_get_level_for_experience(float experience) {
  if (experience < 0.0f)
    return 0;

  int level = 0;
  float total_exp = 0.0f;

  while (total_exp <= experience) {
    level++;
    total_exp += experience_required_for_level(level);

    // Prevent infinite loop
    if (level > 1000)
      break;
  }

  return level - 1;
}

float experience_get_progress_to_next_level(const PlayerLevel *level_data) {
  if (!level_data || level_data->experience_to_next <= 0.0f)
    return 0.0f;
  float progress = level_data->experience / level_data->experience_to_next;
  if (progress < 0.0f)
    progress = 0.0f;
  if (progress > 1.0f)
    progress = 1.0f;
  return progress;
}

void experience_orb_spawn(ExperienceSystem *system, Vec3 position, float amount,
                          EntityID source) {
  if (!system || amount <= 0.0f)
    return;
  (void)source;

  // Find available orb slot
  for (uint32_t i = 0; i < system->max_orbs; i++) {
    ExperienceOrb *orb = &system->experience_orbs[i];
    if (orb->lifetime <= 0.0f) {
      // Initialize orb
      orb->position = position;
      orb->velocity = vec3_zero();
      orb->experience_amount = amount;
      orb->lifetime = 60.0f; // 60 seconds lifetime
      orb->collected = false;
      orb->target_entity = 0;
      orb->attraction_range = system->orb_attraction_range;
      orb->attraction_speed = system->orb_collection_speed;

      // Set color based on amount
      if (amount < 5.0f) {
        orb->color = 0x00FF00; // Green
      } else if (amount < 20.0f) {
        orb->color = 0x00FFFF; // Cyan
      } else {
        orb->color = 0xFFFF00; // Yellow
      }

      system->active_orbs++;
      return;
    }
  }
}

void experience_orb_spawn_multiple(ExperienceSystem *system, Vec3 position,
                                   float total_amount, uint32_t count) {
  if (!system || total_amount <= 0.0f || count == 0)
    return;

  float amount_per_orb = total_amount / count;

  for (uint32_t i = 0; i < count; i++) {
    Vec3 spread_pos =
        experience_calculate_orb_spread_position(position, i, count);
    experience_orb_spawn(system, spread_pos, amount_per_orb, 0);
  }
}

void experience_orb_update(ExperienceSystem *system, float delta_time) {
  if (!system)
    return;

  for (uint32_t i = 0; i < system->max_orbs; i++) {
    ExperienceOrb *orb = &system->experience_orbs[i];

    if (orb->lifetime > 0.0f && !orb->collected) {
      // Update lifetime
      orb->lifetime -= delta_time;

      // Remove if expired
      if (orb->lifetime <= 0.0f) {
        system->active_orbs--;
        continue;
      }

      // Update position
      orb->position =
          vec3_add(orb->position, vec3_mul(orb->velocity, delta_time));

      // Apply gravity
      orb->velocity.y -= 9.8f * delta_time * 0.1f;

      // Apply friction
      orb->velocity = vec3_mul(orb->velocity, 0.98f);

      // Check for player attraction
      EntityID nearest = 0;
      float nearest_dist = orb->attraction_range;

      for (u32 p = 0; p < g_player_entry_count; p++) {
        TransformComponent *transform = (TransformComponent *)ecs_get_component(
            system->ecs_world, (Entity){g_player_entries[p].entity, 0},
            TRANSFORM_COMPONENT_ID);
        if (!transform)
          continue;

        float dist = vec3_distance(orb->position, transform->position);
        if (dist < nearest_dist) {
          nearest = g_player_entries[p].entity;
          nearest_dist = dist;
        }
      }

      if (nearest != 0) {
        orb->target_entity = nearest;
        experience_orb_attract_to_player(system, i, nearest);
        if (nearest_dist < 1.0f) {
          experience_orb_collect(system, i, nearest);
        }
      }
    }
  }
}

bool experience_orb_collect(ExperienceSystem *system, uint32_t orb_index,
                            EntityID collector) {
  if (!system || orb_index >= system->max_orbs)
    return false;

  ExperienceOrb *orb = &system->experience_orbs[orb_index];
  if (orb->collected || orb->lifetime <= 0.0f)
    return false;

  // Award experience to collector
  experience_add_experience_to_player(system, collector,
                                      orb->experience_amount);

  // Update statistics
  system->stats.experience_orbs_collected++;
  if (orb->experience_amount > system->stats.largest_orb_collected) {
    system->stats.largest_orb_collected = orb->experience_amount;
  }

  // Mark as collected
  orb->collected = true;
  orb->lifetime = 0.0f;
  system->active_orbs--;

  // Play collection sound
  LOG_DEBUG("Experience orb collected");

  LOG_DEBUG("Collected experience orb worth %.1f XP", orb->experience_amount);
  return true;
}

void experience_orb_attract_to_player(ExperienceSystem *system,
                                      uint32_t orb_index, EntityID player) {
  if (!system || orb_index >= system->max_orbs)
    return;

  ExperienceOrb *orb = &system->experience_orbs[orb_index];
  if (orb->collected || orb->lifetime <= 0.0f)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      system->ecs_world, (Entity){player, 0}, TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  Vec3 to_target = vec3_sub(transform->position, orb->position);
  float dist = vec3_length(to_target);
  if (dist <= 0.001f)
    return;

  Vec3 direction = vec3_div(to_target, dist);
  float speed = orb->attraction_speed;
  if (dist < orb->attraction_range * 0.5f) {
    speed *= 1.5f;
  }

  orb->velocity = vec3_add(orb->velocity, vec3_mul(direction, speed));

  float vel_len = vec3_length(orb->velocity);
  float max_speed = orb->attraction_speed * 2.0f;
  if (vel_len > max_speed) {
    orb->velocity = vec3_mul(vec3_div(orb->velocity, vel_len), max_speed);
  }
}

void experience_trigger_level_up_effects(ExperienceSystem *system,
                                         EntityID entity) {
  if (!system)
    return;

  // Play level-up sound
  experience_play_level_up_sound(system, entity);

  // Create particle effects
  experience_create_level_up_particles(system, entity);

  // Show notification
  ExperiencePlayerEntry *entry = experience_get_player_entry(system, entity);
  int level = entry ? entry->level : 1;
  experience_show_level_up_notification(system, entity, level);
}

void experience_play_level_up_sound(ExperienceSystem *system, EntityID entity) {
  if (!system)
    return;

  LOG_DEBUG("Playing level-up sound for entity %d", entity);
}

void experience_create_level_up_particles(ExperienceSystem *system,
                                          EntityID entity) {
  if (!system)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      system->ecs_world, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  LOG_DEBUG("Creating level-up particles at position (%.1f, %.1f, %.1f)",
            transform->position.x, transform->position.y,
            transform->position.z);
}

void experience_show_level_up_notification(ExperienceSystem *system,
                                           EntityID entity, int new_level) {
  if (!system)
    return;

  LOG_INFO("Level up! You are now level %d", new_level);
}

void experience_award_mining(ExperienceSystem *system, EntityID entity,
                             BlockID block_type) {
  if (!system)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_MINING);

  // Modify amount based on block type
  switch (block_type) {
  case BLOCK_STONE:
  case BLOCK_COAL_ORE:
    base_amount *= 1.0f;
    break;
  case BLOCK_IRON_ORE:
  case BLOCK_GOLD_ORE:
    base_amount *= 2.0f;
    break;
  case BLOCK_DIAMOND_ORE:
    base_amount *= 5.0f;
    break;
  case BLOCK_EMERALD_ORE:
    base_amount *= 6.0f;
    break;
  default:
    base_amount *= 0.5f;
    break;
  }

  experience_add_experience(system, entity, base_amount, EXP_SOURCE_MINING);
}

void experience_award_combat(ExperienceSystem *system, EntityID killer,
                             EntityID victim) {
  if (!system)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_COMBAT);

  // Get victim's health to calculate experience
  HealthComponent *victim_health = (HealthComponent *)ecs_get_component(
      system->ecs_world, (Entity){victim, 0}, HEALTH_COMPONENT_ID);
  if (victim_health) {
    // More experience for tougher enemies
    base_amount *= (1.0f + victim_health->max_health * 0.1f);
  }

  experience_add_experience(system, killer, base_amount, EXP_SOURCE_COMBAT);
}

void experience_award_exploration(ExperienceSystem *system, EntityID entity,
                                  Vec3 position) {
  if (!system)
    return;

  // Simple exploration experience based on distance from origin
  float distance = vec3_length(position);
  float base_amount = experience_source_get_base_amount(EXP_SOURCE_EXPLORATION);

  // Award experience for exploring new areas
  if (distance > 100.0f) {
    float exploration_bonus = (distance - 100.0f) * 0.01f;
    experience_add_experience(system, entity, base_amount + exploration_bonus,
                              EXP_SOURCE_EXPLORATION);
  }
}

void experience_award_crafting(ExperienceSystem *system, EntityID entity,
                               uint32_t item_id, uint32_t count) {
  (void)item_id;
  if (!system || count == 0)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_CRAFTING);
  float total = base_amount * (float)count;
  experience_add_experience(system, entity, total, EXP_SOURCE_CRAFTING);
}

void experience_award_smelting(ExperienceSystem *system, EntityID entity,
                               uint32_t item_id) {
  (void)item_id;
  if (!system)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_SMELTING);
  experience_add_experience(system, entity, base_amount, EXP_SOURCE_SMELTING);
}

void experience_award_fishing(ExperienceSystem *system, EntityID entity,
                              uint32_t fish_type) {
  (void)fish_type;
  if (!system)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_FISHING);
  experience_add_experience(system, entity, base_amount, EXP_SOURCE_FISHING);
}

void experience_award_farming(ExperienceSystem *system, EntityID entity,
                              uint32_t crop_type) {
  (void)crop_type;
  if (!system)
    return;

  float base_amount = experience_source_get_base_amount(EXP_SOURCE_FARMING);
  experience_add_experience(system, entity, base_amount, EXP_SOURCE_FARMING);
}

void skill_tree_init(SkillTree *tree, SkillCategory category) {
  if (!tree)
    return;

  memset(tree, 0, sizeof(SkillTree));
  tree->category = category;
  tree->skill_points_available = 0;
  tree->total_skill_points_earned = 0;
  tree->auto_allocate = false;

  // Initialize skill nodes based on category
  switch (category) {
  case SKILL_CATEGORY_COMBAT:
    tree->node_count = 6;
    break;
  case SKILL_CATEGORY_MINING:
    tree->node_count = 4;
    break;
  case SKILL_CATEGORY_CRAFTING:
    tree->node_count = 4;
    break;
  case SKILL_CATEGORY_FARMING:
    tree->node_count = 3;
    break;
  case SKILL_CATEGORY_MAGIC:
    tree->node_count = 4;
    break;
  case SKILL_CATEGORY_ARCHERY:
    tree->node_count = 3;
    break;
  case SKILL_CATEGORY_SURVIVAL:
    tree->node_count = 4;
    break;
  default:
    tree->node_count = 0;
    break;
  }

  if (tree->node_count > 0) {
    tree->nodes = (SkillNode *)core_alloc(tree->node_count * sizeof(SkillNode));
    if (tree->nodes) {
      memset(tree->nodes, 0, tree->node_count * sizeof(SkillNode));

      // Initialize basic skill data
      for (uint32_t i = 0; i < tree->node_count; i++) {
        SkillNode *node = &tree->nodes[i];
        node->skill_id = (SkillType)i;
        node->level = 0;
        node->max_level = 10;
        node->experience = 0.0f;
        node->experience_to_next = 100.0f;
        node->unlocked = false;
        node->prerequisite_count = 0;
        node->cost_multiplier = 1.0f;

        // Set skill names and descriptions
        strncpy(node->name, skill_get_name(node->skill_id),
                sizeof(node->name) - 1);
        strncpy(node->description, skill_get_description(node->skill_id),
                sizeof(node->description) - 1);
      }
    }
  }
}

void skill_tree_cleanup(SkillTree *tree) {
  if (!tree)
    return;

  if (tree->nodes) {
    core_free(tree->nodes);
    tree->nodes = NULL;
  }

  memset(tree, 0, sizeof(SkillTree));
}

bool skill_unlock_node(ExperienceSystem *system, EntityID entity,
                       SkillCategory category, SkillType skill) {
  if (!system || category >= SKILL_CATEGORY_COUNT)
    return false;

  SkillTree *tree = &system->skill_trees[category];
  SkillNode *node = skill_tree_get_node(tree, skill);

  if (!node || node->unlocked)
    return false;

  // Check prerequisites
  if (!skill_tree_has_prerequisites(tree, entity, node)) {
    return false;
  }

  // Check if player has skill points
  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry || entry->skill_points <= 0) {
    return false;
  }
  entry->skill_points -= 1;
  tree->skill_points_available = (uint32_t)entry->skill_points;

  // Unlock the skill
  node->unlocked = true;
  node->level = 1;

  system->stats.skills_unlocked++;

  if (system->enable_skill_notifications) {
    LOG_INFO("Unlocked skill: %s", node->name);
  }

  return true;
}

bool skill_upgrade_node(ExperienceSystem *system, EntityID entity,
                        SkillCategory category, SkillType skill) {
  if (!system || category >= SKILL_CATEGORY_COUNT)
    return false;

  SkillTree *tree = &system->skill_trees[category];
  SkillNode *node = skill_tree_get_node(tree, skill);

  if (!node || !node->unlocked || node->level >= node->max_level)
    return false;

  // Check if player has skill points
  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry || entry->skill_points <= 0) {
    return false;
  }
  entry->skill_points -= 1;
  tree->skill_points_available = (uint32_t)entry->skill_points;

  // Upgrade the skill
  node->level++;
  node->experience = 0.0f;
  node->experience_to_next = 100.0f * node->level * node->cost_multiplier;

  if (system->enable_skill_notifications) {
    LOG_INFO("Upgraded skill: %s to level %d", node->name, node->level);
  }

  return true;
}

SkillNode *skill_tree_get_node(SkillTree *tree, SkillType skill) {
  if (!tree || skill >= SKILL_COUNT)
    return NULL;

  for (uint32_t i = 0; i < tree->node_count; i++) {
    if (tree->nodes[i].skill_id == skill) {
      return &tree->nodes[i];
    }
  }

  return NULL;
}

bool skill_tree_has_prerequisites(const SkillTree *tree, EntityID entity,
                                  const SkillNode *node) {
  (void)entity;
  if (!tree || !node)
    return false;
  if (node->prerequisite_count == 0)
    return true;

  for (uint32_t i = 0; i < node->prerequisite_count; i++) {
    SkillNode *prereq =
        skill_tree_get_node((SkillTree *)tree, node->prerequisites[i]);
    if (!prereq || !prereq->unlocked) {
      return false;
    }
  }
  return true;
}

void skill_tree_reset(SkillTree *tree) {
  if (!tree || !tree->nodes)
    return;
  for (uint32_t i = 0; i < tree->node_count; i++) {
    SkillNode *node = &tree->nodes[i];
    node->unlocked = false;
    node->level = 0;
    node->experience = 0.0f;
    node->experience_to_next = 100.0f * node->cost_multiplier;
  }
  tree->skill_points_available = 0;
  tree->total_skill_points_earned = 0;
}

bool skill_can_unlock(const ExperienceSystem *system, EntityID entity,
                      const SkillNode *node) {
  if (!system || !node || node->unlocked)
    return false;
  ExperiencePlayerEntry *entry =
      experience_get_player_entry((ExperienceSystem *)system, entity);
  if (!entry || entry->skill_points <= 0)
    return false;
  SkillCategory category = skill_get_category(node->skill_id);
  return skill_tree_has_prerequisites(&system->skill_trees[category], entity,
                                      node);
}

int skill_get_level(const ExperienceSystem *system, EntityID entity,
                    SkillType skill) {
  if (!system || skill >= SKILL_COUNT)
    return 0;
  SkillCategory category = skill_get_category(skill);
  const SkillTree *tree = &system->skill_trees[category];
  const SkillNode *node = skill_tree_get_node((SkillTree *)tree, skill);
  if (!node || !node->unlocked)
    return 0;
  (void)entity;
  return node->level;
}

float skill_get_effectiveness(const ExperienceSystem *system, EntityID entity,
                              SkillType skill) {
  if (!system || skill >= SKILL_COUNT)
    return 1.0f;

  // Find skill in appropriate tree
  SkillCategory category = skill_get_category(skill);
  const SkillTree *tree = &system->skill_trees[category];
  const SkillNode *node = skill_tree_get_node((SkillTree *)tree, skill);

  if (!node || !node->unlocked)
    return 1.0f;

  return skill_get_base_effect(skill, node->level);
}

// Utility Functions
const char *experience_source_get_name(ExperienceSource source) {
  if (source >= EXP_SOURCE_COUNT)
    return "Unknown";
  return EXPERIENCE_SOURCE_NAMES[source];
}

float experience_source_get_base_amount(ExperienceSource source) {
  if (source >= EXP_SOURCE_COUNT)
    return 0.0f;
  return EXPERIENCE_BASE_AMOUNTS[source];
}

bool experience_source_should_award(ExperienceSource source, EntityID entity) {
  (void)entity;
  return source < EXP_SOURCE_COUNT;
}

const char *skill_get_name(SkillType skill) {
  if (skill >= SKILL_COUNT)
    return "Unknown";
  return SKILL_NAMES[skill];
}

const char *skill_get_description(SkillType skill) {
  // Return basic descriptions for now
  switch (skill) {
  case SKILL_SWORD_MASTERY:
    return "Increases sword damage by 5% per level";
  case SKILL_MINING_SPEED:
    return "Increases mining speed by 10% per level";
  case SKILL_SPELL_POWER:
    return "Increases spell damage by 8% per level";
  default:
    return "A useful skill that improves your abilities";
  }
}

SkillCategory skill_get_category(SkillType skill) {
  if (skill <= SKILL_CRITICAL_STRIKES)
    return SKILL_CATEGORY_COMBAT;
  if (skill <= SKILL_GEM_FINDING)
    return SKILL_CATEGORY_MINING;
  if (skill <= SKILL_ALCHEMY)
    return SKILL_CATEGORY_CRAFTING;
  if (skill <= SKILL_ANIMAL_HUSBANDRY)
    return SKILL_CATEGORY_FARMING;
  if (skill <= SKILL_ELEMENTAL_MASTERY)
    return SKILL_CATEGORY_MAGIC;
  if (skill <= SKILL_STEALTH)
    return SKILL_CATEGORY_SURVIVAL;
  return SKILL_CATEGORY_SURVIVAL;
}

float skill_get_base_effect(SkillType skill, int level) {
  float base_effect = 1.0f;

  switch (skill) {
  case SKILL_SWORD_MASTERY:
    base_effect = 1.0f + (level * 0.05f);
    break;
  case SKILL_MINING_SPEED:
    base_effect = 1.0f + (level * 0.10f);
    break;
  case SKILL_SPELL_POWER:
    base_effect = 1.0f + (level * 0.08f);
    break;
  default:
    base_effect = 1.0f + (level * 0.03f);
    break;
  }

  return base_effect;
}

bool skill_is_max_level(const SkillNode *node) {
  if (!node)
    return true;
  return node->level >= node->max_level;
}

float experience_apply_stat_scaling(ExperienceSystem *system, EntityID entity,
                                    float base_value, const char *stat_name) {
  if (!system || !stat_name)
    return base_value;
  ExperiencePlayerEntry *entry = experience_get_player_entry(system, entity);
  if (!entry)
    return base_value;

  float level_bonus = 1.0f + (float)entry->level * 0.02f;
  float skill_bonus = 1.0f;

  if (strcmp(stat_name, "health") == 0) {
    skill_bonus = skill_get_effectiveness(system, entity, SKILL_ENDURANCE);
  } else if (strcmp(stat_name, "damage") == 0) {
    skill_bonus = skill_get_effectiveness(system, entity, SKILL_SWORD_MASTERY);
  } else if (strcmp(stat_name, "speed") == 0) {
    skill_bonus = skill_get_effectiveness(system, entity, SKILL_ENDURANCE);
  } else if (strcmp(stat_name, "mining_speed") == 0) {
    skill_bonus = skill_get_effectiveness(system, entity, SKILL_MINING_SPEED);
  }

  return base_value * level_bonus * skill_bonus;
}

float experience_get_health_modifier(ExperienceSystem *system,
                                     EntityID entity) {
  return experience_apply_stat_scaling(system, entity, 1.0f, "health");
}

float experience_get_damage_modifier(ExperienceSystem *system,
                                     EntityID entity) {
  return experience_apply_stat_scaling(system, entity, 1.0f, "damage");
}

float experience_get_speed_modifier(ExperienceSystem *system, EntityID entity) {
  return experience_apply_stat_scaling(system, entity, 1.0f, "speed");
}

float experience_get_mining_speed_modifier(ExperienceSystem *system,
                                           EntityID entity) {
  return experience_apply_stat_scaling(system, entity, 1.0f, "mining_speed");
}

Vec3 experience_calculate_orb_spread_position(Vec3 center, uint32_t index,
                                              uint32_t total) {
  if (total == 1)
    return center;

  float angle = (2.0f * M_PI * index) / total;
  float radius = 0.5f;

  Vec3 spread = {center.x + cosf(angle) * radius, center.y,
                 center.z + sinf(angle) * radius};

  return spread;
}

bool experience_is_valid_amount(float amount) {
  return amount > 0.0f && amount < 10000.0f; // Reasonable limits
}

float experience_get_distance_to_nearest_player(const ExperienceSystem *system,
                                                Vec3 position) {
  if (!system || g_player_entry_count == 0)
    return -1.0f;

  float best = 1e9f;
  for (u32 i = 0; i < g_player_entry_count; i++) {
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        system->ecs_world, (Entity){g_player_entries[i].entity, 0},
        TRANSFORM_COMPONENT_ID);
    if (!transform)
      continue;
    float dist = vec3_distance(position, transform->position);
    if (dist < best) {
      best = dist;
    }
  }

  return (best >= 1e9f) ? -1.0f : best;
}

bool experience_save_data(const ExperienceSystem *system,
                          const char *filepath) {
  if (!system || !filepath)
    return false;
  FILE *file = fopen(filepath, "w");
  if (!file)
    return false;

  fprintf(file, "v1\n");
  fprintf(file, "stats %u %u %u %u %.2f\n", system->stats.levels_gained,
          system->stats.skill_points_earned, system->stats.skills_unlocked,
          system->stats.experience_orbs_collected,
          system->stats.largest_orb_collected);
  fprintf(file, "players %u\n", g_player_entry_count);
  for (u32 i = 0; i < g_player_entry_count; i++) {
    fprintf(file, "player %u %.3f %d %d\n", (u32)g_player_entries[i].entity,
            g_player_entries[i].experience, g_player_entries[i].level,
            g_player_entries[i].skill_points);
  }

  fclose(file);
  return true;
}

bool experience_load_data(ExperienceSystem *system, const char *filepath) {
  if (!system || !filepath)
    return false;
  FILE *file = fopen(filepath, "r");
  if (!file)
    return false;

  char line[256];
  if (!fgets(line, sizeof(line), file)) {
    fclose(file);
    return false;
  }

  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "stats", 5) == 0) {
      sscanf(line, "stats %u %u %u %u %f", &system->stats.levels_gained,
             &system->stats.skill_points_earned, &system->stats.skills_unlocked,
             &system->stats.experience_orbs_collected,
             &system->stats.largest_orb_collected);
    } else if (strncmp(line, "players", 7) == 0) {
      u32 count = 0;
      sscanf(line, "players %u", &count);
      if (g_player_entries) {
        core_free(g_player_entries);
      }
      g_player_entries = NULL;
      g_player_entry_count = 0;
      if (count > 0) {
        g_player_entries = (ExperiencePlayerEntry *)core_alloc(
            count * sizeof(ExperiencePlayerEntry));
        if (!g_player_entries) {
          fclose(file);
          return false;
        }
        memset(g_player_entries, 0, count * sizeof(ExperiencePlayerEntry));
        g_player_entry_count = count;
        for (u32 i = 0; i < count; i++) {
          if (!fgets(line, sizeof(line), file))
            break;
          ExperiencePlayerEntry *entry = &g_player_entries[i];
          sscanf(line, "player %u %f %d %d", (u32 *)&entry->entity,
                 &entry->experience, &entry->level, &entry->skill_points);
        }
      }
    }
  }

  fclose(file);
  return true;
}

bool experience_save_player_data(const ExperienceSystem *system,
                                 EntityID entity, const char *filepath) {
  if (!system || !filepath)
    return false;
  ExperiencePlayerEntry *entry =
      experience_get_player_entry((ExperienceSystem *)system, entity);
  if (!entry)
    return false;

  FILE *file = fopen(filepath, "w");
  if (!file)
    return false;
  fprintf(file, "player %u %.3f %d %d\n", (u32)entry->entity, entry->experience,
          entry->level, entry->skill_points);
  fclose(file);
  return true;
}

bool experience_load_player_data(ExperienceSystem *system, EntityID entity,
                                 const char *filepath) {
  if (!system || !filepath)
    return false;
  FILE *file = fopen(filepath, "r");
  if (!file)
    return false;

  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (!entry) {
    fclose(file);
    return false;
  }

  u32 entity_id = 0;
  if (fscanf(file, "player %u %f %d %d", &entity_id, &entry->experience,
             &entry->level, &entry->skill_points) != 4) {
    fclose(file);
    return false;
  }
  entry->entity = entity_id;
  fclose(file);
  return true;
}

void experience_debug_print_skill_tree(const ExperienceSystem *system,
                                       SkillCategory category) {
  if (!system || category >= SKILL_CATEGORY_COUNT)
    return;

  const SkillTree *tree = &system->skill_trees[category];
  LOG_INFO("=== Skill Tree: %s ===", SKILL_CATEGORY_NAMES[category]);
  for (u32 i = 0; i < tree->node_count; i++) {
    const SkillNode *node = &tree->nodes[i];
    LOG_INFO("%s (lvl %d/%d) %s", node->name, node->level, node->max_level,
             node->unlocked ? "unlocked" : "locked");
  }
}

void experience_debug_add_experience(ExperienceSystem *system, EntityID entity,
                                     float amount) {
  if (!system)
    return;
  experience_add_experience(system, entity, amount, EXP_SOURCE_QUEST);
}

void experience_debug_unlock_all_skills(ExperienceSystem *system,
                                        EntityID entity) {
  if (!system)
    return;

  ExperiencePlayerEntry *entry = get_or_create_player_entry(entity);
  if (entry) {
    entry->skill_points = 0;
  }

  for (u32 c = 0; c < SKILL_CATEGORY_COUNT; c++) {
    SkillTree *tree = &system->skill_trees[c];
    for (u32 i = 0; i < tree->node_count; i++) {
      SkillNode *node = &tree->nodes[i];
      node->unlocked = true;
      node->level = node->max_level;
    }
  }
}

// Debug Functions
void experience_debug_print_stats(const ExperienceSystem *system) {
  if (!system)
    return;

  LOG_INFO("=== Experience System Stats ===");
  LOG_INFO("Active Orbs: %u/%u", system->active_orbs, system->max_orbs);
  LOG_INFO("Levels Gained: %u", system->stats.levels_gained);
  LOG_INFO("Skills Unlocked: %u", system->stats.skills_unlocked);

  for (int i = 0; i < EXP_SOURCE_COUNT; i++) {
    if (system->stats.experience_gained[i] > 0.0f) {
      LOG_INFO("XP from %s: %.1f",
               experience_source_get_name((ExperienceSource)i),
               system->stats.experience_gained[i]);
    }
  }
}

// Global accessor functions
ExperienceSystem *get_experience_system(void) { return &g_experience_system; }

bool init_experience_system(World *ecs_world) {
  return experience_system_init(&g_experience_system, ecs_world);
}

void cleanup_experience_system(void) {
  experience_system_cleanup(&g_experience_system);
}
