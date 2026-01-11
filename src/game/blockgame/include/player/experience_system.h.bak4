#ifndef EXPERIENCE_SYSTEM_H
#define EXPERIENCE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec3.h"
#include "ecs/ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Experience Sources
typedef enum {
    EXP_SOURCE_MINING = 0,
    EXP_SOURCE_CRAFTING,
    EXP_SOURCE_COMBAT,
    EXP_SOURCE_EXPLORATION,
    EXP_SOURCE_SMELTING,
    EXP_SOURCE_FISHING,
    EXP_SOURCE_FARMING,
    EXP_SOURCE_ENCHANTING,
    EXP_SOURCE_BREWING,
    EXP_SOURCE_BUILDING,
    EXP_SOURCE_TRADE,
    EXP_SOURCE_QUEST,
    EXP_SOURCE_COUNT
} ExperienceSource;

// Skill Categories
typedef enum {
    SKILL_CATEGORY_COMBAT = 0,
    SKILL_CATEGORY_MINING,
    SKILL_CATEGORY_CRAFTING,
    SKILL_CATEGORY_FARMING,
    SKILL_CATEGORY_MAGIC,
    SKILL_CATEGORY_ARCHERY,
    SKILL_CATEGORY_SURVIVAL,
    SKILL_CATEGORY_COUNT
} SkillCategory;

// Individual Skills
typedef enum {
    // Combat Skills
    SKILL_SWORD_MASTERY = 0,
    SKILL_AXE_MASTERY,
    SKILL_ARCHERY,
    SKILL_UNARMED_COMBAT,
    SKILL_SHIELD_MASTERY,
    SKILL_CRITICAL_STRIKES,
    
    // Mining Skills
    SKILL_MINING_SPEED,
    SKILL_MINING_FORTUNE,
    SKILL_EXCAVATION,
    SKILL_GEM_FINDING,
    
    // Crafting Skills
    SKILL_CRAFTING_EFFICIENCY,
    SKILL_SMITHING,
    SKILL_ENCHANTING_PROFICIENCY,
    SKILL_ALCHEMY,
    
    // Farming Skills
    SKILL_HARVESTING,
    SKILL_CROP_YIELD,
    SKILL_ANIMAL_HUSBANDRY,
    
    // Magic Skills
    SKILL_SPELL_POWER,
    SKILL_MANA_EFFICIENCY,
    SKILL_SUMMONING,
    SKILL_ELEMENTAL_MASTERY,
    
    // Survival Skills
    SKILL_ENDURANCE,
    SKILL_FORAGING,
    SKILL_STEALTH,
    SKILL_FIRST_AID,
    
    SKILL_COUNT
} SkillType;

// Skill Node Structure (for skill trees)
typedef struct {
    SkillType skill_id;
    int level;
    int max_level;
    float experience;
    float experience_to_next;
    bool unlocked;
    SkillType prerequisites[4];
    uint32_t prerequisite_count;
    float cost_multiplier;
    char name[64];
    char description[256];
} SkillNode;

// Skill Tree
typedef struct {
    SkillCategory category;
    SkillNode* nodes;
    uint32_t node_count;
    uint32_t skill_points_available;
    uint32_t total_skill_points_earned;
    bool auto_allocate;
} SkillTree;

// Experience Orb
typedef struct {
    Vec3 position;
    Vec3 velocity;
    float experience_amount;
    float lifetime;
    bool collected;
    EntityID target_entity;
    float attraction_range;
    float attraction_speed;
    uint32_t color;
} ExperienceOrb;

// Player Level Data
typedef struct {
    int level;
    float experience;
    float experience_to_next;
    float total_experience;
    uint32_t skill_points;
    bool level_up_pending;
    float level_up_animation_time;
    Vec3 level_up_effect_position;
} PlayerLevel;

// Experience Statistics
typedef struct {
    float experience_gained[EXP_SOURCE_COUNT];
    uint32_t levels_gained;
    uint32_t skill_points_earned;
    uint32_t skills_unlocked;
    uint32_t experience_orbs_collected;
    float largest_orb_collected;
    uint64_t session_start_time;
    uint64_t total_play_time;
} ExperienceStats;

// Experience System Context
typedef struct {
    ECSWorld* ecs_world;
    ExperienceOrb* experience_orbs;
    uint32_t max_orbs;
    uint32_t active_orbs;
    SkillTree skill_trees[SKILL_CATEGORY_COUNT];
    ExperienceStats stats;
    bool enable_orb_effects;
    bool enable_level_up_effects;
    bool enable_skill_notifications;
    float orb_attraction_range;
    float orb_collection_speed;
} ExperienceSystem;

// Experience System Functions
bool experience_system_init(ExperienceSystem* system, ECSWorld* ecs_world);
void experience_system_cleanup(ExperienceSystem* system);
void experience_system_update(ExperienceSystem* system, float delta_time);

// Level Management
bool experience_add_experience(ExperienceSystem* system, EntityID entity, float amount, ExperienceSource source);
/* Adds experience directly to a player and handles level-ups. */
bool experience_add_experience_to_player(ExperienceSystem* system, EntityID entity, float amount);
bool experience_set_level(ExperienceSystem* system, EntityID entity, int level);
bool experience_level_up(ExperienceSystem* system, EntityID entity);

