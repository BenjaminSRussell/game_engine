#ifndef DAMAGE_SYSTEM_H
#define DAMAGE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec3.h"
#include "ecs/ecs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Damage Types
typedef enum {
    DAMAGE_TYPE_PHYSICAL = 0,
    DAMAGE_TYPE_FIRE,
    DAMAGE_TYPE_MAGIC,
    DAMAGE_TYPE_POISON,
    DAMAGE_TYPE_FALL,
    DAMAGE_TYPE_DROWNING,
    DAMAGE_TYPE_EXPLOSION,
    DAMAGE_TYPE_LIGHTNING,
    DAMAGE_TYPE_COLD,
    DAMAGE_TYPE_THORNS,
    DAMAGE_TYPE_STARVATION,
    DAMAGE_TYPE_SUFFOCATION,
    DAMAGE_TYPE_LAVA,
    DAMAGE_TYPE_COUNT
} DamageType;

// Damage Source Types
typedef enum {
    DAMAGE_SOURCE_ENTITY = 0,
    DAMAGE_SOURCE_BLOCK,
    DAMAGE_SOURCE_ENVIRONMENT,
    DAMAGE_SOURCE_SELF,
    DAMAGE_SOURCE_UNKNOWN
} DamageSourceType;

// Armor Types and Protection Values
typedef enum {
    ARMOR_TYPE_NONE = 0,
    ARMOR_TYPE_LEATHER,
    ARMOR_TYPE_CHAIN,
    ARMOR_TYPE_IRON,
    ARMOR_TYPE_GOLD,
    ARMOR_TYPE_DIAMOND,
    ARMOR_TYPE_NETHERITE,
    ARMOR_TYPE_COUNT
} ArmorType;

typedef enum {
    ARMOR_SLOT_HELMET = 0,
    ARMOR_SLOT_CHESTPLATE,
    ARMOR_SLOT_LEGGINGS,
    ARMOR_SLOT_BOOTS,
    ARMOR_SLOT_COUNT
} ArmorSlot;

// Damage Instance
typedef struct {
    DamageType type;
    DamageSourceType source_type;
    EntityID source_entity;
    float amount;
    Vec3 position;
    Vec3 direction;
    bool is_critical;
    float armor_penetration;
    bool bypass_armor;
    bool bypass_invulnerability;
} DamageInstance;

// Armor Piece
typedef struct {
    ArmorType type;
    ArmorSlot slot;
    float durability;
    float max_durability;
    float protection_values[DAMAGE_TYPE_COUNT];
    uint32_t enchantments;
} ArmorPiece;

// Armor Set
typedef struct {
    ArmorPiece pieces[ARMOR_SLOT_COUNT];
    float total_protection[DAMAGE_TYPE_COUNT];
    float toughness;
    float enchantment_protection;
} ArmorSet;

// Damage Over Time (DOT) Effect
typedef struct {
    DamageType type;
    float damage_per_second;
    float duration;
    float time_remaining;
    float interval;
    float time_since_last_tick;
    EntityID source;
    bool stacks;
    int stack_count;
} DOTEffect;

// Damage Statistics
typedef struct {
    float total_damage_dealt;
    float total_damage_taken;
    float damage_by_type[DAMAGE_TYPE_COUNT];
    uint32_t hits_dealt;
    uint32_t hits_taken;
    uint32_t critical_hits;
    uint32_t deaths;
    float largest_hit_dealt;
    float largest_hit_taken;
    uint64_t combat_start_time;
    uint64_t combat_end_time;
} DamageStats;

// Damage Visualization
typedef struct {
    Vec3 position;
    float damage_amount;
    float display_time;
    float fade_time;
    bool is_critical;
    bool is_healing;
    DamageType type;
    Vec3 velocity;
    float scale;
} DamageNumber;

// Damage Immunity Frame
typedef struct {
    bool immune;
    float duration;
    float time_remaining;
    DamageType immune_types[DAMAGE_TYPE_COUNT];
    uint32_t immune_type_count;
} DamageImmunity;

// Damage System Context
typedef struct {
    ECSWorld* ecs_world;
    DamageStats* player_stats;
    DamageNumber* damage_numbers;
    uint32_t max_damage_numbers;
    uint32_t active_damage_numbers;
    bool enable_visualization;
    bool enable_logging;
    bool enable_sound_effects;
    bool enable_particle_effects;
} DamageSystem;

