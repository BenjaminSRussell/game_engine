#include "engine_coverage_test_framework.h"
#include <math.h>
#include <pthread.h>

// Mock audio system structures and functions
typedef struct {
    float *buffer;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t buffer_size;
    bool is_initialized;
} AudioSystem;

typedef struct {
    float volume;
    float pan;
    bool is_playing;
    uint32_t position;
    float *audio_data;
    uint32_t audio_length;
} AudioSource;

static AudioSystem g_audio_system = {0};

// Mock audio functions
bool audio_system_init(uint32_t sample_rate, uint32_t channels, uint32_t buffer_size) {
    if (g_audio_system.is_initialized) {
        return false; // Already initialized
    }
    
    g_audio_system.buffer = MALLOC(sizeof(float) * buffer_size * channels);
    if (!g_audio_system.buffer) {
        return false;
    }
    
    g_audio_system.sample_rate = sample_rate;
    g_audio_system.channels = channels;
    g_audio_system.buffer_size = buffer_size;
    g_audio_system.is_initialized = true;
    
    return true;
}

void audio_system_shutdown(void) {
    if (g_audio_system.buffer) {
        FREE(g_audio_system.buffer);
        g_audio_system.buffer = NULL;
    }
    g_audio_system.is_initialized = false;
}

AudioSource* audio_source_create(const float *audio_data, uint32_t audio_length) {
    if (!g_audio_system.is_initialized) {
        return NULL;
    }
    
    AudioSource *source = MALLOC(sizeof(AudioSource));
    if (!source) {
        return NULL;
    }
    
    source->audio_data = MALLOC(sizeof(float) * audio_length);
    if (!source->audio_data) {
        FREE(source);
        return NULL;
    }
    
    memcpy(source->audio_data, audio_data, sizeof(float) * audio_length);
    source->audio_length = audio_length;
    source->volume = 1.0f;
    source->pan = 0.0f;
    source->is_playing = false;
    source->position = 0;
    
    return source;
}

void audio_source_destroy(AudioSource *source) {
    if (source) {
        if (source->audio_data) {
            FREE(source->audio_data);
        }
        FREE(source);
    }
}

bool audio_source_play(AudioSource *source) {
    if (!source || !g_audio_system.is_initialized) {
        return false;
    }
    
    source->is_playing = true;
    source->position = 0;
    return true;
}

bool audio_source_stop(AudioSource *source) {
    if (!source) {
        return false;
    }
    
    source->is_playing = false;
    source->position = 0;
    return true;
}

bool audio_source_set_volume(AudioSource *source, float volume) {
    if (!source || volume < 0.0f || volume > 2.0f) {
        return false;
    }
    
    source->volume = volume;
    return true;
}

bool audio_source_set_pan(AudioSource *source, float pan) {
    if (!source || pan < -1.0f || pan > 1.0f) {
        return false;
    }
    
    source->pan = pan;
    return true;
}

uint32_t audio_mix_buffer(float *output_buffer, uint32_t frames) {
    if (!g_audio_system.is_initialized || !output_buffer) {
        return 0;
    }
    
    // Clear output buffer
    memset(output_buffer, 0, sizeof(float) * frames * g_audio_system.channels);
    
    // This would mix all active audio sources
    // For testing purposes, we'll just return the frame count
    return frames;
}

// Audio system tests
static bool test_audio_system_initialization(void) {
    // Test initialization with valid parameters
    bool result = audio_system_init(44100, 2, 1024);
    ASSERT_TRUE(result);
    ASSERT_TRUE(g_audio_system.is_initialized);
    ASSERT_EQ(44100, g_audio_system.sample_rate);
    ASSERT_EQ(2, g_audio_system.channels);
    ASSERT_EQ(1024, g_audio_system.buffer_size);
    ASSERT_NOT_NULL(g_audio_system.buffer);
    
    // Test double initialization
    result = audio_system_init(48000, 2, 512);
    ASSERT_FALSE(result);
    
    audio_system_shutdown();
    return true;
}

static bool test_audio_system_shutdown(void) {
    // Initialize first
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    // Test shutdown
    audio_system_shutdown();
    ASSERT_FALSE(g_audio_system.is_initialized);
    ASSERT_NULL(g_audio_system.buffer);
    
    // Test shutdown when not initialized (should be safe)
    audio_system_shutdown();
    
    return true;
}

static bool test_audio_source_creation(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    // Create test audio data
    float test_data[1024];
    for (int i = 0; i < 1024; i++) {
        test_data[i] = sinf(2.0f * M_PI * 440.0f * i / 44100.0f); // 440Hz sine wave
    }
    
    // Test source creation
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(source->audio_data);
    ASSERT_EQ(1024, source->audio_length);
    ASSERT_FLOAT_EQ(1.0f, source->volume, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, source->pan, 0.0001f);
    ASSERT_FALSE(source->is_playing);
    ASSERT_EQ(0, source->position);
    
    // Verify audio data was copied correctly
    for (int i = 0; i < 1024; i++) {
        ASSERT_FLOAT_EQ(test_data[i], source->audio_data[i]);
    }
    
    audio_source_destroy(source);
    audio_system_shutdown();
    return true;
}