/* Simple per-player entry (exposed for tests) */
typedef struct {
    EntityID entity;
    float experience;
    int level;
    int skill_points;
} ExperiencePlayerEntry;

ExperiencePlayerEntry* experience_get_player_entry(ExperienceSystem* system, EntityID entity);
float experience_get_experience_for_level(int level);
int experience_get_level_for_experience(float experience);
float experience_get_progress_to_next_level(const PlayerLevel* level_data);

// Skill System Functions
bool skill_unlock_node(ExperienceSystem* system, EntityID entity, SkillCategory category, SkillType skill);
bool skill_upgrade_node(ExperienceSystem* system, EntityID entity, SkillCategory category, SkillType skill);
bool skill_can_unlock(const ExperienceSystem* system, EntityID entity, const SkillNode* node);
float skill_get_effectiveness(const ExperienceSystem* system, EntityID entity, SkillType skill);
int skill_get_level(const ExperienceSystem* system, EntityID entity, SkillType skill);

// Experience Orb Functions
void experience_orb_spawn(ExperienceSystem* system, Vec3 position, float amount, EntityID source);
void experience_orb_spawn_multiple(ExperienceSystem* system, Vec3 position, float total_amount, uint32_t count);
void experience_orb_update(ExperienceSystem* system, float delta_time);
bool experience_orb_collect(ExperienceSystem* system, uint32_t orb_index, EntityID collector);
void experience_orb_attract_to_player(ExperienceSystem* system, uint32_t orb_index, EntityID player);

// Skill Tree Functions
void skill_tree_init(SkillTree* tree, SkillCategory category);
void skill_tree_cleanup(SkillTree* tree);
SkillNode* skill_tree_get_node(SkillTree* tree, SkillType skill);
bool skill_tree_has_prerequisites(const SkillTree* tree, EntityID entity, const SkillNode* node);
void skill_tree_reset(SkillTree* tree);

// Experience Award Functions
void experience_award_mining(ExperienceSystem* system, EntityID entity, BlockID block_type);
void experience_award_crafting(ExperienceSystem* system, EntityID entity, uint32_t item_id, uint32_t count);
void experience_award_combat(ExperienceSystem* system, EntityID killer, EntityID victim);
void experience_award_exploration(ExperienceSystem* system, EntityID entity, Vec3 position);
void experience_award_smelting(ExperienceSystem* system, EntityID entity, uint32_t item_id);
void experience_award_fishing(ExperienceSystem* system, EntityID entity, uint32_t fish_type);
void experience_award_farming(ExperienceSystem* system, EntityID entity, uint32_t crop_type);

// Level-up Effects
void experience_trigger_level_up_effects(ExperienceSystem* system, EntityID entity);
void experience_play_level_up_sound(ExperienceSystem* system, EntityID entity);
void experience_create_level_up_particles(ExperienceSystem* system, EntityID entity);
void experience_show_level_up_notification(ExperienceSystem* system, EntityID entity, int new_level);

// Stat Scaling Functions
float experience_apply_stat_scaling(ExperienceSystem* system, EntityID entity, float base_value, const char* stat_name);
float experience_get_health_modifier(ExperienceSystem* system, EntityID entity);
float experience_get_damage_modifier(ExperienceSystem* system, EntityID entity);
float experience_get_speed_modifier(ExperienceSystem* system, EntityID entity);
float experience_get_mining_speed_modifier(ExperienceSystem* system, EntityID entity);

// Experience Source Utilities
const char* experience_source_get_name(ExperienceSource source);
float experience_source_get_base_amount(ExperienceSource source);
bool experience_source_should_award(ExperienceSource source, EntityID entity);

// Skill Utilities
const char* skill_get_name(SkillType skill);
const char* skill_get_description(SkillType skill);
SkillCategory skill_get_category(SkillType skill);
float skill_get_base_effect(SkillType skill, int level);
bool skill_is_max_level(const SkillNode* node);

// Save/Load Functions
bool experience_save_data(const ExperienceSystem* system, const char* filepath);
bool experience_load_data(ExperienceSystem* system, const char* filepath);
bool experience_save_player_data(const ExperienceSystem* system, EntityID entity, const char* filepath);
bool experience_load_player_data(ExperienceSystem* system, EntityID entity, const char* filepath);

// Debug and Testing Functions
void experience_debug_print_stats(const ExperienceSystem* system);
void experience_debug_print_skill_tree(const ExperienceSystem* system, SkillCategory category);
void experience_debug_add_experience(ExperienceSystem* system, EntityID entity, float amount);
void experience_debug_unlock_all_skills(ExperienceSystem* system, EntityID entity);

// Helper Functions
bool experience_is_valid_amount(float amount);
float experience_calculate_orb_value(float total_experience, uint32_t orb_count);
Vec3 experience_calculate_orb_spread_position(Vec3 center, uint32_t index, uint32_t total);
float experience_get_distance_to_nearest_player(const ExperienceSystem* system, Vec3 position);

#ifdef __cplusplus
}
#endif

#endif // EXPERIENCE_SYSTEM_H
