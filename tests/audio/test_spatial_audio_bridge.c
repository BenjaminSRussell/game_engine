/**
 * =================================================================================================
 *                          SPATIAL AUDIO BRIDGE API TESTS
 * =================================================================================================
 */

#include "../spatial_audio_bridge.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
} while(0)

#define ASSERT_TRUE(expr) assert(expr)
#define ASSERT_FALSE(expr) assert(!(expr))
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_FLOAT_EQ(a, b) assert(fabsf((a) - (b)) < 0.0001f)
#define ASSERT_NOT_NULL(ptr) assert((ptr) != NULL)

// Test: Context creation and destruction
TEST(context_lifecycle) {
    SpatialAudioContext* ctx = spatial_audio_create();
    ASSERT_NOT_NULL(ctx);
    
    spatial_audio_destroy(ctx);
    // Should not crash
}

// Test: Format control
TEST(format_control) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    // Set to DTS:X
    spatial_audio_set_format(ctx, 1);  // AUDIO_FORMAT_DTSX
    int32_t format = spatial_audio_get_format(ctx);
    ASSERT_EQ(format, 1);
    
    // Set to Custom Surround
    spatial_audio_set_format(ctx, 2);  // AUDIO_FORMAT_CUSTOM_SURROUND
    format = spatial_audio_get_format(ctx);
    ASSERT_EQ(format, 2);
    
    spatial_audio_destroy(ctx);
}

// Test: Master volume control
TEST(master_volume) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    spatial_audio_set_master_volume(ctx, 0.5f);
    float volume = spatial_audio_get_master_volume(ctx);
    ASSERT_FLOAT_EQ(volume, 0.5f);
    
    spatial_audio_set_master_volume(ctx, 1.0f);
    volume = spatial_audio_get_master_volume(ctx);
    ASSERT_FLOAT_EQ(volume, 1.0f);
    
    spatial_audio_destroy(ctx);
}

// Test: Speaker gain control
TEST(speaker_gain) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    spatial_audio_set_speaker_gain(ctx, "front_left", 0.8f);
    float gain = spatial_audio_get_speaker_gain(ctx, "front_left");
    ASSERT_FLOAT_EQ(gain, 0.8f);
    
    spatial_audio_set_speaker_gain(ctx, "center", 1.2f);
    gain = spatial_audio_get_speaker_gain(ctx, "center");
    ASSERT_FLOAT_EQ(gain, 1.2f);
    
    spatial_audio_destroy(ctx);
}

// Test: Speaker delay configuration
TEST(speaker_delay) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    // Set delay for front left
    spatial_audio_set_speaker_delay(ctx, "front_left", 5.0f);
    
    // No getter for delay, but should not crash
    
    spatial_audio_destroy(ctx);
}

// Test: Speaker position
TEST(speaker_position) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    spatial_audio_set_speaker_position(ctx, "front_left", -1.0f, 0.0f, 2.0f);
    spatial_audio_set_speaker_position(ctx, "front_right", 1.0f, 0.0f, 2.0f);
    
    // Should configure speakers without crashing
    
    spatial_audio_destroy(ctx);
}

// Test: Audio object creation
TEST(object_creation) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    int32_t obj1 = spatial_audio_create_object(ctx, 0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(obj1 >= 0);
    
    int32_t obj2 = spatial_audio_create_object(ctx, 1.0f, 0.5f, 2.0f);
    ASSERT_TRUE(obj2 >= 0);
    ASSERT_TRUE(obj2 != obj1);
    
    spatial_audio_destroy(ctx);
}

// Test: Object position update
TEST(object_position_update) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    int32_t obj = spatial_audio_create_object(ctx, 0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(obj >= 0);
    
    // Update position
    spatial_audio_set_object_position(ctx, obj, 2.0f, 1.0f, 3.0f);
    
    // Should update without crashing
    
    spatial_audio_destroy(ctx);
}

// Test: Object destruction
TEST(object_destruction) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    int32_t obj = spatial_audio_create_object(ctx, 0.0f, 0.0f, 1.0f);
    ASSERT_TRUE(obj >= 0);
    
    spatial_audio_destroy_object(ctx, obj);
    
    // Should remove without crashing
    
    spatial_audio_destroy(ctx);
}

// Test: Speaker level monitoring
TEST(level_monitoring) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    float level = spatial_audio_get_speaker_level(ctx, "front_left");
    ASSERT_TRUE(level >= 0.0f);
    ASSERT_TRUE(level <= 1.0f);
    
    float peak = spatial_audio_get_master_peak(ctx);
    ASSERT_TRUE(peak >= 0.0f);
    ASSERT_TRUE(peak <= 1.0f);
    
    spatial_audio_destroy(ctx);
}

// Test: Test tone generation
TEST(test_tone) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    spatial_audio_play_test_tone(ctx);
    
    // Should enable test tone without crashing
    
    spatial_audio_destroy(ctx);
}

// Test: Audio processing
TEST(audio_processing) {
    SpatialAudioContext* ctx = spatial_audio_create();
    
    int frames = 512;
    float* input_buffers[2];
    float* output_buffers[12];  // Max speakers
    
    for (int i = 0; i < 2; i++) {
        input_buffers[i] = (float*)calloc(frames, sizeof(float));
    }
    for (int i = 0; i < 12; i++) {
        output_buffers[i] = (float*)calloc(frames, sizeof(float));
    }
    
    // Process a frame
    spatial_audio_process(ctx, input_buffers, 2, output_buffers, 12, frames);
    
    // Should process without crashing
    
    for (int i = 0; i < 2; i++) {
        free(input_buffers[i]);
    }
    for (int i = 0; i < 12; i++) {
        free(output_buffers[i]);
    }
    
    spatial_audio_destroy(ctx);
}

// Test: Null safety
TEST(null_safety) {
    // All functions should handle NULL gracefully
    spatial_audio_destroy(NULL);
    spatial_audio_set_format(NULL, 0);
    spatial_audio_set_master_volume(NULL, 1.0f);
    spatial_audio_set_speaker_gain(NULL, "front_left", 1.0f);
    
    float vol = spatial_audio_get_master_volume(NULL);
    ASSERT_FLOAT_EQ(vol, 0.0f);
    
    float level = spatial_audio_get_speaker_level(NULL, "front_left");
    ASSERT_FLOAT_EQ(level, 0.0f);
}

int main() {
    printf("Running Spatial Audio Bridge API Tests\n");
    printf("=======================================\n\n");
    
    RUN_TEST(context_lifecycle);
    RUN_TEST(format_control);
    RUN_TEST(master_volume);
    RUN_TEST(speaker_gain);
    RUN_TEST(speaker_delay);
    RUN_TEST(speaker_position);
    RUN_TEST(object_creation);
    RUN_TEST(object_position_update);
    RUN_TEST(object_destruction);
    RUN_TEST(level_monitoring);
    RUN_TEST(test_tone);
    RUN_TEST(audio_processing);
    RUN_TEST(null_safety);
    
    printf("\n=======================================\n");
    printf("All tests passed!\n");
    
    return 0;
}
