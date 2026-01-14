// include/player/spell_upgrade.h
//
// Purpose: Defines the spell upgrade system for managing spell progression,
// skill trees, and advancement paths. This header provides the framework for
// leveling up spells, unlocking new abilities, and managing upgrade prerequisites.
//
// Public APIs:
// - `SpellUpgradeType`: Enumeration defining upgrade types (e.g., `UPGRADE_DAMAGE`,
//   `UPGRADE_COOLDOWN`, `UPGRADE_DURATION`, `UPGRADE_RANGE`).
// - `SpellUpgradeNode`: Structure representing an upgrade node in the skill tree.
// - `SpellUpgradeTree`: Complete skill tree structure for a spell type.
// - `SpellUpgradeManager`: Main system for managing all spell upgrades and trees.
// - `spell_upgrade_manager_init()`: Initializes the spell upgrade system.
// - `spell_upgrade_apply()`: Applies an upgrade to a specific spell.
// - `spell_upgrade_can_unlock()`: Checks if an upgrade can be unlocked.
// - `spell_upgrade_get_available()`: Gets list of available upgrades for a spell.
// - `spell_upgrade_reset()`: Resets all upgrades for a spell.
//
// Ownership: The `SpellUpgradeManager` manages the lifecycle of all upgrade nodes
// and skill trees. It interacts with the player magic system for applying upgrades.
//
// Invariants:
// - Spell upgrades must be initialized with `spell_upgrade_manager_init()` before use.
// - Upgrade prerequisites must be satisfied before unlocking new upgrades.
// - Upgrade points are consumed when unlocking new abilities.
// - All upgrades provide permanent improvements to spell capabilities.

#ifndef SPELL_UPGRADE_H
#define SPELL_UPGRADE_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>

// Forward declarations
typedef struct PlayerSystem PlayerSystem;
typedef struct PlayerComponent PlayerComponent;
typedef struct PlayerMagicComponent PlayerMagicComponent;

// Spell upgrade types
typedef enum {
    UPGRADE_DAMAGE = 0,         // Increase spell damage/healing magnitude
    UPGRADE_COOLDOWN,           // Reduce spell cooldown
    UPGRADE_DURATION,           // Increase effect duration
    UPGRADE_RANGE,              // Increase spell range
    UPGRADE_MANA_EFFICIENCY,    // Reduce mana cost
    UPGRADE_CAST_TIME,         // Reduce cast time
    UPGRADE_AREA_EFFECT,        // Add or increase area of effect
    UPGRADE_PIERCING,          // Add piercing to projectiles
    UPGRADE_MULTICAST,         // Allow casting multiple instances
    UPGRADE_CHAINING,          // Add chaining to spells
    UPGRADE_CRITICAL_CHANCE,    // Increase critical hit chance
    UPGRADE_CRITICAL_DAMAGE,    // Increase critical damage multiplier
    UPGRADE_LIFE_STEAL,       // Add life steal effect
    UPGRADE_MANA_DRAIN,        // Add mana drain effect
    UPGRADE_STUN,              // Add stun effect
    UPGRADE_SLOW,              // Add slow effect
    UPGRADE_KNOCKBACK,         // Add knockback effect
    UPGRADE_COUNT
} SpellUpgradeType;

// Upgrade tier levels
typedef enum {
    UPGRADE_TIER_BASIC = 0,     // Basic improvements
    UPGRADE_TIER_ADVANCED,      // Significant improvements
    UPGRADE_TIER_MASTER,        // Major improvements
    UPGRADE_TIER_LEGENDARY,     // Ultimate improvements
    UPGRADE_TIER_COUNT
} SpellUpgradeTier;

// Upgrade node structure
typedef struct SpellUpgradeNode {
    SpellUpgradeType type;
    SpellUpgradeTier tier;
    u32 node_id;
    u32 prerequisite_nodes[4];   // Node IDs that must be unlocked first
    u32 prerequisite_count;
    u32 cost_points;             // Upgrade points required
    f32 magnitude;              // Upgrade magnitude (percentage or flat value)
    const char* name;
    const char* description;
    bool is_unlocked;
    bool is_available;           // Can be unlocked (prerequisites met)
    u32 max_rank;               // Maximum times this can be upgraded
    u32 current_rank;           // Current upgrade rank
} SpellUpgradeNode;

// Spell upgrade tree
typedef struct SpellUpgradeTree {
    SpellType spell_type;
    SpellUpgradeNode* nodes;
    u32 node_count;
    u32 max_points_spent;
    u32 points_spent;
    bool is_mastered;            // All nodes unlocked
} SpellUpgradeTree;

// Upgrade manager
typedef struct {
    SpellUpgradeTree* trees;
    u32 tree_count;
    u32 total_upgrade_points;
    u32 points_spent;
    bool auto_unlock_available;   // Auto-unlock available upgrades
    f32 discount_multiplier;     // Point cost discount
} SpellUpgradeManager;

// Upgrade application result
typedef struct {
    bool success;
    u32 points_spent;
    const char* error_message;
    SpellUpgradeNode* applied_node;
} SpellUpgradeResult;

// ============================================================================
// SYSTEM LIFECYCLE
// ============================================================================

// Initialize spell upgrade manager
bool spell_upgrade_manager_init(SpellUpgradeManager* manager);

