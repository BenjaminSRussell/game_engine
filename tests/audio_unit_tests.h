/**
 * AUDIO SYSTEM UNIT TESTS
 * Comprehensive testing for audio core functionality
 */

#ifndef AUDIO_UNIT_TESTS_H
#define AUDIO_UNIT_TESTS_H

#include <stdint.h>
#include <stdbool.h>

// Test result structure
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char last_error[256];
} AudioTestResults;

// Test suite functions
bool audio_unit_tests_run_all(AudioTestResults* results);
bool audio_unit_tests_initialization(AudioTestResults* results);
bool audio_unit_tests_volume_controls(AudioTestResults* results);
bool audio_unit_tests_sound_loading(AudioTestResults* results);
bool audio_unit_tests_sound_playback(AudioTestResults* results);
bool audio_unit_tests_sound_controls(AudioTestResults* results);
bool audio_unit_tests_audio_mixing(AudioTestResults* results);
bool audio_unit_tests_memory_management(AudioTestResults* results);
bool audio_unit_tests_error_handling(AudioTestResults* results);
bool audio_unit_tests_edge_cases(AudioTestResults* results);

// Individual test functions
bool test_audio_core_init_shutdown(void);
bool test_audio_core_volume_controls(void);
bool test_audio_core_sound_loading(void);
bool test_audio_core_sound_playback(void);
bool test_audio_core_sound_controls(void);
bool test_audio_core_audio_mixing(void);
bool test_audio_core_memory_management(void);
bool test_audio_core_error_handling(void);
bool test_audio_core_edge_cases(void);

// Helper functions
void audio_test_assert(bool condition, const char* message, AudioTestResults* results);
void audio_test_log(const char* message, AudioTestResults* results);
void audio_test_reset_results(AudioTestResults* results);

// Test data generators
void generate_test_sine_wave(float* buffer, uint32_t length, float frequency, float amplitude);
void generate_test_square_wave(float* buffer, uint32_t length, float frequency, float amplitude);
void generate_test_noise(float* buffer, uint32_t length, float amplitude);

// Audio validation functions
bool validate_audio_buffer(const float* buffer, uint32_t length, float min_val, float max_val);
bool compare_audio_buffers(const float* buffer1, const float* buffer2, uint32_t length, float tolerance);

#endif // AUDIO_UNIT_TESTS_H
