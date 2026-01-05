
// src/engine/test/integration/test_engine_integration.c
//
// Purpose: Integration test to verify that Combat, Inventory, and Crafting
// systems work correctly within the engine core.
//
#include <assert.h>
#include <core/engine.h>
#include <core/logger.h>
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/ecs.h>
#include <gameplay/combat/combat_system.h>
#include <gameplay/combat/hitbox.h>
#include <gameplay/crafting/crafting.h>
#include <gameplay/inventory/inventory.h>
#include <gameplay/inventory/item_database.h>
#include <stdio.h>

void test_engine_systems_integration() {
  LOG_INFO("Running Engine Systems Integration Test...");

  // 1. Setup Engine Minimal Config
  EngineConfig config = engine_create_debug_config();
  Engine engine;

  // For testing purposes, we might need a dummy window or mock renderer
  // since we're in a headless environment.
  // Assuming engine_init can handle a minimal setup.
  if (!engine_init(&engine, &config)) {
    LOG_ERROR("Engine init failed for test");
    return;
  }

  World *world = (World *)engine_get_entities(&engine);
  assert(world != NULL);

  // 2. Test Combat Integration
  LOG_INFO("Testing Combat Integration...");
  Entity attacker = ecs_create_entity(world);
  Entity target = ecs_create_entity(world);

  // Add health to target
  HealthComponent health = {.health = 100.0f, .max_health = 100.0f};
  ecs_add_component(world, target, HEALTH_COMPONENT_ID, &health);

  // Add hitbox to target
  HitboxComponent target_hitbox = hitbox_create_sphere(1.0f, 0);
  target_hitbox.world_position = (Vec3){10.0f, 0.0f, 0.0f};
  target_hitbox.active = true;
  ecs_add_component(world, target, HITBOX_COMPONENT_ID, &target_hitbox);

  // Create melee attack at target position
  combat_create_melee_attack(world, attacker, (Vec3){10.0f, 0.0f, 0.0f},
                             (Vec3){0.0f, 0.0f, 1.0f}, 25.0f, 2.0f);

  // Update engine loop once to process combat
  engine_update(&engine, 0.016f); // 60 FPS delta

  // Check if target took damage
  HealthComponent *target_health =
      (HealthComponent *)ecs_get_component(world, target, HEALTH_COMPONENT_ID);
  LOG_INFO("Target Health after attack: %.2f", target_health->health);
  assert(target_health->health < 100.0f);

  // 3. Test Inventory Integration
  LOG_INFO("Testing Inventory Integration...");
  // Create player entity with inventory
  Entity player = ecs_create_entity(world);
  InventoryComponent player_inv = inventory_component_create(30, 9);
  ecs_add_component(world, player, INVENTORY_COMPONENT_ID, &player_inv);

  // Add items from database
  ItemStack wood_stack = item_create_stack(3001, 10); // Wood
  bool added = container_add_item(player_inv.main_inventory, wood_stack, NULL);
  assert(added);
  LOG_INFO("Added 10 Wood to player inventory");

  // 4. Test Crafting Integration
  LOG_INFO("Testing Crafting Integration...");
  // Try to craft sticks from planks (recipe registered in defaults)
  // First we need planks
  ItemStack plank_stack = item_create_stack(3002, 2); // Planks
  container_add_item(player_inv.main_inventory, plank_stack, NULL);

  // Setup 2x1 grid for sticks (2 planks vertically)
  ItemStack grid[4] = {0};
  grid[0] = item_create_stack(3002, 1);
  grid[2] = item_create_stack(3002, 1);

  const Recipe *stick_recipe = crafting_find_shaped_recipe(grid, 2);
  assert(stick_recipe != NULL);
  LOG_INFO("Found Stick recipe in integration test");

  ItemStack result;
  bool crafted =
      crafting_craft(player_inv.main_inventory, stick_recipe, &result);
  assert(crafted);
  LOG_INFO("Successfully crafted %d Sticks!", result.quantity);

  // Cleanup
  engine_shutdown(&engine);
  LOG_INFO("Engine Systems Integration Test PASSED! ✅");
}

int main() {
  test_engine_systems_integration();
  return 0;
}
