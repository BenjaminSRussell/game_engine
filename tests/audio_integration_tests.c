/**
 * AUDIO SYSTEM INTEGRATION TESTS IMPLEMENTATION
 */

#include "audio_integration_tests.h"
#include "audio/audio_core.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

void audio_integration_assert(bool condition, const char* message, AudioIntegrationResults* results) {
    results->total_tests++;
    if (condition) {
        results->passed_tests++;
        LOG_INFO("INTEGRATION PASS: %s", message);
    } else {
        results->failed_tests++;
        snprintf(results->last_error, sizeof(results->last_error), "INTEGRATION FAIL: %s", message);
        LOG_ERROR("%s", results->last_error);
    }
}

void audio_integration_reset_results(AudioIntegrationResults* results) {
    memset(results, 0, sizeof(AudioIntegrationResults));
}

double audio_integration_get_time(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

// Mock gameplay systems
static MockPlayer g_player = {{0, 0, 0}, {0, 0, 0}, false, false, 100.0f, 0, false};
static MockEnemy g_enemy = {{10, 0, 0}, 50.0f, true, 1, true};
static MockQuest g_quest = {true, 0.0f, 1, "Test Quest"};
static MockAchievement g_achievement = {false, 1, "Test Achievement", "Complete test quest"};

bool test_footstep_audio_system(void) {
    AudioIntegrationResults results = {0};
    
    audio_core_init();
    
    // Generate footstep sounds
    uint32_t footstep_grass = audio_core_generate_test_tone("footstep_grass", 200.0f, 0.05f);
    uint32_t footstep_stone = audio_core_generate_test_tone("footstep_stone", 400.0f, 0.05f);
    
    audio_integration_assert(footstep_grass != UINT32_MAX, "Footstep grass sound should load");
    audio_integration_assert(footstep_stone != UINT32_MAX, "Footstep stone sound should load");
    
    // Simulate player movement
    mock_player_move(&g_player, 1.0f, 0.0f, 0.0f);
    audio_integration_assert(g_player.is_moving, "Player should be moving");
    
    // Play footstep sounds based on movement
    if (g_player.is_moving) {
        uint32_t footstep_id = audio_core_play_sfx(footstep_grass, 0.3f, 1.0f, false);
        audio_integration_assert(footstep_id != 0, "Footstep sound should play during movement");
    }
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_combat_audio_feedback(void) {
    AudioIntegrationResults results = {0};
    
    audio_core_init();
    
    // Generate combat sounds
    uint32_t sword_swing = audio_core_generate_test_tone("sword_swing", 800.0f, 0.1f);
    uint32_t hit_sound = audio_core_generate_test_tone("hit", 150.0f, 0.15f);
    uint32_t block_sound = audio_core_generate_test_tone("block", 300.0f, 0.1f);
    
    audio_integration_assert(sword_swing != UINT32_MAX, "Sword swing sound should load");
    audio_integration_assert(hit_sound != UINT32_MAX, "Hit sound should load");
    audio_integration_assert(block_sound != UINT32_MAX, "Block sound should load");
    
    // Simulate combat
    mock_player_attack(&g_player, &g_enemy);
    audio_integration_assert(g_player.in_combat, "Player should be in combat");
    
    // Play combat sounds
    uint32_t swing_id = audio_core_play_sfx(sword_swing, 0.7f, 1.2f, false);
    uint32_t hit_id = audio_core_play_sfx(hit_sound, 0.8f, 1.0f, false);
    
    audio_integration_assert(swing_id != 0, "Sword swing should play");
    audio_integration_assert(hit_id != 0, "Hit sound should play");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_achievement_unlock_audio(void) {
    AudioIntegrationResults results = {0};
    
    audio_core_init();
    
    // Generate achievement sounds
    uint32_t achievement_sound = audio_core_generate_test_tone("achievement", 600.0f, 0.3f);
    audio_integration_assert(achievement_sound != UINT32_MAX, "Achievement sound should load");
    
    // Simulate achievement unlock
    mock_achievement_unlock(&g_achievement);
    audio_integration_assert(g_achievement.is_unlocked, "Achievement should be unlocked");
    
    // Play achievement sound
    uint32_t sound_id = audio_core_play_sfx(achievement_sound, 0.9f, 1.0f, false);
    audio_integration_assert(sound_id != 0, "Achievement sound should play");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_quest_progress_audio(void) {
    AudioIntegrationResults results = {0};
    
    audio_core_init();
    
    // Generate quest sounds
    uint32_t progress_sound = audio_core_generate_test_tone("quest_progress", 500.0f, 0.2f);
    uint32_t complete_sound = audio_core_generate_test_tone("quest_complete", 700.0f, 0.4f);
    
    audio_integration_assert(progress_sound != UINT32_MAX, "Quest progress sound should load");
    audio_integration_assert(complete_sound != UINT32_MAX, "Quest complete sound should load");
    
    // Simulate quest progress
    mock_quest_update_progress(&g_quest, 0.25f);
    audio_integration_assert(g_quest.progress > 0.0f, "Quest progress should update");
    
    // Play progress sound
    uint32_t progress_id = audio_core_play_sfx(progress_sound, 0.5f, 1.0f, false);
    audio_integration_assert(progress_id != 0, "Quest progress sound should play");
    
    // Complete quest
    g_quest.progress = 1.0f;
    uint32_t complete_id = audio_core_play_sfx(complete_sound, 0.8f, 1.0f, false);
    audio_integration_assert(complete_id != 0, "Quest complete sound should play");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_performance_impact(void) {
    AudioIntegrationResults results = {0};
    
    audio_core_init();
    
    double start_time = audio_integration_get_time();
    
    // Load many sounds
    uint32_t sound_ids[50];
    for (int i = 0; i < 50; i++) {
        char name[64];
        snprintf(name, sizeof(name), "perf_test_%d", i);
        sound_ids[i] = audio_core_generate_test_tone(name, 440.0f + i * 10.0f, 0.01f);
    }
    
    double load_time = audio_integration_get_time() - start_time;
    audio_integration_assert(load_time < 1.0, "Loading 50 sounds should take less than 1 second");
    
    // Play many sounds simultaneously
    start_time = audio_integration_get_time();
    for (int i = 0; i < 20; i++) {
        audio_core_play_sfx(sound_ids[i], 0.1f, 1.0f, false);
    }
    double play_time = audio_integration_get_time() - start_time;
    audio_integration_assert(play_time < 0.1, "Playing 20 sounds should take less than 0.1 seconds");
    
    // Test audio mixing performance
    float mix_buffer[4096];
    start_time = audio_integration_get_time();
    for (int i = 0; i < 100; i++) {
        audio_core_mix(mix_buffer, 1024);
    }
    double mix_time = audio_integration_get_time() - start_time;
    audio_integration_assert(mix_time < 0.5, "100 audio mixes should take less than 0.5 seconds");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

// Mock system implementations
void mock_gameplay_init(void) {
    memset(&g_player, 0, sizeof(MockPlayer));
    memset(&g_enemy, 0, sizeof(MockEnemy));
    memset(&g_quest, 0, sizeof(MockQuest));
    memset(&g_achievement, 0, sizeof(MockAchievement));
    
    g_player.health = 100.0f;
    g_enemy.health = 50.0f;
    g_enemy.is_alive = true;
    g_quest.is_active = true;
}

void mock_player_move(MockPlayer* player, float dx, float dy, float dz) {
    player->position[0] += dx;
    player->position[1] += dy;
    player->position[2] += dz;
    player->velocity[0] = dx;
    player->velocity[1] = dy;
    player->velocity[2] = dz;
    player->is_moving = (dx != 0 || dy != 0 || dz != 0);
}

void mock_player_attack(MockPlayer* player, MockEnemy* enemy) {
    player->in_combat = true;
    if (enemy->is_alive) {
        enemy->health -= 10.0f;
        if (enemy->health <= 0) {
            enemy->is_alive = false;
        }
    }
}

void mock_quest_update_progress(MockQuest* quest, float delta) {
    if (quest->is_active) {
        quest->progress = fminf(quest->progress + delta, 1.0f);
    }
}

void mock_achievement_unlock(MockAchievement* achievement) {
    achievement->is_unlocked = true;
}

// Main test runner
bool audio_integration_tests_run_all(AudioIntegrationResults* results) {
    audio_integration_reset_results(results);
    double start_time = audio_integration_get_time();
    
    LOG_INFO("Starting comprehensive audio integration tests...");
    
    mock_gameplay_init();
    
    // Run all integration tests
    bool all_passed = true;
    all_passed &= test_footstep_audio_system();
    all_passed &= test_combat_audio_feedback();
    all_passed &= test_achievement_unlock_audio();
    all_passed &= test_quest_progress_audio();
    all_passed &= test_audio_performance_impact();
    
    results->total_test_time = audio_integration_get_time() - start_time;
    
    LOG_INFO("Audio integration tests completed in %.2f seconds", results->total_test_time);
    return all_passed;
}

bool audio_integration_tests_gameplay_scenarios(AudioIntegrationResults* results) {
    audio_integration_reset_results(results);
    mock_gameplay_init();
    
    bool passed = true;
    passed &= test_footstep_audio_system();
    passed &= test_combat_audio_feedback();
    
    return passed;
}

bool audio_integration_tests_multiplayer_sync(AudioIntegrationResults* results) {
    audio_integration_reset_results(results);
    // Placeholder for multiplayer tests
    return true;
}

bool audio_integration_tests_quest_achievements(AudioIntegrationResults* results) {
    audio_integration_reset_results(results);
    mock_gameplay_init();
    
    bool passed = true;
    passed &= test_achievement_unlock_audio();
    passed &= test_quest_progress_audio();
    
    return passed;
}

bool audio_integration_tests_system_integration(AudioIntegrationResults* results) {
    audio_integration_reset_results(results);
    
    bool passed = true;
    passed &= test_audio_performance_impact();
    
    return passed;
}
