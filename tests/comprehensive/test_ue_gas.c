/**
 * UNREAL ENGINE PARITY TESTS - GAMEPLAY ABILITY SYSTEM (GAS)
 * Test-Driven Development for Abilities, Effects, Attributes, and Tags
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// ABILITY SYSTEM COMPONENT TESTS
// =============================================================================

static TestResult test_gas_creation(void) {
    AbilitySystemComponent* asc = gas_create(1); // Entity 1
    TEST_ASSERT_NOT_NULL(asc, "ASC should be created");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_attributes(void) {
    AbilitySystemComponent* asc = gas_create(1);
    
    // Initialize attributes
    AttributeDef health = {
        .name = "Health",
        .base_value = 100.0f,
        .current_value = 100.0f,
        .min_value = 0.0f,
        .max_value = 100.0f
    };
    
    AttributeDef mana = {
        .name = "Mana",
        .base_value = 50.0f,
        .current_value = 50.0f,
        .min_value = 0.0f,
        .max_value = 100.0f
    };
    
    gas_init_attribute(asc, &health);
    gas_init_attribute(asc, &mana);
    
    // Get and verify
    float current_health = gas_get_attribute(asc, "Health");
    TEST_ASSERT_FLOAT_EQ(current_health, 100.0f, 0.01f, "Health should be 100");
    
    // Modify
    gas_set_attribute(asc, "Health", 75.0f);
    current_health = gas_get_attribute(asc, "Health");
    TEST_ASSERT_FLOAT_EQ(current_health, 75.0f, 0.01f, "Health should be 75");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_tags(void) {
    AbilitySystemComponent* asc = gas_create(1);
    
    GameplayTag stunned = {.hash = 0x12345678};
    GameplayTag burning = {.hash = 0x87654321};
    GameplayTag poisoned = {.hash = 0xDEADBEEF};
    
    // Add tags
    gas_add_tag(asc, stunned);
    gas_add_tag(asc, burning);
    
    TEST_ASSERT_TRUE(gas_has_tag(asc, stunned), "Should have stunned tag");
    TEST_ASSERT_TRUE(gas_has_tag(asc, burning), "Should have burning tag");
    TEST_ASSERT_FALSE(gas_has_tag(asc, poisoned), "Should not have poisoned tag");
    
    // Remove tag
    gas_remove_tag(asc, stunned);
    TEST_ASSERT_FALSE(gas_has_tag(asc, stunned), "Stunned should be removed");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_tag_queries(void) {
    AbilitySystemComponent* asc = gas_create(1);
    
    GameplayTag tag1 = {.hash = 1};
    GameplayTag tag2 = {.hash = 2};
    GameplayTag tag3 = {.hash = 3};
    GameplayTag tag4 = {.hash = 4};
    
    gas_add_tag(asc, tag1);
    gas_add_tag(asc, tag2);
    gas_add_tag(asc, tag3);
    
    // Has any
    GameplayTag any_check[] = {tag2, tag4};
    TEST_ASSERT_TRUE(gas_has_any_tag(asc, any_check, 2), "Should have at least one");
    
    // Has all
    GameplayTag all_check[] = {tag1, tag2};
    TEST_ASSERT_TRUE(gas_has_all_tags(asc, all_check, 2), "Should have all");
    
    GameplayTag all_fail[] = {tag1, tag4};
    TEST_ASSERT_FALSE(gas_has_all_tags(asc, all_fail, 2), "Should not have all");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_ability_activation(void) {
    AbilitySystemComponent* asc = gas_create(1);
    
    // Give ability
    GameplayAbility ability = {0}; // Stub
    Handle ability_handle = gas_give_ability(asc, &ability);
    TEST_ASSERT_TRUE(ability_handle != 0, "Should grant ability");
    
    // Check activation
    bool can_activate = gas_can_activate_ability(asc, ability_handle);
    TEST_ASSERT_TRUE(can_activate, "Should be able to activate");
    
    // Activate
    bool activated = gas_try_activate_ability(asc, ability_handle);
    TEST_ASSERT_TRUE(activated, "Should activate ability");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_effects(void) {
    AbilitySystemComponent* source = gas_create(1);
    AbilitySystemComponent* target = gas_create(2);
    
    // Create damage effect
    GameplayEffect damage_effect = {0}; // Stub
    
    // Apply effect
    Handle effect_handle = gas_apply_effect(target, &damage_effect, source, 1.0f);
    TEST_ASSERT_TRUE(effect_handle != 0, "Effect should be applied");
    TEST_ASSERT_TRUE(gas_has_effect(target, effect_handle), "Target should have effect");
    
    // Remove effect
    gas_remove_effect(target, effect_handle);
    TEST_ASSERT_FALSE(gas_has_effect(target, effect_handle), "Effect should be removed");
    
    gas_destroy(source);
    gas_destroy(target);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_gas_tests(void) {
    TEST_REGISTER("UE:GAS", "ASC creation", test_gas_creation);
    TEST_REGISTER("UE:GAS", "Attributes", test_gas_attributes);
    TEST_REGISTER("UE:GAS", "Tags", test_gas_tags);
    TEST_REGISTER("UE:GAS", "Tag queries", test_gas_tag_queries);
    TEST_REGISTER("UE:GAS", "Ability activation", test_gas_ability_activation);
    TEST_REGISTER("UE:GAS", "Effects", test_gas_effects);
}
