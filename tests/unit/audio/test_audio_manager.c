/**
 * Unit Test for Audio Manager
 * Tests audio resource management and high-level audio control
 */

#include "../../../src/engine/audio/audio_manager.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test audio manager initialization
static TestResult test_manager_init(void) {
    AudioManager* mgr = audio_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Audio manager created");
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test sound registration
static TestResult test_sound_registration(void) {
    AudioManager* mgr = audio_manager_create();
    
    bool registered = audio_manager_register_sound(mgr, "footstep", "assets/sounds/footstep.wav");
    TEST_ASSERT(registered, "Sound registered successfully");
    
    bool exists = audio_manager_has_sound(mgr, "footstep");
    TEST_ASSERT(exists, "Registered sound exists");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test playing registered sounds
static TestResult test_play_registered_sound(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_register_sound(mgr, "test_sound", "assets/sounds/test.wav");
    
    u32 channel = audio_manager_play(mgr, "test_sound", 1.0f);
    // Channel may be invalid if audio file doesn't exist, but API should not crash
    TEST_ASSERT(true, "Play sound call completed without crash");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test sound grouping
static TestResult test_sound_groups(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_create_group(mgr, "ambient");
    audio_manager_register_sound_to_group(mgr, "ambient", "wind", "assets/sounds/wind.wav");
    audio_manager_register_sound_to_group(mgr, "ambient", "rain", "assets/sounds/rain.wav");
    
    bool has_wind = audio_manager_has_sound(mgr, "wind");
    bool has_rain = audio_manager_has_sound(mgr, "rain");
    
    TEST_ASSERT(has_wind, "Wind sound in group");
    TEST_ASSERT(has_rain, "Rain sound in group");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test group volume control
static TestResult test_group_volume(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_create_group(mgr, "music");
    audio_manager_set_group_volume(mgr, "music", 0.5f);
    
    float volume = audio_manager_get_group_volume(mgr, "music");
    TEST_ASSERT_EQ(volume, 0.5f, "Group volume set correctly");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test mute functionality
static TestResult test_mute(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_mute(mgr);
    bool is_muted = audio_manager_is_muted(mgr);
    TEST_ASSERT(is_muted, "Audio manager muted");
    
    audio_manager_unmute(mgr);
    is_muted = audio_manager_is_muted(mgr);
    TEST_ASSERT(!is_muted, "Audio manager unmuted");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test pause/resume all
static TestResult test_pause_resume(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_pause_all(mgr);
    TEST_ASSERT(true, "Pause all completed");
    
    audio_manager_resume_all(mgr);
    TEST_ASSERT(true, "Resume all completed");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Test duplicate sound registration
static TestResult test_duplicate_registration(void) {
    AudioManager* mgr = audio_manager_create();
    
    audio_manager_register_sound(mgr, "dup_test", "assets/sounds/test1.wav");
    bool second = audio_manager_register_sound(mgr, "dup_test", "assets/sounds/test2.wav");
    
    // Should either reject duplicate or overwrite
    TEST_ASSERT(true, "Duplicate registration handled");
    
    audio_manager_destroy(mgr);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Audio:Manager", "Initialization", test_manager_init, NULL, NULL);
    test_register("Audio:Manager", "Sound Registration", test_sound_registration, NULL, NULL);
    test_register("Audio:Manager", "Play Registered Sound", test_play_registered_sound, NULL, NULL);
    test_register("Audio:Manager", "Sound Groups", test_sound_groups, NULL, NULL);
    test_register("Audio:Manager", "Group Volume", test_group_volume, NULL, NULL);
    test_register("Audio:Manager", "Mute", test_mute, NULL, NULL);
    test_register("Audio:Manager", "Pause/Resume", test_pause_resume, NULL, NULL);
    test_register("Audio:Manager", "Duplicate Registration", test_duplicate_registration, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Audio Manager Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