static bool test_audio_source_creation_without_system(void) {
    // Test source creation when audio system is not initialized
    float test_data[1024];
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NULL(source);
    
    return true;
}

static bool test_audio_source_playback(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    float test_data[1024];
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NOT_NULL(source);
    
    // Test play
    ASSERT_TRUE(audio_source_play(source));
    ASSERT_TRUE(source->is_playing);
    ASSERT_EQ(0, source->position);
    
    // Test play when already playing
    ASSERT_TRUE(audio_source_play(source));
    ASSERT_TRUE(source->is_playing);
    ASSERT_EQ(0, source->position); // Should reset position
    
    audio_source_destroy(source);
    audio_system_shutdown();
    return true;
}

static bool test_audio_source_stop(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    float test_data[1024];
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NOT_NULL(source);
    
    // Test stop when not playing
    ASSERT_TRUE(audio_source_stop(source));
    ASSERT_FALSE(source->is_playing);
    ASSERT_EQ(0, source->position);
    
    // Test stop when playing
    ASSERT_TRUE(audio_source_play(source));
    ASSERT_TRUE(source->is_playing);
    
    ASSERT_TRUE(audio_source_stop(source));
    ASSERT_FALSE(source->is_playing);
    ASSERT_EQ(0, source->position);
    
    audio_source_destroy(source);
    audio_system_shutdown();
    return true;
}

static bool test_audio_source_volume_control(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    float test_data[1024];
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NOT_NULL(source);
    
    // Test valid volume settings
    ASSERT_TRUE(audio_source_set_volume(source, 0.0f));
    ASSERT_FLOAT_EQ(0.0f, source->volume, 0.0001f);
    
    ASSERT_TRUE(audio_source_set_volume(source, 0.5f));
    ASSERT_FLOAT_EQ(0.5f, source->volume, 0.0001f);
    
    ASSERT_TRUE(audio_source_set_volume(source, 1.0f));
    ASSERT_FLOAT_EQ(1.0f, source->volume, 0.0001f);
    
    ASSERT_TRUE(audio_source_set_volume(source, 2.0f));
    ASSERT_FLOAT_EQ(2.0f, source->volume, 0.0001f);
    
    // Test invalid volume settings
    ASSERT_FALSE(audio_source_set_volume(source, -0.1f));
    ASSERT_FLOAT_EQ(2.0f, source->volume, 0.0001f); // Should remain unchanged
    
    ASSERT_FALSE(audio_source_set_volume(source, 2.1f));
    ASSERT_FLOAT_EQ(2.0f, source->volume, 0.0001f); // Should remain unchanged
    
    audio_source_destroy(source);
    audio_system_shutdown();
    return true;
}

static bool test_audio_source_pan_control(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    float test_data[1024];
    AudioSource *source = audio_source_create(test_data, 1024);
    ASSERT_NOT_NULL(source);
    
    // Test valid pan settings
    ASSERT_TRUE(audio_source_set_pan(source, -1.0f));
    ASSERT_FLOAT_EQ(-1.0f, source->pan, 0.0001f);
    
    ASSERT_TRUE(audio_source_set_pan(source, 0.0f));
    ASSERT_FLOAT_EQ(0.0f, source->pan, 0.0001f);
    
    ASSERT_TRUE(audio_source_set_pan(source, 1.0f));
    ASSERT_FLOAT_EQ(1.0f, source->pan, 0.0001f);
    
    // Test invalid pan settings
    ASSERT_FALSE(audio_source_set_pan(source, -1.1f));
    ASSERT_FLOAT_EQ(1.0f, source->pan, 0.0001f); // Should remain unchanged
    
    ASSERT_FALSE(audio_source_set_pan(source, 1.1f));
    ASSERT_FLOAT_EQ(1.0f, source->pan, 0.0001f); // Should remain unchanged
    
    audio_source_destroy(source);
    audio_system_shutdown();
    return true;
}

static bool test_audio_mixing(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    float output_buffer[1024 * 2]; // Stereo buffer
    
    // Test mixing with valid parameters
    uint32_t frames_mixed = audio_mix_buffer(output_buffer, 1024);
    ASSERT_EQ(1024, frames_mixed);
    
    // Verify buffer is cleared
    for (int i = 0; i < 1024 * 2; i++) {
        ASSERT_FLOAT_EQ(0.0f, output_buffer[i]);
    }
    
    // Test mixing with NULL buffer
    frames_mixed = audio_mix_buffer(NULL, 1024);
    ASSERT_EQ(0, frames_mixed);
    
    audio_system_shutdown();
    return true;
}

// Thread safety test
static void* audio_thread_function(void *arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < 100; i++) {
        float test_data[256];
        AudioSource *source = audio_source_create(test_data, 256);
        
        if (source) {
            audio_source_set_volume(source, (float)thread_id / 10.0f);
            audio_source_set_pan(source, (float)(thread_id % 3 - 1));
            audio_source_play(source);
            audio_source_stop(source);
            audio_source_destroy(source);
        }
        
        usleep(100); // Small delay
    }
    
    return NULL;
}