// Damage System Functions
bool damage_system_init(DamageSystem* system, ECSWorld* ecs_world);
void damage_system_cleanup(DamageSystem* system);
void damage_system_update(DamageSystem* system, float delta_time);

// Core Damage Application
float damage_apply_to_entity(DamageSystem* system, EntityID target, const DamageInstance* damage);
float damage_apply_with_armor(DamageSystem* system, EntityID target, const DamageInstance* damage, const ArmorSet* armor);
bool damage_can_damage_entity(DamageSystem* system, EntityID target, const DamageInstance* damage);

// Damage Instance Creation
DamageInstance damage_create(DamageType type, float amount, EntityID source);
DamageInstance damage_create_positional(DamageType type, float amount, EntityID source, Vec3 position, Vec3 direction);
DamageInstance damage_create_critical(DamageType type, float amount, EntityID source);
DamageInstance damage_create_armor_penetrating(DamageType type, float amount, EntityID source, float penetration);

// Armor System
void armor_set_init(ArmorSet* armor);
void armor_set_add_piece(ArmorSet* armor, const ArmorPiece* piece);
void armor_set_remove_piece(ArmorSet* armor, ArmorSlot slot);
float armor_calculate_protection(const ArmorSet* armor, DamageType type);
float armor_calculate_toughness_reduction(const ArmorSet* armor, float damage);
void armor_apply_durability_damage(ArmorSet* armor, DamageType type, float damage);
bool armor_is_piece_broken(const ArmorPiece* piece);

// Damage Over Time System
void damage_dot_add_effect(DamageSystem* system, EntityID entity, const DOTEffect* dot);
void damage_dot_remove_effect(DamageSystem* system, EntityID entity, DamageType type);
void damage_dot_update_effects(DamageSystem* system, float delta_time);
bool damage_dot_has_effect(DamageSystem* system, EntityID entity, DamageType type);

// Damage Immunity System
void damage_immunity_set(DamageSystem* system, EntityID entity, float duration, const DamageType* types, uint32_t type_count);
void damage_immunity_clear(DamageSystem* system, EntityID entity);
bool damage_immunity_is_immune(DamageSystem* system, EntityID entity, DamageType type);
void damage_immunity_update(DamageSystem* system, float delta_time);

// Damage Statistics
void damage_stats_reset(DamageStats* stats);
void damage_stats_record_damage_dealt(DamageStats* stats, DamageType type, float amount);
void damage_stats_record_damage_taken(DamageStats* stats, DamageType type, float amount);
void damage_stats_record_death(DamageStats* stats);
void damage_stats_record_combat_start(DamageStats* stats);
void damage_stats_record_combat_end(DamageStats* stats);

// Damage Visualization
void damage_visualization_show_number(DamageSystem* system, Vec3 position, float amount, DamageType type, bool is_critical);
void damage_visualization_show_healing(DamageSystem* system, Vec3 position, float amount);
void damage_visualization_update(DamageSystem* system, float delta_time);
void damage_visualization_clear_all(DamageSystem* system);

// Damage Type Utilities
const char* damage_type_get_name(DamageType type);
bool damage_type_is_physical(DamageType type);
bool damage_type_is_elemental(DamageType type);
bool damage_type_is_environmental(DamageType type);
float damage_type_get_default_resistance(DamageType type);

// Armor Utilities
const char* armor_type_get_name(ArmorType type);
float armor_type_get_base_protection(ArmorType type);
float armor_type_get_durability(ArmorType type);
float armor_type_get_toughness(ArmorType type);

// Damage Calculation Utilities
float damage_calculate_reduction(float damage, float armor_value, float toughness);
float damage_calculate_enchantment_reduction(float damage, const ArmorSet* armor, DamageType type);
float damage_calculate_critical_hit_damage(float base_damage);
float damage_calculate_fall_damage(float fall_distance, bool has_feather_falling);

// Status Effect Damage Integration
float damage_apply_status_effect_modifiers(float base_damage, EntityID entity);
bool damage_should_apply_status_effect_resistance(EntityID entity, DamageType type);

// Helper Functions
bool damage_is_zero(const DamageInstance* damage);
DamageInstance damage_scale(const DamageInstance* damage, float scale_factor);
DamageInstance damage_combine(const DamageInstance* damage1, const DamageInstance* damage2);

#ifdef __cplusplus
}
#endif

#endif // DAMAGE_SYSTEM_H
