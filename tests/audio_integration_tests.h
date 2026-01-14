/**
 * AUDIO SYSTEM INTEGRATION TESTS
 * End-to-end gameplay tests for audio functionality
 */

#ifndef AUDIO_INTEGRATION_TESTS_H
#define AUDIO_INTEGRATION_TESTS_H

#include <stdint.h>
#include <stdbool.h>

// Integration test result structure
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char last_error[256];
    double total_test_time;
} AudioIntegrationResults;

// Test suite functions
bool audio_integration_tests_run_all(AudioIntegrationResults* results);
bool audio_integration_tests_gameplay_scenarios(AudioIntegrationResults* results);
bool audio_integration_tests_multiplayer_sync(AudioIntegrationResults* results);
bool audio_integration_tests_quest_achievements(AudioIntegrationResults* results);
bool audio_integration_tests_system_integration(AudioIntegrationResults* results);

// Gameplay scenario tests
bool test_footstep_audio_system(void);
bool test_combat_audio_feedback(void);
bool test_environmental_ambience(void);
bool test_music_adaptation(void);
bool test_ui_audio_responses(void);
bool test_vehicle_audio(void);
bool test_weapon_audio_system(void);
bool test_npc_voice_system(void);

// Multiplayer tests
bool test_voice_chat_integration(void);
bool test_synchronized_events(void);
bool test_network_audio_latency(void);
bool test_multiplayer_ambience(void);
bool test_positional_audio_sync(void);

// Quest/Achievement tests
bool test_achievement_unlock_audio(void);
bool test_quest_progress_audio(void);
bool test_level_up_audio(void);
bool test_story_event_audio(void);
bool test_challenge_completion_audio(void);

// System integration tests
bool test_audio_physics_integration(void);
bool test_audio_rendering_sync(void);
bool test_audio_input_system(void);
bool test_audio_performance_impact(void);
bool test_audio_memory_integration(void);

// Helper functions
void audio_integration_assert(bool condition, const char* message, AudioIntegrationResults* results);
void audio_integration_log(const char* message, AudioIntegrationResults* results);
void audio_integration_reset_results(AudioIntegrationResults* results);
double audio_integration_get_time(void);

// Mock gameplay systems
typedef struct {
    float position[3];
    float velocity[3];
    bool is_moving;
    bool is_running;
    float health;
    uint32_t current_weapon;
    bool in_combat;
} MockPlayer;

typedef struct {
    float position[3];
    float health;
    bool is_hostile;
    uint32_t enemy_type;
    bool is_alive;
} MockEnemy;

typedef struct {
    bool is_active;
    float progress;
    uint32_t quest_id;
    char name[64];
} MockQuest;

typedef struct {
    bool is_unlocked;
    uint32_t achievement_id;
    char name[64];
    char description[128];
} MockAchievement;

// Mock system functions
void mock_gameplay_init(void);
void mock_gameplay_shutdown(void);
void mock_player_move(MockPlayer* player, float dx, float dy, float dz);
void mock_player_attack(MockPlayer* player, MockEnemy* enemy);
void mock_quest_update_progress(MockQuest* quest, float delta);
void mock_achievement_unlock(MockAchievement* achievement);

#endif // AUDIO_INTEGRATION_TESTS_H
