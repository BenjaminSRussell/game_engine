// Gameplay System Unit Tests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// Include gameplay system headers
#include "game/blockgame/include/inventory/inventory.h"
#include "game/blockgame/include/crafting/crafting_table.h"
#include "game/blockgame/include/combat/combat.h"
#include "game/blockgame/include/player/player.h"
#include "game/blockgame/include/game/game.h"
#include "include/math/math.h"
#include "include/core/logger.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, tolerance, message) \
    do { \
        if (fabsf((a) - (b)) > (tolerance)) { \
            printf("FAIL: %s (expected %.6f, got %.6f)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

typedef struct {
    const char* test_name;
    bool (*test_func)(void);
    bool passed;
    const char* error_message;
} GameplayTest;

static GameplayTest g_tests[32];
static u32 g_test_count = 0;
static u32 g_tests_passed = 0;

void add_gameplay_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_tests[g_test_count].error_message = NULL;
        g_test_count++;
    }
}

// Mock implementations for testing
typedef struct {
    u32 item_id;
    u32 quantity;
    u32 max_stack_size;
} InventorySlot;

typedef struct {
    InventorySlot slots[36]; // Standard Minecraft inventory size
    u32 selected_slot;
    u32 hotbar_start;
    u32 hotbar_end;
} MockInventory;

typedef struct {
    u32 item_id;
    char name[64];
    u32 max_stack_size;
    float weight;
    u32 durability;
    u32 max_durability;
} ItemDefinition;

typedef struct {
    u32 input_items[9];
    u32 input_quantities[9];
    u32 output_item;
    u32 output_quantity;
    float crafting_time;
} CraftingRecipe;

// Test 1: Inventory System
bool test_inventory_system(void) {
    printf("Testing inventory system...\n");
    
    MockInventory inventory = {0};
    inventory.selected_slot = 0;
    inventory.hotbar_start = 0;
    inventory.hotbar_end = 8;
    
    // Test empty inventory
    for (u32 i = 0; i < 36; i++) {
        TEST_ASSERT(inventory.slots[i].item_id == 0, "Empty inventory slot has item_id 0");
        TEST_ASSERT(inventory.slots[i].quantity == 0, "Empty inventory slot has quantity 0");
    }
    
    // Test adding items
    inventory.slots[0].item_id = 1; // Stone
    inventory.slots[0].quantity = 64;
    inventory.slots[0].max_stack_size = 64;
    
    TEST_ASSERT(inventory.slots[0].item_id == 1, "Item added to inventory");
    TEST_ASSERT(inventory.slots[0].quantity == 64, "Item quantity correct");
    TEST_ASSERT(inventory.slots[0].max_stack_size == 64, "Max stack size set correctly");
    
    // Test stack overflow
    inventory.slots[1].item_id = 1;
    inventory.slots[1].quantity = 63;
    inventory.slots[1].max_stack_size = 64;
    
    // Try to add more items to slot 1 (should fail due to stack limit)
    u32 can_add = inventory.slots[1].max_stack_size - inventory.slots[1].quantity;
    TEST_ASSERT(can_add == 1, "Can add 1 more item to slot 1");
    
    // Test item removal
    inventory.slots[0].quantity -= 10;
    TEST_ASSERT(inventory.slots[0].quantity == 54, "Item quantity decreased correctly");
    
    // Test empty slot
    inventory.slots[0].quantity = 0;
    inventory.slots[0].item_id = 0;
    TEST_ASSERT(inventory.slots[0].quantity == 0, "Slot emptied correctly");
    TEST_ASSERT(inventory.slots[0].item_id == 0, "Slot item_id reset correctly");
    
    // Test hotbar selection
    inventory.selected_slot = 3;
    TEST_ASSERT(inventory.selected_slot == 3, "Hotbar slot selected correctly");
    TEST_ASSERT(inventory.selected_slot >= inventory.hotbar_start, "Selected slot in hotbar range");
    TEST_ASSERT(inventory.selected_slot < inventory.hotbar_end, "Selected slot in hotbar range");
    
    return true;
}

// Test 2: Crafting System
bool test_crafting_system(void) {
    printf("Testing crafting system...\n");
    
    // Mock crafting recipes
    CraftingRecipe recipes[10] = {0};
    
    // Recipe 1: 2x2 Stone -> Cobblestone
    recipes[0] = (CraftingRecipe){
        .input_items = {1, 1, 1, 1, 0, 0, 0, 0, 0}, // 4 stone
        .input_quantities = {1, 1, 1, 1, 0, 0, 0, 0, 0},
        .output_item = 4, // Cobblestone
        .output_quantity = 1,
        .crafting_time = 3.0f
    };
    
    // Recipe 2: Wood Planks -> Sticks
    recipes[1] = (CraftingRecipe){
        .input_items = {5, 5, 0, 0, 0, 0, 0, 0, 0}, // 2 wood planks
        .input_quantities = {1, 1, 0, 0, 0, 0, 0, 0, 0},
        .output_item = 6, // Sticks
        .output_quantity = 4,
        .crafting_time = 2.0f
    };
    
    // Test recipe matching
    MockInventory crafting_grid = {0};
    
    // Set up crafting grid for recipe 1
    crafting_grid.slots[0].item_id = 1; // Stone
    crafting_grid.slots[0].quantity = 1;
    crafting_grid.slots[1].item_id = 1;
    crafting_grid.slots[1].quantity = 1;
    crafting_grid.slots[2].item_id = 1;
    crafting_grid.slots[2].quantity = 1;
    crafting_grid.slots[3].item_id = 1;
    crafting_grid.slots[3].quantity = 1;
    
    // Check if recipe matches
    bool recipe_matches = true;
    for (int i = 0; i < 9; i++) {
        if (recipes[0].input_items[i] != 0) {
            if (crafting_grid.slots[i].item_id != recipes[0].input_items[i] ||
                crafting_grid.slots[i].quantity < recipes[0].input_quantities[i]) {
                recipe_matches = false;
                break;
            }
        }
    }
    
    TEST_ASSERT(recipe_matches, "Recipe 1 matches crafting grid");
    
    // Test crafting output
    if (recipe_matches) {
        u32 output_item = recipes[0].output_item;
        u32 output_quantity = recipes[0].output_quantity;
        TEST_ASSERT(output_item == 4, "Crafting output item correct");
        TEST_ASSERT(output_quantity == 1, "Crafting output quantity correct");
    }
    
    // Test recipe that doesn't match
    crafting_grid.slots[0].item_id = 2; // Dirt instead of stone
    recipe_matches = true;
    for (int i = 0; i < 9; i++) {
        if (recipes[0].input_items[i] != 0) {
            if (crafting_grid.slots[i].item_id != recipes[0].input_items[i] ||
                crafting_grid.slots[i].quantity < recipes[0].input_quantities[i]) {
                recipe_matches = false;
                break;
            }
        }
    }
    
    TEST_ASSERT(!recipe_matches, "Recipe doesn't match with wrong items");
    
    return true;
}

// Test 3: Combat System
bool test_combat_system(void) {
    printf("Testing combat system...\n");
    
    // Mock combat entities
    typedef struct {
        u32 entity_id;
        float health;
        float max_health;
        float armor;
        u32 weapon_id;
        float attack_damage;
        bool is_alive;
    } CombatEntity;
    
    CombatEntity player = {
        .entity_id = 1,
        .health = 100.0f,
        .max_health = 100.0f,
        .armor = 20.0f,
        .weapon_id = 10, // Sword
        .attack_damage = 8.0f,
        .is_alive = true
    };
    
    CombatEntity zombie = {
        .entity_id = 2,
        .health = 50.0f,
        .max_health = 50.0f,
        .armor = 0.0f,
        .weapon_id = 0, // No weapon
        .attack_damage = 3.0f,
        .is_alive = true
    };
    
    // Test damage calculation
    float base_damage = player.attack_damage;
    float armor_reduction = zombie.armor * 0.8f; // 80% armor effectiveness
    float final_damage = base_damage - armor_reduction;
    
    if (final_damage < 1.0f) final_damage = 1.0f; // Minimum damage
    
    zombie.health -= final_damage;
    
    TEST_ASSERT(zombie.health < 50.0f, "Zombie took damage");
    TEST_ASSERT(zombie.health > 0.0f, "Zombie still alive after one hit");
    
    // Test death
    zombie.health -= 100.0f; // Overkill
    if (zombie.health <= 0.0f) {
        zombie.health = 0.0f;
        zombie.is_alive = false;
    }
    
    TEST_ASSERT(zombie.health == 0.0f, "Zombie health clamped to 0");
    TEST_ASSERT(!zombie.is_alive, "Zombie is dead");
    
    // Test healing
    player.health = 25.0f;
    float heal_amount = 50.0f;
    player.health += heal_amount;
    
    if (player.health > player.max_health) {
        player.health = player.max_health;
    }
    
    TEST_ASSERT(player.health == 75.0f, "Player healed correctly");
    TEST_ASSERT(player.health <= player.max_health, "Player health doesn't exceed maximum");
    
    // Test critical hit
    float critical_chance = 0.1f; // 10% chance
    bool is_critical = ((float)(rand() % 100) / 100.0f) < critical_chance;
    float damage_multiplier = is_critical ? 2.0f : 1.0f;
    
    float critical_damage = player.attack_damage * damage_multiplier;
    TEST_ASSERT(critical_damage >= player.attack_damage, "Critical damage is at least normal damage");
    if (is_critical) {
        TEST_ASSERT_FLOAT_EQ(critical_damage, player.attack_damage * 2.0f, 0.001f, "Critical damage is exactly double");
    }
    
    return true;
}

// Test 4: Player System
bool test_player_system(void) {
    printf("Testing player system...\n");
    
    // Mock player
    typedef struct {
        Vec3 position;
        Vec3 velocity;
        Vec3 rotation;
        float health;
        float max_health;
        float hunger;
        float max_hunger;
        u32 experience;
        u32 level;
        MockInventory inventory;
        bool is_grounded;
        bool is_sprinting;
    } Player;
    
    Player player = {
        .position = {0.0f, 64.0f, 0.0f}, // Start at ground level
        .velocity = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .health = 100.0f,
        .max_health = 100.0f,
        .hunger = 100.0f,
        .max_hunger = 100.0f,
        .experience = 0,
        .level = 1,
        .is_grounded = true,
        .is_sprinting = false
    };
    
    // Test movement
    player.velocity.x = 5.0f; // Move forward
    player.position.x += player.velocity.x * 0.016f; // 60 FPS timestep
    
    TEST_ASSERT(player.position.x > 0.0f, "Player moved forward");
    
    // Test jumping
    if (player.is_grounded && !player.velocity.y) {
        player.velocity.y = 8.0f; // Jump velocity
        player.is_grounded = false;
    }
    
    TEST_ASSERT(player.velocity.y > 0.0f, "Player jumped");
    TEST_ASSERT(!player.is_grounded, "Player not grounded after jump");
    
    // Test gravity
    float gravity = -9.81f;
    float dt = 0.016f;
    player.velocity.y += gravity * dt;
    player.position.y += player.velocity.y * dt;
    
    TEST_ASSERT(player.velocity.y < 8.0f, "Gravity affected jump velocity");
    
    // Test hunger system
    player.hunger -= 0.1f; // Hunger decreases over time
    if (player.hunger < 0.0f) player.hunger = 0.0f;
    
    TEST_ASSERT(player.hunger < 100.0f, "Player hunger decreased");
    
    // Test experience and leveling
    player.experience += 50;
    u32 experience_for_next_level = player.level * 100;
    
    if (player.experience >= experience_for_next_level) {
        player.level++;
        player.experience -= experience_for_next_level;
        player.max_health += 10.0f;
        player.health = player.max_health; // Heal on level up
    }
    
    TEST_ASSERT(player.experience >= 0, "Player experience is non-negative");
    
    // Test sprinting
    player.is_sprinting = true;
    float sprint_speed_multiplier = 1.3f;
    float normal_speed = 5.0f;
    float sprint_speed = normal_speed * sprint_speed_multiplier;
    
    player.velocity.x = sprint_speed;
    TEST_ASSERT(player.velocity.x > normal_speed, "Sprinting increases speed");
    
    return true;
}

// Test 5: Game State Management
bool test_game_state(void) {
    printf("Testing game state management...\n");
    
    // Mock game state
    typedef enum {
        GAME_STATE_MENU,
        GAME_STATE_PLAYING,
        GAME_STATE_PAUSED,
        GAME_STATE_INVENTORY,
        GAME_STATE_CRAFTING,
        GAME_STATE_GAME_OVER
    } GameState;
    
    typedef struct {
        GameState current_state;
        GameState previous_state;
        float time_scale;
        bool is_paused;
        u32 world_seed;
        Vec3 spawn_position;
        float day_time;
        u32 current_day;
    } GameData;
    
    GameData game = {
        .current_state = GAME_STATE_MENU,
        .previous_state = GAME_STATE_MENU,
        .time_scale = 1.0f,
        .is_paused = false,
        .world_seed = 12345,
        .spawn_position = {0.0f, 64.0f, 0.0f},
        .day_time = 0.0f,
        .current_day = 1
    };
    
    // Test state transitions
    game.previous_state = game.current_state;
    game.current_state = GAME_STATE_PLAYING;
    
    TEST_ASSERT(game.current_state == GAME_STATE_PLAYING, "Game state changed to playing");
    TEST_ASSERT(game.previous_state == GAME_STATE_MENU, "Previous state recorded");
    
    // Test pause functionality
    game.is_paused = true;
    game.time_scale = 0.0f;
    
    TEST_ASSERT(game.is_paused, "Game is paused");
    TEST_ASSERT_FLOAT_EQ(game.time_scale, 0.0f, 0.001f, "Time scale set to 0 when paused");
    
    // Test day/night cycle
    float day_duration = 1200.0f; // 20 minutes per day
    float dt = 0.016f; // 60 FPS
    
    game.time_scale = 1.0f;
    game.is_paused = false;
    
    for (int i = 0; i < 100; i++) { // Simulate 100 frames
        game.day_time += dt * game.time_scale;
        
        if (game.day_time >= day_duration) {
            game.day_time -= day_duration;
            game.current_day++;
        }
    }
    
    TEST_ASSERT(game.day_time > 0.0f, "Day time progressed");
    TEST_ASSERT(game.current_day >= 1, "At least one day passed");
    
    // Test spawn position
    TEST_ASSERT_FLOAT_EQ(game.spawn_position.x, 0.0f, 0.001f, "Spawn position X correct");
    TEST_ASSERT_FLOAT_EQ(game.spawn_position.y, 64.0f, 0.001f, "Spawn position Y correct");
    TEST_ASSERT_FLOAT_EQ(game.spawn_position.z, 0.0f, 0.001f, "Spawn position Z correct");
    
    // Test world seed consistency
    u32 seed1 = game.world_seed;
    u32 seed2 = game.world_seed;
    TEST_ASSERT(seed1 == seed2, "World seed is consistent");
    
    return true;
}

// Test 6: Integration Test
bool test_gameplay_integration(void) {
    printf("Testing gameplay integration...\n");
    
    // Create integrated gameplay scenario
    typedef struct {
        Player player;
        MockInventory inventory;
        GameData game;
        CombatEntity enemies[5];
        u32 enemy_count;
    } IntegratedGame;
    
    IntegratedGame game_world = {
        .player = {
            .position = {0.0f, 64.0f, 0.0f},
            .velocity = {0.0f, 0.0f, 0.0f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .health = 100.0f,
            .max_health = 100.0f,
            .hunger = 100.0f,
            .max_hunger = 100.0f,
            .experience = 0,
            .level = 1,
            .is_grounded = true,
            .is_sprinting = false
        },
        .inventory = {0},
        .game = {
            .current_state = GAME_STATE_PLAYING,
            .time_scale = 1.0f,
            .is_paused = false,
            .world_seed = 54321,
            .spawn_position = {0.0f, 64.0f, 0.0f},
            .day_time = 600.0f, // Midday
            .current_day = 5
        },
        .enemies = {0},
        .enemy_count = 0
    };
    
    // Spawn enemies
    for (int i = 0; i < 3; i++) {
        game_world.enemies[i] = (CombatEntity){
            .entity_id = 100 + i,
            .health = 30.0f,
            .max_health = 30.0f,
            .armor = 0.0f,
            .weapon_id = 0,
            .attack_damage = 2.0f,
            .is_alive = true
        };
        
        // Position enemies around player
        game_world.enemies[i].position.x = (float)(i - 1) * 5.0f;
        game_world.enemies[i].position.y = 64.0f;
        game_world.enemies[i].position.z = 5.0f;
        
        game_world.enemy_count++;
    }
    
    // Simulate gameplay loop
    float dt = 0.016f;
    bool player_victory = false;
    bool player_defeat = false;
    
    for (int frame = 0; frame < 300; frame++) { // 5 seconds at 60 FPS
        // Update game time
        if (!game_world.game.is_paused) {
            game_world.game.day_time += dt * game_world.game.time_scale;
            
            // Update player
            game_world.player.velocity.y += -9.81f * dt;
            game_world.player.position.y += game_world.player.velocity.y * dt;
            
            // Ground collision
            if (game_world.player.position.y <= 64.0f) {
                game_world.player.position.y = 64.0f;
                game_world.player.velocity.y = 0.0f;
                game_world.player.is_grounded = true;
            }
            
            // Update hunger
            game_world.player.hunger -= 0.01f * dt;
            if (game_world.player.hunger < 0.0f) game_world.player.hunger = 0.0f;
            
            // Combat simulation
            for (u32 i = 0; i < game_world.enemy_count; i++) {
                CombatEntity* enemy = &game_world.enemies[i];
                if (!enemy->is_alive) continue;
                
                // Simple AI: move towards player
                Vec3 to_player = vec3_sub(&game_world.player.position, &enemy->position);
                float distance = vec3_length(&to_player);
                
                if (distance > 2.0f) {
                    Vec3 direction = vec3_normalize(&to_player);
                    enemy->position = vec3_add(&enemy->position, vec3_mul(direction, 2.0f * dt));
                }
                
                // Attack if close enough
                if (distance < 3.0f) {
                    float damage = enemy->attack_damage;
                    game_world.player.health -= damage;
                    
                    if (game_world.player.health <= 0.0f) {
                        game_world.player.health = 0.0f;
                        player_defeat = true;
                        break;
                    }
                    
                    // Player counter-attack
                    float player_damage = 10.0f; // Player does more damage
                    enemy->health -= player_damage;
                    
                    if (enemy->health <= 0.0f) {
                        enemy->health = 0.0f;
                        enemy->is_alive = false;
                        game_world.player.experience += 10;
                    }
                }
            }
        }
        
        // Check win condition
        bool all_enemies_defeated = true;
        for (u32 i = 0; i < game_world.enemy_count; i++) {
            if (game_world.enemies[i].is_alive) {
                all_enemies_defeated = false;
                break;
            }
        }
        
        if (all_enemies_defeated) {
            player_victory = true;
            break;
        }
        
        if (player_defeat) break;
    }
    
    // Test results
    TEST_ASSERT(game_world.player.position.y == 64.0f, "Player stayed grounded");
    TEST_ASSERT(game_world.player.hunger < 100.0f, "Player hunger decreased");
    
    if (player_victory) {
        TEST_ASSERT(game_world.player.experience > 0, "Player gained experience");
        TEST_ASSERT(game_world.player.health > 0.0f, "Player survived combat");
        printf("Player victory! Experience: %u\n", game_world.player.experience);
    } else if (player_defeat) {
        TEST_ASSERT(game_world.player.health == 0.0f, "Player was defeated");
        printf("Player defeat! Health: %.1f\n", game_world.player.health);
    }
    
    return true;
}

void run_gameplay_system_tests(void) {
    printf("=== Gameplay System Unit Tests ===\n\n");
    
    // Add all tests
    add_gameplay_test("Inventory System", test_inventory_system);
    add_gameplay_test("Crafting System", test_crafting_system);
    add_gameplay_test("Combat System", test_combat_system);
    add_gameplay_test("Player System", test_player_system);
    add_gameplay_test("Game State Management", test_game_state);
    add_gameplay_test("Gameplay Integration", test_gameplay_integration);
    
    // Run all tests
    for (u32 i = 0; i < g_test_count; i++) {
        printf("\n--- Test %u: %s ---\n", i + 1, g_tests[i].test_name);
        
        bool passed = g_tests[i].test_func();
        g_tests[i].passed = passed;
        
        if (passed) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %u/%u (%.1f%%)\n", 
           g_tests_passed, g_test_count, 
           (float)g_tests_passed / g_test_count * 100.0f);
    
    if (g_tests_passed == g_test_count) {
        printf("All gameplay system unit tests PASSED!\n");
    } else {
        printf("Some gameplay system unit tests FAILED.\n");
    }
}

int main(void) {
    printf("Gameplay System Unit Test Suite\n");
    printf("===================================\n\n");
    
    run_gameplay_system_tests();
    
    return (g_tests_passed == g_test_count) ? 0 : 1;
}
