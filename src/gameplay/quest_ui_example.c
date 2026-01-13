#include "quest_ui.h"
#include "engine/include/ui/ui_manager.h"
#include <stdio.h>
#include <stdlib.h>

// Example quest data
static quest_t example_quests[] = {
    {
        .id = 0,
        .title = "First Steps",
        .description = "Welcome to the world! Complete these basic tasks to get started with your adventure.",
        .objective_count = 3,
        .reward_count = 2,
        .level_requirement = 1,
        .is_repeatable = false,
        .time_limit_seconds = 0,
        .auto_accept = true,
        .prerequisite_count = 0
    },
    {
        .id = 0,
        .title = "The Lost Artifact",
        .description = "An ancient artifact has been stolen from the village. Find it and return it safely.",
        .objective_count = 2,
        .reward_count = 3,
        .level_requirement = 5,
        .is_repeatable = false,
        .time_limit_seconds = 3600, // 1 hour
        .auto_accept = false,
        .prerequisite_count = 1
    },
    {
        .id = 0,
        .title = "Master Crafter",
        .description = "Craft various items to prove your skills as a master craftsman.",
        .objective_count = 4,
        .reward_count = 2,
        .level_requirement = 10,
        .is_repeatable = true,
        .time_limit_seconds = 0,
        .auto_accept = false,
        .prerequisite_count = 0
    }
};

// Example objectives for First Steps quest
static quest_objective_t first_steps_objectives[] = {
    {
        .type = QUEST_OBJECTIVE_EXPLORE,
        .target = "village_center",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Visit the village center"
    },
    {
        .type = QUEST_OBJECTIVE_TALK,
        .target = "village_elder",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Speak with the village elder"
    },
    {
        .type = QUEST_OBJECTIVE_COLLECT,
        .target = "wood",
        .required_amount = 10,
        .current_amount = 0,
        .completed = false,
        .description = "Collect 10 pieces of wood"
    }
};

// Example rewards for First Steps quest
static quest_reward_t first_steps_rewards[] = {
    {
        .type = QUEST_REWARD_EXPERIENCE,
        .amount = 100,
        .item_id = "",
        .description = "100 Experience Points"
    },
    {
        .type = QUEST_REWARD_ITEM,
        .amount = 1,
        .item_id = "basic_sword",
        .description = "Basic Sword"
    }
};

// Example objectives for The Lost Artifact quest
static quest_objective_t artifact_objectives[] = {
    {
        .type = QUEST_OBJECTIVE_EXPLORE,
        .target = "ancient_ruins",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Find the ancient ruins"
    },
    {
        .type = QUEST_OBJECTIVE_COLLECT,
        .target = "ancient_artifact",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Recover the ancient artifact"
    }
};

// Example rewards for The Lost Artifact quest
static quest_reward_t artifact_rewards[] = {
    {
        .type = QUEST_REWARD_EXPERIENCE,
        .amount = 500,
        .item_id = "",
        .description = "500 Experience Points"
    },
    {
        .type = QUEST_REWARD_CURRENCY,
        .amount = 1000,
        .item_id = "",
        .description = "1000 Gold Coins"
    },
    {
        .type = QUEST_REWARD_REPUTATION,
        .amount = 50,
        .item_id = "",
        .description = "50 Reputation with Village"
    }
};

// Example objectives for Master Crafter quest
static quest_objective_t crafting_objectives[] = {
    {
        .type = QUEST_OBJECTIVE_CRAFT,
        .target = "wooden_sword",
        .required_amount = 5,
        .current_amount = 0,
        .completed = false,
        .description = "Craft 5 Wooden Swords"
    },
    {
        .type = QUEST_OBJECTIVE_CRAFT,
        .target = "iron_armor",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Craft 1 Iron Armor Set"
    },
    {
        .type = QUEST_OBJECTIVE_CRAFT,
        .target = "health_potion",
        .required_amount = 10,
        .current_amount = 0,
        .completed = false,
        .description = "Craft 10 Health Potions"
    },
    {
        .type = QUEST_OBJECTIVE_CRAFT,
        .target = "magic_wand",
        .required_amount = 1,
        .current_amount = 0,
        .completed = false,
        .description = "Craft 1 Magic Wand"
    }
};

