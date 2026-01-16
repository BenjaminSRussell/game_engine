// Spell Upgrade System Implementation
#include <player/spell_upgrade.h>
#include <player/player_magic.h>
#include <core/logger.h>
#include <string.h>
#include <stdlib.h>

// Upgrade node definitions for each spell
static SpellUpgradeNode fireball_upgrades[] = {
    {UPGRADE_DAMAGE, UPGRADE_TIER_BASIC, 1, {0}, 0, 2, 1.2f, "Enhanced Damage", "+20% damage", false, true, 3, 0},
    {UPGRADE_COOLDOWN, UPGRADE_TIER_BASIC, 2, {1}, 1, 3, 0.85f, "Rapid Cast", "-15% cooldown", false, true, 2, 0},
    {UPGRADE_AREA_EFFECT, UPGRADE_TIER_ADVANCED, 3, {2}, 1, 5, 1.0f, "Explosion", "Area damage", false, true, 1, 0}
};

static SpellUpgradeTree spell_trees[SPELL_COUNT] = {
    [SPELL_FIREBALL] = {
        .spell_type = SPELL_FIREBALL,
        .nodes = fireball_upgrades,
        .node_count = 3,
        .max_points_spent = 10,
        .points_spent = 0,
        .is_mastered = false
    }
    // Add other spells...
};

SpellUpgradeManager g_upgrade_manager = {0};

bool spell_upgrade_manager_init(SpellUpgradeManager* manager) {
    if (!manager) return false;
    
    memset(manager, 0, sizeof(SpellUpgradeManager));
    manager->trees = spell_trees;
    manager->tree_count = SPELL_COUNT;
    manager->auto_unlock_available = false;
    manager->discount_multiplier = 1.0f;
    
    LOG_INFO("Spell upgrade manager initialized");
    return true;
}

SpellUpgradeResult spell_upgrade_apply(SpellUpgradeManager* manager,
                                   PlayerMagicComponent* magic,
                                   SpellType spell,
                                   u32 node_id) {
    SpellUpgradeResult result = {0};
    
    if (!manager || !magic || spell >= SPELL_COUNT) {
        result.error_message = "Invalid parameters";
        return result;
    }
    
    SpellUpgradeTree* tree = &manager->trees[spell];
    if (node_id >= tree->node_count) {
        result.error_message = "Invalid node ID";
        return result;
    }
    
    SpellUpgradeNode* node = &tree->nodes[node_id];
    if (node->is_unlocked) {
        result.error_message = "Already unlocked";
        return result;
    }
    
    u32 cost = (u32)(node->cost_points * manager->discount_multiplier);
    if (magic->spell_points < cost) {
        result.error_message = "Insufficient points";
        return result;
    }
    
    // Check prerequisites
    for (u32 i = 0; i < node->prerequisite_count; i++) {
        u32 prereq_id = node->prerequisite_nodes[i];
        if (!tree->nodes[prereq_id].is_unlocked) {
            result.error_message = "Prerequisites not met";
            return result;
        }
    }
    
    // Apply upgrade
    magic->spell_points -= cost;
    node->is_unlocked = true;
    node->current_rank++;
    tree->points_spent += cost;
    manager->points_spent += cost;
    
    result.success = true;
    result.points_spent = cost;
    result.applied_node = node;
    
    LOG_INFO("Applied upgrade %s to spell %d", node->name, spell);
    return result;
}

f32 spell_upgrade_calculate_damage(SpellUpgradeManager* manager,
                                 PlayerMagicComponent* magic,
                                 SpellType spell,
                                 f32 base_damage) {
    if (!manager || spell >= SPELL_COUNT) return base_damage;
    
    SpellUpgradeTree* tree = &manager->trees[spell];
    f32 damage_multiplier = 1.0f;
    
    for (u32 i = 0; i < tree->node_count; i++) {
        SpellUpgradeNode* node = &tree->nodes[i];
        if (node->is_unlocked && node->type == UPGRADE_DAMAGE) {
            damage_multiplier *= (1.0f + (node->magnitude * node->current_rank));
        }
    }
    
    return base_damage * damage_multiplier;
}

f32 spell_upgrade_calculate_cooldown(SpellUpgradeManager* manager,
                                   PlayerMagicComponent* magic,
                                   SpellType spell,
                                   f32 base_cooldown) {
    if (!manager || spell >= SPELL_COUNT) return base_cooldown;
    
    SpellUpgradeTree* tree = &manager->trees[spell];
    f32 cooldown_multiplier = 1.0f;
    
    for (u32 i = 0; i < tree->node_count; i++) {
        SpellUpgradeNode* node = &tree->nodes[i];
        if (node->is_unlocked && node->type == UPGRADE_COOLDOWN) {
            cooldown_multiplier *= node->magnitude;
        }
    }
    
    return base_cooldown * cooldown_multiplier;
}