static bool test_audio_thread_safety(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    pthread_t threads[5];
    int thread_ids[5];
    
    // Create multiple threads
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i;
        int result = pthread_create(&threads[i], NULL, audio_thread_function, &thread_ids[i]);
        ASSERT_EQ(0, result);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    audio_system_shutdown();
    ASSERT_NO_MEMORY_LEAKS();
    return true;
}

// Performance test
static bool test_audio_performance(void) {
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    
    const int num_sources = 1000;
    AudioSource *sources[num_sources];
    float test_data[1024];
    
    uint64_t start_time, end_time, total_time;
    
    // Measure source creation time
    MEASURE_PERFORMANCE_START();
    for (int i = 0; i < num_sources; i++) {
        sources[i] = audio_source_create(test_data, 1024);
        ASSERT_NOT_NULL(sources[i]);
    }
    MEASURE_PERFORMANCE_END(&total_time);
    
    double creation_time_ms = (double)total_time / 1000000.0;
    log_test_message("INFO", "AUDIO", "Created %d sources in %.2f ms (%.2f μs per source)", 
                    num_sources, creation_time_ms, (double)total_time / 1000.0 / num_sources);
    
    // Measure volume setting time
    MEASURE_PERFORMANCE_START();
    for (int i = 0; i < num_sources; i++) {
        audio_source_set_volume(sources[i], (float)i / num_sources);
    }
    MEASURE_PERFORMANCE_END(&total_time);
    
    double volume_time_ms = (double)total_time / 1000000.0;
    log_test_message("INFO", "AUDIO", "Set volume for %d sources in %.2f ms", 
                    num_sources, volume_time_ms);
    
    // Cleanup
    for (int i = 0; i < num_sources; i++) {
        audio_source_destroy(sources[i]);
    }
    
    audio_system_shutdown();
    return true;
}

// Edge case tests
static bool test_audio_edge_cases(void) {
    // Test initialization with extreme values
    ASSERT_TRUE(audio_system_init(8000, 1, 64)); // Minimum values
    audio_system_shutdown();
    
    ASSERT_TRUE(audio_system_init(192000, 8, 8192)); // High values
    audio_system_shutdown();
    
    // Test source creation with zero-length data
    ASSERT_TRUE(audio_system_init(44100, 2, 1024));
    AudioSource *source = audio_source_create(NULL, 0);
    ASSERT_NOT_NULL(source); // Should handle gracefully
    audio_source_destroy(source);
    audio_system_shutdown();
    
    return true;
}

// Main audio test suite
TestSuite engine_run_audio_tests(void) {
    TestSuite suite = {0};
    strcpy(suite.suite_name, "Audio Tests");
    
    const int test_count = 11;
    suite.tests = MALLOC(sizeof(TestResult) * test_count);
    suite.test_count = test_count;
    
    TestCase tests[] = {
        {"System Initialization", test_audio_system_initialization, "Test audio system initialization"},
        {"System Shutdown", test_audio_system_shutdown, "Test audio system shutdown"},
        {"Source Creation", test_audio_source_creation, "Test audio source creation"},
        {"Source Creation Without System", test_audio_source_creation_without_system, "Test source creation without initialized system"},
        {"Source Playback", test_audio_source_playback, "Test audio source playback control"},
        {"Source Stop", test_audio_source_stop, "Test audio source stop functionality"},
        {"Volume Control", test_audio_source_volume_control, "Test audio source volume control"},
        {"Pan Control", test_audio_source_pan_control, "Test audio source pan control"},
        {"Audio Mixing", test_audio_mixing, "Test audio mixing functionality"},
        {"Thread Safety", test_audio_thread_safety, "Test thread safety of audio operations"},
        {"Performance", test_audio_performance, "Test audio system performance"}
    };
    
    suite.total_execution_time_ms = get_time_ms();
    
    for (int i = 0; i < test_count; i++) {
        TestResult *result = &suite.tests[i];
        strcpy(result->test_name, tests[i].name);
        strcpy(result->suite_name, suite.suite_name);
        result->assertions_run = 0;
        result->assertions_passed = 0;
        result->error_message[0] = '\0';
        
        g_current_test = result;
        
        double test_start = get_time_ms();
        result->passed = tests[i].function();
        result->execution_time_ms = get_time_ms() - test_start;
        
        if (result->passed) {
            suite.tests_passed++;
            log_test_message("PASS", suite.suite_name, "%s (%.2f ms, %u assertions)", 
                           result->test_name, result->execution_time_ms, result->assertions_passed);
        } else {
            suite.tests_failed++;
            log_test_message("FAIL", suite.suite_name, "%s (%.2f ms): %s", 
                           result->test_name, result->execution_time_ms, result->error_message);
        }
    }
    
    suite.total_execution_time_ms = get_time_ms() - suite.total_execution_time_ms;
    suite.tests_skipped = 0;
    
    g_current_test = NULL;
    
    return suite;
}