// Example rewards for Master Crafter quest
static quest_reward_t crafting_rewards[] = {
    {
        .type = QUEST_REWARD_EXPERIENCE,
        .amount = 1000,
        .item_id = "",
        .description = "1000 Experience Points"
    },
    {
        .type = QUEST_REWARD_ITEM,
        .amount = 1,
        .item_id = "master_crafter_hammer",
        .description = "Master Crafter's Hammer"
    }
};

// Initialize example quests with objectives and rewards
void quest_example_initialize_quests(quest_manager_t* manager) {
    if (!manager) return;
    
    // Setup First Steps quest
    example_quests[0].objectives = first_steps_objectives;
    example_quests[0].rewards = first_steps_rewards;
    
    // Setup The Lost Artifact quest
    example_quests[1].objectives = artifact_objectives;
    example_quests[1].rewards = artifact_rewards;
    example_quests[1].prerequisite_quest_ids[0] = 1; // Requires First Steps quest
    example_quests[1].prerequisite_count = 1;
    
    // Setup Master Crafter quest
    example_quests[2].objectives = crafting_objectives;
    example_quests[2].rewards = crafting_rewards;
    
    // Add quests to manager
    for (int i = 0; i < 3; i++) {
        int quest_id = quest_manager_add_quest(manager, &example_quests[i]);
        printf("Added quest '%s' with ID %d\n", example_quests[i].title, quest_id);
    }
}

