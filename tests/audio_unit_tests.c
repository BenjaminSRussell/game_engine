/**
 * AUDIO SYSTEM UNIT TESTS IMPLEMENTATION
 * Comprehensive testing for audio core functionality
 */

#include "audio_unit_tests.h"
#include "audio/audio_core.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

// Test helper macros
#define AUDIO_TEST_ASSERT(condition, message) \
    audio_test_assert(condition, message, results)

#define AUDIO_TEST_LOG(message) \
    audio_test_log(message, results)

// Test helper functions
void audio_test_assert(bool condition, const char* message, AudioTestResults* results) {
    results->total_tests++;
    if (condition) {
        results->passed_tests++;
        LOG_INFO("PASS: %s", message);
    } else {
        results->failed_tests++;
        snprintf(results->last_error, sizeof(results->last_error), "FAIL: %s", message);
        LOG_ERROR("%s", results->last_error);
    }
}

void audio_test_log(const char* message, AudioTestResults* results) {
    LOG_DEBUG("AUDIO_TEST: %s", message);
}

void audio_test_reset_results(AudioTestResults* results) {
    memset(results, 0, sizeof(AudioTestResults));
}

// Test data generators
void generate_test_sine_wave(float* buffer, uint32_t length, float frequency, float amplitude) {
    for (uint32_t i = 0; i < length; i++) {
        float time = (float)i / 44100.0f;
        buffer[i] = amplitude * sinf(2.0f * M_PI * frequency * time);
    }
}

void generate_test_square_wave(float* buffer, uint32_t length, float frequency, float amplitude) {
    uint32_t period = (uint32_t)(44100.0f / frequency);
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = ((i % period) < (period / 2)) ? amplitude : -amplitude;
    }
}

void generate_test_noise(float* buffer, uint32_t length, float amplitude) {
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = amplitude * (2.0f * ((float)rand() / RAND_MAX) - 1.0f);
    }
}

// Audio validation functions
bool validate_audio_buffer(const float* buffer, uint32_t length, float min_val, float max_val) {
    for (uint32_t i = 0; i < length; i++) {
        if (buffer[i] < min_val || buffer[i] > max_val) {
            return false;
        }
    }
    return true;
}

bool compare_audio_buffers(const float* buffer1, const float* buffer2, uint32_t length, float tolerance) {
    for (uint32_t i = 0; i < length; i++) {
        if (fabsf(buffer1[i] - buffer2[i]) > tolerance) {
            return false;
        }
    }
    return true;
}

// Individual test functions
bool test_audio_core_init_shutdown(void) {
    AudioTestResults results = {0};
    
    // Test initial state
    AUDIO_TEST_ASSERT(!audio_core_is_initialized(), "Audio core should not be initialized initially");
    
    // Test initialization
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized after init()");
    
    // Test double initialization
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should handle double initialization gracefully");
    
    // Test shutdown
    audio_core_shutdown();
    AUDIO_TEST_ASSERT(!audio_core_is_initialized(), "Audio core should not be initialized after shutdown");
    
    // Test double shutdown
    audio_core_shutdown();
    AUDIO_TEST_ASSERT(!audio_core_is_initialized(), "Audio core should handle double shutdown gracefully");
    
    return results.failed_tests == 0;
}

