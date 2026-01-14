/**
 * AUDIO TEST RUNNER
 * Comprehensive test execution and reporting for audio system
 */

#include "audio_unit_tests.h"
#include "audio_integration_tests.h"
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    char test_name[128];
    char category[64];
    bool passed;
    double execution_time;
    char error_message[256];
} TestResult;

typedef struct {
    TestResult* results;
    int total_tests;
    int passed_tests;
    int failed_tests;
    double total_time;
    time_t start_time;
    time_t end_time;
} TestReport;

static TestReport g_test_report = {0};

void test_report_init(void) {
    memset(&g_test_report, 0, sizeof(TestReport));
    g_test_report.results = malloc(1000 * sizeof(TestResult));
    g_test_report.start_time = time(NULL);
}

void test_report_cleanup(void) {
    if (g_test_report.results) {
        free(g_test_report.results);
        g_test_report.results = NULL;
    }
    memset(&g_test_report, 0, sizeof(TestReport));
}

void test_report_add_result(const char* test_name, const char* category, 
                           bool passed, double execution_time, const char* error_message) {
    if (g_test_report.total_tests < 1000) {
        TestResult* result = &g_test_report.results[g_test_report.total_tests];
        strncpy(result->test_name, test_name, sizeof(result->test_name) - 1);
        strncpy(result->category, category, sizeof(result->category) - 1);
        result->passed = passed;
        result->execution_time = execution_time;
        if (error_message) {
            strncpy(result->error_message, error_message, sizeof(result->error_message) - 1);
        }
        
        g_test_report.total_tests++;
        if (passed) {
            g_test_report.passed_tests++;
        } else {
            g_test_report.failed_tests++;
        }
    }
}

void test_report_print_summary(void) {
    g_test_report.end_time = time(NULL);
    g_test_report.total_time = difftime(g_test_report.end_time, g_test_report.start_time);
    
    printf("\n" "=" * 80 "\n");
    printf("AUDIO SYSTEM TEST REPORT\n");
    printf("=" * 80 "\n");
    printf("Total Tests:     %d\n", g_test_report.total_tests);
    printf("Passed:          %d\n", g_test_report.passed_tests);
    printf("Failed:          %d\n", g_test_report.failed_tests);
    printf("Success Rate:    %.1f%%\n", 
           g_test_report.total_tests > 0 ? (100.0 * g_test_report.passed_tests / g_test_report.total_tests) : 0.0);
    printf("Total Time:      %.2f seconds\n", g_test_report.total_time);
    printf("Test Date:       %s", ctime(&g_test_report.start_time));
    printf("=" * 80 "\n");
    
    // Print failed tests
    if (g_test_report.failed_tests > 0) {
        printf("\nFAILED TESTS:\n");
        printf("-" * 80 "\n");
        for (int i = 0; i < g_test_report.total_tests; i++) {
            TestResult* result = &g_test_report.results[i];
            if (!result->passed) {
                printf("[%s] %s\n", result->category, result->test_name);
                printf("  Error: %s\n", result->error_message);
                printf("  Time:  %.3f seconds\n", result->execution_time);
                printf("\n");
            }
        }
    }
    
    // Print test categories
    printf("\nTEST CATEGORIES:\n");
    printf("-" * 80 "\n");
    char categories[10][64] = {0};
    int category_counts[10] = {0};
    int category_passed[10] = {0};
    int category_count = 0;
    
    for (int i = 0; i < g_test_report.total_tests; i++) {
        TestResult* result = &g_test_report.results[i];
        
        // Find category
        int cat_index = -1;
        for (int j = 0; j < category_count; j++) {
            if (strcmp(categories[j], result->category) == 0) {
                cat_index = j;
                break;
            }
        }
        
        if (cat_index == -1 && category_count < 10) {
            strncpy(categories[category_count], result->category, sizeof(categories[category_count]) - 1);
            cat_index = category_count++;
        }
        
        if (cat_index >= 0) {
            category_counts[cat_index]++;
            if (result->passed) {
                category_passed[cat_index]++;
            }
        }
    }
    
    for (int i = 0; i < category_count; i++) {
        printf("%-20s: %3d/%3d tests passed (%.1f%%)\n", 
               categories[i], category_passed[i], category_counts[i],
               100.0 * category_passed[i] / category_counts[i]);
    }
}

bool run_audio_unit_tests(void) {
    printf("\nRunning Audio Unit Tests...\n");
    printf("-" * 50 "\n");
    
    AudioTestResults unit_results = {0};
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    bool success = audio_unit_tests_run_all(&unit_results);
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    
    test_report_add_result("Audio Unit Tests", "Unit", success, end_time - start_time, 
                          unit_results.failed_tests > 0 ? unit_results.last_error : NULL);
    
    printf("Unit Tests: %d total, %d passed, %d failed\n", 
           unit_results.total_tests, unit_results.passed_tests, unit_results.failed_tests);
    
    return success;
}

bool run_audio_integration_tests(void) {
    printf("\nRunning Audio Integration Tests...\n");
    printf("-" * 50 "\n");
    
    AudioIntegrationResults integration_results = {0};
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    bool success = audio_integration_tests_run_all(&integration_results);
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    
    test_report_add_result("Audio Integration Tests", "Integration", success, 
                          end_time - start_time,
                          integration_results.failed_tests > 0 ? integration_results.last_error : NULL);
    
    printf("Integration Tests: %d total, %d passed, %d failed\n", 
           integration_results.total_tests, integration_results.passed_tests, integration_results.failed_tests);
    printf("Integration Time: %.2f seconds\n", integration_results.total_test_time);
    
    return success;
}