// Demonstrate quest UI integration
void quest_example_demonstrate_ui(void) {
    printf("=== Quest UI System Demonstration ===\n\n");
    
    // Create quest manager
    quest_manager_t* quest_manager = quest_manager_create();
    if (!quest_manager) {
        printf("Failed to create quest manager\n");
        return;
    }
    
    // Create UI manager (in a real implementation, this would be created by the engine)
    UIManager* ui_manager = create_ui_manager();
    if (!ui_manager) {
        printf("Failed to create UI manager\n");
        quest_manager_destroy(quest_manager);
        return;
    }
    
    // Initialize UI manager
    if (!ui_manager->init(ui_manager, 1000)) {
        printf("Failed to initialize UI manager\n");
        quest_manager_destroy(quest_manager);
        return;
    }
    
    // Create quest UI context
    quest_ui_context_t* quest_ui = quest_ui_create(quest_manager, ui_manager);
    if (!quest_ui) {
        printf("Failed to create quest UI context\n");
        ui_manager->shutdown(ui_manager);
        quest_manager_destroy(quest_manager);
        return;
    }
    
    // Initialize example quests
    quest_example_initialize_quests(quest_manager);
    
    printf("Quest system initialized with %d quests\n", quest_manager->quest_count);
    printf("Player ID: %d\n\n", quest_ui->current_player_id);
    
    // Set up quest event callbacks
    quest_manager_set_callbacks(quest_manager,
        (quest_started_callback_t)quest_ui_on_quest_started,
        (quest_completed_callback_t)quest_ui_on_quest_completed,
        (quest_objective_updated_callback_t)quest_ui_on_objective_updated,
        (quest_failed_callback_t)quest_ui_on_quest_failed);
    
    // Simulate quest progression
    printf("=== Simulating Quest Progression ===\n\n");
    
    // Start the first quest (auto-accept)
    printf("Starting quest 'First Steps'...\n");
    bool started = quest_manager_start_quest(quest_manager, quest_ui->current_player_id, 1);
    printf("Quest started: %s\n\n", started ? "SUCCESS" : "FAILED");
    
    // Update some objectives
    printf("Updating objectives...\n");
    quest_manager_update_objective(quest_manager, quest_ui->current_player_id, 1, 0, 1); // Visit village center
    quest_manager_update_objective(quest_manager, quest_ui->current_player_id, 1, 2, 5); // Collect 5 wood
    
    // Show quest log
    printf("Showing quest log...\n");
    quest_ui_show_quest_log(quest_ui, true);
    
    // Simulate some time passing for notifications
    printf("\nSimulating quest notifications (would appear on screen)...\n");
    quest_ui_update(quest_ui, 0.1f); // Update UI
    
    // Complete the first quest
    printf("Completing remaining objectives...\n");
    quest_manager_update_objective(quest_manager, quest_ui->current_player_id, 1, 1, 1); // Talk to elder
    quest_manager_update_objective(quest_manager, quest_ui->current_player_id, 1, 2, 10); // Collect remaining wood
    
    printf("Completing quest 'First Steps'...\n");
    bool completed = quest_manager_complete_quest(quest_manager, quest_ui->current_player_id, 1);
    printf("Quest completed: %s\n\n", completed ? "SUCCESS" : "FAILED");
    
    // Start the second quest (now available)
    printf("Starting quest 'The Lost Artifact'...\n");
    started = quest_manager_start_quest(quest_manager, quest_ui->current_player_id, 2);
    printf("Quest started: %s\n", started ? "SUCCESS" : "FAILED");
    
    // Track the quest
    if (started) {
        quest_ui_track_quest(quest_ui, 2);
        printf("Now tracking quest 'The Lost Artifact'\n");
    }
    
    // Show final quest status
    printf("\n=== Final Quest Status ===\n");
    int active_count = 0;
    player_quest_t* active_quests = quest_manager_get_active_quests(quest_manager, quest_ui->current_player_id, &active_count);
    
    printf("Active quests: %d\n", active_count);
    for (int i = 0; i < active_count; i++) {
        quest_t* quest = quest_manager_get_quest(quest_manager, active_quests[i].quest_id);
        if (quest) {
            float progress = quest_get_completion_percentage(&active_quests[i], quest);
            printf("  - %s: %.1f%% complete\n", quest->title, progress * 100.0f);
        }
    }
    
    free(active_quests);
    
    printf("\n=== Quest UI Features ===\n");
    printf("✓ Quest log window with quest list\n");
    printf("✓ Quest details panel with objectives and rewards\n");
    printf("✓ On-screen notifications for quest events\n");
    printf("✓ Progress tracking and completion percentage\n");
    printf("✓ Quest tracking system\n");
    printf("✓ Integration with existing UI manager\n");
    printf("✓ Configurable notification system\n");
    printf("✓ Support for multiple quest states\n");
    
    // Cleanup
    printf("\nCleaning up...\n");
    quest_ui_destroy(quest_ui);
    ui_manager->shutdown(ui_manager);
    quest_manager_destroy(quest_manager);
    
    printf("Quest UI demonstration complete!\n");
}

// Example of how to integrate with game loop
void quest_example_game_loop_integration(void) {
    printf("\n=== Game Loop Integration Example ===\n");
    
    // These would be called from your main game loop
    
    printf("In your game loop:\n");
    printf("1. quest_ui_update(quest_ui, delta_time);\n");
    printf("2. quest_ui_render(quest_ui);\n");
    printf("3. Handle input events for quest log toggle\n");
    printf("4. Call quest manager functions when game events occur\n");
    
    printf("\nInput handling example:\n");
    printf("if (key_pressed(KEY_Q)) {\n");
    printf("    quest_ui_show_quest_log(quest_ui, !quest_ui->config.show_quest_log);\n");
    printf("}\n");
    
    printf("\nGame event integration:\n");
    printf("When player kills enemy:\n");
    printf("    quest_manager_update_objective(manager, player_id, quest_id, objective_index, new_count);\n");
    printf("\nWhen player collects item:\n");
    printf("    quest_manager_update_objective(manager, player_id, quest_id, objective_index, new_count);\n");
    printf("\nWhen player visits location:\n");
    printf("    quest_manager_update_objective(manager, player_id, quest_id, objective_index, 1);\n");
}

// Main function for demonstration
int main(void) {
    quest_example_demonstrate_ui();
    quest_example_game_loop_integration();
    return 0;
}