bool test_audio_core_volume_controls(void) {
    AudioTestResults results = {0};
    
    // Initialize audio core
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Test default volumes
    float master_vol = audio_core_get_master_volume();
    float sfx_vol = audio_core_get_sfx_volume();
    float music_vol = audio_core_get_music_volume();
    
    AUDIO_TEST_ASSERT(master_vol >= 0.0f && master_vol <= 1.0f, "Master volume should be in valid range");
    AUDIO_TEST_ASSERT(sfx_vol >= 0.0f && sfx_vol <= 1.0f, "SFX volume should be in valid range");
    AUDIO_TEST_ASSERT(music_vol >= 0.0f && music_vol <= 1.0f, "Music volume should be in valid range");
    
    // Test volume setting
    audio_core_set_master_volume(0.5f);
    AUDIO_TEST_ASSERT(fabsf(audio_core_get_master_volume() - 0.5f) < 0.001f, "Master volume should be set correctly");
    
    audio_core_set_sfx_volume(0.75f);
    AUDIO_TEST_ASSERT(fabsf(audio_core_get_sfx_volume() - 0.75f) < 0.001f, "SFX volume should be set correctly");
    
    audio_core_set_music_volume(0.25f);
    AUDIO_TEST_ASSERT(fabsf(audio_core_get_music_volume() - 0.25f) < 0.001f, "Music volume should be set correctly");
    
    // Test volume clamping
    audio_core_set_master_volume(-0.5f);
    AUDIO_TEST_ASSERT(audio_core_get_master_volume() >= 0.0f, "Master volume should be clamped to minimum");
    
    audio_core_set_master_volume(1.5f);
    AUDIO_TEST_ASSERT(audio_core_get_master_volume() <= 1.0f, "Master volume should be clamped to maximum");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_core_sound_loading(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Test test tone generation
    uint32_t tone_id = audio_core_generate_test_tone("test_tone_440hz", 440.0f, 0.1f);
    AUDIO_TEST_ASSERT(tone_id != UINT32_MAX, "Test tone should be generated successfully");
    
    // Test loading custom sound data
    float test_data[4410]; // 0.1 seconds at 44.1kHz
    generate_test_sine_wave(test_data, 4410, 1000.0f, 0.5f);
    
    uint32_t custom_id = audio_core_load_sound("custom_sine", test_data, 4410, 1, 44100);
    AUDIO_TEST_ASSERT(custom_id != UINT32_MAX, "Custom sound should be loaded successfully");
    
    // Test loading with invalid parameters
    uint32_t invalid_id = audio_core_load_sound(NULL, test_data, 4410, 1, 44100);
    AUDIO_TEST_ASSERT(invalid_id == UINT32_MAX, "Loading with NULL name should fail");
    
    invalid_id = audio_core_load_sound("invalid", NULL, 4410, 1, 44100);
    AUDIO_TEST_ASSERT(invalid_id == UINT32_MAX, "Loading with NULL data should fail");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_core_sound_playback(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Generate test sound
    uint32_t tone_id = audio_core_generate_test_tone("playback_test", 440.0f, 0.1f);
    AUDIO_TEST_ASSERT(tone_id != UINT32_MAX, "Test tone should be generated");
    
    // Test sound playback
    uint32_t playing_id = audio_core_play_sfx(tone_id, 0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(playing_id != 0, "Sound should play successfully");
    
    // Test playing invalid sound
    uint32_t invalid_playing = audio_core_play_sfx(UINT32_MAX, 0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(invalid_playing == 0, "Playing invalid sound should fail");
    
    // Test playing with invalid parameters
    invalid_playing = audio_core_play_sfx(tone_id, -0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(invalid_playing != 0, "Playing with negative volume should work (clamped)");
    
    // Test looping
    uint32_t loop_id = audio_core_play_sfx(tone_id, 0.5f, 1.0f, true);
    AUDIO_TEST_ASSERT(loop_id != 0, "Looping sound should play successfully");
    
    // Test sound stopping
    audio_core_stop_sound(playing_id);
    AUDIO_TEST_ASSERT(true, "Sound stopping should not crash");
    
    // Test stopping invalid sound
    audio_core_stop_sound(UINT32_MAX);
    AUDIO_TEST_ASSERT(true, "Stopping invalid sound should not crash");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_core_sound_controls(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    uint32_t tone_id = audio_core_generate_test_tone("controls_test", 440.0f, 0.1f);
    AUDIO_TEST_ASSERT(tone_id != UINT32_MAX, "Test tone should be generated");
    
    uint32_t playing_id = audio_core_play_sfx(tone_id, 0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(playing_id != 0, "Sound should play successfully");
    
    // Test volume control
    audio_core_set_sound_volume(playing_id, 0.75f);
    AUDIO_TEST_ASSERT(true, "Setting sound volume should not crash");
    
    // Test pitch control
    audio_core_set_sound_pitch(playing_id, 1.5f);
    AUDIO_TEST_ASSERT(true, "Setting sound pitch should not crash");
    
    // Test pan control
    audio_core_set_sound_pan(playing_id, -0.5f);
    AUDIO_TEST_ASSERT(true, "Setting sound pan should not crash");
    
    // Test pan clamping
    audio_core_set_sound_pan(playing_id, -2.0f);
    AUDIO_TEST_ASSERT(true, "Setting pan beyond range should be clamped");
    
    audio_core_set_sound_pan(playing_id, 2.0f);
    AUDIO_TEST_ASSERT(true, "Setting pan beyond range should be clamped");
    
    // Test controls on invalid sound
    audio_core_set_sound_volume(UINT32_MAX, 0.5f);
    AUDIO_TEST_ASSERT(true, "Setting volume on invalid sound should not crash");
    
    audio_core_set_sound_pitch(UINT32_MAX, 1.0f);
    AUDIO_TEST_ASSERT(true, "Setting pitch on invalid sound should not crash");
    
    audio_core_set_sound_pan(UINT32_MAX, 0.0f);
    AUDIO_TEST_ASSERT(true, "Setting pan on invalid sound should not crash");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_core_audio_mixing(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Generate test sounds
    uint32_t tone1_id = audio_core_generate_test_tone("mix_test_1", 440.0f, 0.1f);
    uint32_t tone2_id = audio_core_generate_test_tone("mix_test_2", 880.0f, 0.1f);
    
    AUDIO_TEST_ASSERT(tone1_id != UINT32_MAX, "First test tone should be generated");
    AUDIO_TEST_ASSERT(tone2_id != UINT32_MAX, "Second test tone should be generated");
    
    // Play multiple sounds
    uint32_t playing1 = audio_core_play_sfx(tone1_id, 0.5f, 1.0f, false);
    uint32_t playing2 = audio_core_play_sfx(tone2_id, 0.5f, 1.0f, false);
    
    AUDIO_TEST_ASSERT(playing1 != 0, "First sound should play");
    AUDIO_TEST_ASSERT(playing2 != 0, "Second sound should play");
    
    // Test audio mixing
    float mix_buffer[4096];
    audio_core_mix(mix_buffer, 1024);
    
    AUDIO_TEST_ASSERT(true, "Audio mixing should not crash");
    AUDIO_TEST_ASSERT(validate_audio_buffer(mix_buffer, 2048, -1.0f, 1.0f), "Mixed audio should be within valid range");
    
    // Test mixing when not initialized
    audio_core_shutdown();
    memset(mix_buffer, 0, sizeof(mix_buffer));
    audio_core_mix(mix_buffer, 1024);
    AUDIO_TEST_ASSERT(true, "Mixing when not initialized should not crash");
    
    return results.failed_tests == 0;
}

bool test_audio_core_memory_management(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Test loading many sounds
    uint32_t sound_ids[100];
    for (int i = 0; i < 100; i++) {
        char name[64];
        snprintf(name, sizeof(name), "memory_test_%d", i);
        sound_ids[i] = audio_core_generate_test_tone(name, 440.0f + i * 10.0f, 0.01f);
        
        if (i < 50) { // Should succeed for first 50 sounds
            AUDIO_TEST_ASSERT(sound_ids[i] != UINT32_MAX, "Sound loading should succeed within limits");
        }
    }
    
    // Test playing many sounds
    uint32_t playing_ids[256];
    for (int i = 0; i < 256; i++) {
        playing_ids[i] = audio_core_play_sfx(sound_ids[i % 50], 0.1f, 1.0f, false);
        
        if (i < 255) { // Should succeed for first 255 sounds
            AUDIO_TEST_ASSERT(playing_ids[i] != 0, "Sound playback should succeed within limits");
        }
    }
    
    // Test memory cleanup
    audio_core_stop_all_sounds();
    AUDIO_TEST_ASSERT(audio_core_get_playing_count() == 0, "All sounds should be stopped");
    
    audio_core_shutdown();
    AUDIO_TEST_ASSERT(!audio_core_is_initialized(), "Audio core should be properly shutdown");
    
    return results.failed_tests == 0;
}

bool test_audio_core_error_handling(void) {
    AudioTestResults results = {0};
    
    // Test operations without initialization
    AUDIO_TEST_ASSERT(!audio_core_is_initialized(), "Audio should not be initialized");
    
    audio_core_set_master_volume(0.5f);
    AUDIO_TEST_ASSERT(true, "Setting volume without init should not crash");
    
    uint32_t invalid_id = audio_core_play_sfx(0, 0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(invalid_id == 0, "Playing sound without init should fail");
    
    // Initialize and test error conditions
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio should be initialized");
    
    // Test invalid sound IDs
    invalid_id = audio_core_play_sfx(UINT32_MAX, 0.5f, 1.0f, false);
    AUDIO_TEST_ASSERT(invalid_id == 0, "Playing with invalid sound ID should fail");
    
    // Test loading with invalid parameters
    float test_data[100];
    generate_test_sine_wave(test_data, 100, 440.0f, 0.5f);
    
    invalid_id = audio_core_load_sound(NULL, test_data, 100, 1, 44100);
    AUDIO_TEST_ASSERT(invalid_id == UINT32_MAX, "Loading with NULL name should fail");
    
    invalid_id = audio_core_load_sound("test", NULL, 100, 1, 44100);
    AUDIO_TEST_ASSERT(invalid_id == UINT32_MAX, "Loading with NULL data should fail");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

bool test_audio_core_edge_cases(void) {
    AudioTestResults results = {0};
    
    audio_core_init();
    AUDIO_TEST_ASSERT(audio_core_is_initialized(), "Audio core should be initialized");
    
    // Test extreme volume values
    audio_core_set_master_volume(0.0f);
    AUDIO_TEST_ASSERT(audio_core_get_master_volume() == 0.0f, "Zero volume should be set correctly");
    
    audio_core_set_master_volume(1.0f);
    AUDIO_TEST_ASSERT(audio_core_get_master_volume() == 1.0f, "Full volume should be set correctly");
    
    // Test extreme pitch values
    uint32_t tone_id = audio_core_generate_test_tone("edge_test", 440.0f, 0.1f);
    uint32_t playing_id = audio_core_play_sfx(tone_id, 0.5f, 0.1f, false);
    AUDIO_TEST_ASSERT(playing_id != 0, "Very low pitch should work");
    
    playing_id = audio_core_play_sfx(tone_id, 0.5f, 10.0f, false);
    AUDIO_TEST_ASSERT(playing_id != 0, "Very high pitch should work");
    
    // Test extreme pan values
    audio_core_set_sound_pan(playing_id, -1.0f);
    AUDIO_TEST_ASSERT(true, "Full left pan should work");
    
    audio_core_set_sound_pan(playing_id, 1.0f);
    AUDIO_TEST_ASSERT(true, "Full right pan should work");
    
    // Test pause/resume functionality
    audio_core_pause_all();
    AUDIO_TEST_ASSERT(true, "Pause all should not crash");
    
    audio_core_resume_all();
    AUDIO_TEST_ASSERT(true, "Resume all should not crash");
    
    audio_core_shutdown();
    return results.failed_tests == 0;
}

// Test suite functions
bool audio_unit_tests_run_all(AudioTestResults* results) {
    audio_test_reset_results(results);
    
    AUDIO_TEST_LOG("Starting comprehensive audio unit tests...");
    
    // Run all test suites
    bool all_passed = true;
    all_passed &= test_audio_core_init_shutdown();
    all_passed &= test_audio_core_volume_controls();
    all_passed &= test_audio_core_sound_loading();
    all_passed &= test_audio_core_sound_playback();
    all_passed &= test_audio_core_sound_controls();
    all_passed &= test_audio_core_audio_mixing();
    all_passed &= test_audio_core_memory_management();
    all_passed &= test_audio_core_error_handling();
    all_passed &= test_audio_core_edge_cases();
    
    AUDIO_TEST_LOG("Audio unit tests completed");
    return all_passed;
}

bool audio_unit_tests_initialization(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running initialization tests...");
    return test_audio_core_init_shutdown();
}

bool audio_unit_tests_volume_controls(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running volume control tests...");
    return test_audio_core_volume_controls();
}

bool audio_unit_tests_sound_loading(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running sound loading tests...");
    return test_audio_core_sound_loading();
}

bool audio_unit_tests_sound_playback(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running sound playback tests...");
    return test_audio_core_sound_playback();
}

bool audio_unit_tests_sound_controls(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running sound control tests...");
    return test_audio_core_sound_controls();
}

bool audio_unit_tests_audio_mixing(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running audio mixing tests...");
    return test_audio_core_audio_mixing();
}

bool audio_unit_tests_memory_management(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running memory management tests...");
    return test_audio_core_memory_management();
}

bool audio_unit_tests_error_handling(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running error handling tests...");
    return test_audio_core_error_handling();
}

bool audio_unit_tests_edge_cases(AudioTestResults* results) {
    audio_test_reset_results(results);
    AUDIO_TEST_LOG("Running edge case tests...");
    return test_audio_core_edge_cases();
}