bool run_audio_performance_tests(void) {
    printf("\nRunning Audio Performance Tests...\n");
    printf("-" * 50 "\n");
    
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Initialize audio system
    audio_core_init();
    
    // Performance test 1: Sound loading
    double load_start = (double)clock() / CLOCKS_PER_SEC;
    uint32_t sound_ids[100];
    for (int i = 0; i < 100; i++) {
        char name[64];
        snprintf(name, sizeof(name), "perf_test_%d", i);
        sound_ids[i] = audio_core_generate_test_tone(name, 440.0f + i * 10.0f, 0.01f);
    }
    double load_time = (double)clock() / CLOCKS_PER_SEC - load_start;
    
    bool load_success = (load_time < 2.0);
    test_report_add_result("Sound Loading Performance", "Performance", load_success, load_time,
                          load_success ? NULL : "Sound loading took too long");
    
    // Performance test 2: Simultaneous playback
    double playback_start = (double)clock() / CLOCKS_PER_SEC;
    uint32_t playing_ids[50];
    for (int i = 0; i < 50; i++) {
        playing_ids[i] = audio_core_play_sfx(sound_ids[i], 0.1f, 1.0f, false);
    }
    double playback_time = (double)clock() / CLOCKS_PER_SEC - playback_start;
    
    bool playback_success = (playback_time < 0.1);
    test_report_add_result("Simultaneous Playback Performance", "Performance", playback_success, 
                          playback_time,
                          playback_success ? NULL : "Simultaneous playback took too long");
    
    // Performance test 3: Audio mixing
    float mix_buffer[4096];
    double mix_start = (double)clock() / CLOCKS_PER_SEC;
    for (int i = 0; i < 1000; i++) {
        audio_core_mix(mix_buffer, 1024);
    }
    double mix_time = (double)clock() / CLOCKS_PER_SEC - mix_start;
    
    bool mix_success = (mix_time < 1.0);
    test_report_add_result("Audio Mixing Performance", "Performance", mix_success, mix_time,
                          mix_success ? NULL : "Audio mixing took too long");
    
    audio_core_shutdown();
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    bool overall_success = load_success && playback_success && mix_success;
    
    test_report_add_result("Performance Tests", "Performance", overall_success, end_time - start_time,
                          overall_success ? NULL : "Some performance tests failed");
    
    printf("Performance Tests: Load=%.3fs, Playback=%.3fs, Mix=%.3fs\n", 
           load_time, playback_time, mix_time);
    
    return overall_success;
}

bool run_audio_stress_tests(void) {
    printf("\nRunning Audio Stress Tests...\n");
    printf("-" * 50 "\n");
    
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    audio_core_init();
    
    // Stress test 1: Maximum sound loading
    uint32_t max_sounds = 0;
    bool load_success = true;
    for (int i = 0; i < 1000; i++) {
        char name[64];
        snprintf(name, sizeof(name), "stress_test_%d", i);
        uint32_t id = audio_core_generate_test_tone(name, 440.0f + i, 0.01f);
        if (id == UINT32_MAX) {
            break;
        }
        max_sounds++;
    }
    
    test_report_add_result("Maximum Sound Loading", "Stress", max_sounds > 200, 0.0,
                          max_sounds > 200 ? NULL : "Could not load sufficient sounds");
    
    // Stress test 2: Maximum simultaneous playback
    uint32_t max_playing = 0;
    for (int i = 0; i < max_sounds; i++) {
        uint32_t playing_id = audio_core_play_sfx(i, 0.1f, 1.0f, false);
        if (playing_id == 0) {
            break;
        }
        max_playing++;
    }
    
    test_report_add_result("Maximum Simultaneous Playback", "Stress", max_playing > 100, 0.0,
                          max_playing > 100 ? NULL : "Could not play sufficient sounds simultaneously");
    
    // Stress test 3: Memory usage
    size_t memory_usage = audio_core_get_memory_usage();
    bool memory_ok = (memory_usage < 100 * 1024 * 1024); // Less than 100MB
    test_report_add_result("Memory Usage", "Stress", memory_ok, 0.0,
                          memory_ok ? NULL : "Memory usage too high");
    
    audio_core_shutdown();
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    bool overall_success = (max_sounds > 200) && (max_playing > 100) && memory_ok;
    
    test_report_add_result("Stress Tests", "Stress", overall_success, end_time - start_time,
                          overall_success ? NULL : "Some stress tests failed");
    
    printf("Stress Tests: Max Sounds=%d, Max Playing=%d, Memory=%zu KB\n", 
           max_sounds, max_playing, memory_usage / 1024);
    
    return overall_success;
}

int main(int argc, char* argv[]) {
    printf("AUDIO SYSTEM TEST SUITE\n");
    printf("=======================\n");
    
    test_report_init();
    
    bool all_passed = true;
    
    // Run all test suites
    all_passed &= run_audio_unit_tests();
    all_passed &= run_audio_integration_tests();
    all_passed &= run_audio_performance_tests();
    all_passed &= run_audio_stress_tests();
    
    // Print comprehensive report
    test_report_print_summary();
    
    test_report_cleanup();
    
    printf("\nTEST RESULT: %s\n", all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return all_passed ? 0 : 1;
}