// Shutdown spell upgrade manager and cleanup
void spell_upgrade_manager_shutdown(SpellUpgradeManager* manager);

// ============================================================================
// UPGRADE MANAGEMENT
// ============================================================================

// Apply an upgrade to a spell
SpellUpgradeResult spell_upgrade_apply(SpellUpgradeManager* manager,
                                   PlayerMagicComponent* magic,
                                   SpellType spell,
                                   u32 node_id);

// Check if an upgrade can be unlocked
bool spell_upgrade_can_unlock(SpellUpgradeManager* manager,
                           PlayerMagicComponent* magic,
                           SpellType spell,
                           u32 node_id);

// Get list of available upgrades for a spell
u32 spell_upgrade_get_available(SpellUpgradeManager* manager,
                              PlayerMagicComponent* magic,
                              SpellType spell,
                              SpellUpgradeNode* out_nodes,
                              u32 max_nodes);

// Reset all upgrades for a spell
bool spell_upgrade_reset(SpellUpgradeManager* manager,
                       PlayerMagicComponent* magic,
                       SpellType spell);

// Reset all spell upgrades
void spell_upgrade_reset_all(SpellUpgradeManager* manager,
                           PlayerMagicComponent* magic);

// ============================================================================
// UPGRADE QUERIES
// ============================================================================

// Get upgrade tree for a spell
const SpellUpgradeTree* spell_upgrade_get_tree(SpellUpgradeManager* manager,
                                           SpellType spell);

// Get upgrade node by ID
const SpellUpgradeNode* spell_upgrade_get_node(SpellUpgradeManager* manager,
                                           SpellType spell,
                                           u32 node_id);

// Check if spell has specific upgrade type
bool spell_upgrade_has_type(SpellUpgradeManager* manager,
                          PlayerMagicComponent* magic,
                          SpellType spell,
                          SpellUpgradeType type);

// Get total upgrade magnitude for a type
f32 spell_upgrade_get_magnitude(SpellUpgradeManager* manager,
                              PlayerMagicComponent* magic,
                              SpellType spell,
                              SpellUpgradeType type);

// Get total points spent on a spell
u32 spell_upgrade_get_points_spent(SpellUpgradeManager* manager,
                                  SpellType spell);

// Check if spell tree is mastered
bool spell_upgrade_is_mastered(SpellUpgradeManager* manager,
                              SpellType spell);

// ============================================================================
// UPGRADE CALCULATIONS
// ============================================================================

// Calculate spell damage with upgrades applied
f32 spell_upgrade_calculate_damage(SpellUpgradeManager* manager,
                                 PlayerMagicComponent* magic,
                                 SpellType spell,
                                 f32 base_damage);

// Calculate spell cooldown with upgrades applied
f32 spell_upgrade_calculate_cooldown(SpellUpgradeManager* manager,
                                   PlayerMagicComponent* magic,
                                   SpellType spell,
                                   f32 base_cooldown);

// Calculate spell mana cost with upgrades applied
f32 spell_upgrade_calculate_mana_cost(SpellUpgradeManager* manager,
                                    PlayerMagicComponent* magic,
                                    SpellType spell,
                                    f32 base_mana_cost);

// Calculate spell range with upgrades applied
f32 spell_upgrade_calculate_range(SpellUpgradeManager* manager,
                               PlayerMagicComponent* magic,
                               SpellType spell,
                               f32 base_range);

// Calculate spell duration with upgrades applied
f32 spell_upgrade_calculate_duration(SpellUpgradeManager* manager,
                                  PlayerMagicComponent* magic,
                                  SpellType spell,
                                  f32 base_duration);

// ============================================================================
// POINT MANAGEMENT
// ============================================================================

// Add upgrade points to player
void spell_upgrade_add_points(SpellUpgradeManager* manager,
                            PlayerMagicComponent* magic,
                            u32 points);

// Get available upgrade points
u32 spell_upgrade_get_available_points(SpellUpgradeManager* manager,
                                    PlayerMagicComponent* magic);

// Refund points from reset upgrades
u32 spell_upgrade_refund_points(SpellUpgradeManager* manager,
                              PlayerMagicComponent* magic,
                              SpellType spell);

// ============================================================================
// CONFIGURATION
// ============================================================================

// Enable/disable auto-unlock for available upgrades
void spell_upgrade_set_auto_unlock(SpellUpgradeManager* manager, bool enabled);

// Set point cost discount multiplier
void spell_upgrade_set_discount(SpellUpgradeManager* manager, f32 multiplier);

// Get upgrade type name
const char* spell_upgrade_get_type_name(SpellUpgradeType type);

// Get upgrade tier name
const char* spell_upgrade_get_tier_name(SpellUpgradeTier tier);

// ============================================================================
// STATISTICS
// ============================================================================

// Get upgrade system statistics
void spell_upgrade_get_stats(SpellUpgradeManager* manager,
                           u32* total_points,
                           u32* points_spent,
                           u32* trees_mastered);

// Get spell upgrade progress
void spell_upgrade_get_progress(SpellUpgradeManager* manager,
                             SpellType spell,
                             u32* nodes_unlocked,
                             u32* total_nodes,
                             f32* completion_percentage);

#endif // SPELL_UPGRADE_H
