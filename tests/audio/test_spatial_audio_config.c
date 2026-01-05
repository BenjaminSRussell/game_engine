/**
 * =================================================================================================
 *                          SPATIAL AUDIO CONFIGURATION TESTS
 * =================================================================================================
 */

#include "../spatial_audio_config.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

// Simple test framework macros
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

// Test: Configuration initialization
TEST(config_init) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    ASSERT_EQ(config.format, AUDIO_FORMAT_DOLBY_ATMOS);
    ASSERT_FLOAT_EQ(config.master_volume, 0.75f);
    ASSERT_EQ(config.sample_rate, 48000);
    ASSERT_EQ(config.buffer_size, 512);
    
    // All speakers should be disabled initially
    for (int i = 0; i < SPEAKER_MAX; i++) {
        ASSERT_FALSE(config.speakers[i].enabled);
        ASSERT_FLOAT_EQ(config.speakers[i].gain, 1.0f);
    }
}

// Test: Format switching
TEST(format_switching) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    // Switch to DTS:X
    spatial_audio_config_set_format(&config, AUDIO_FORMAT_DTSX);
    ASSERT_EQ(config.format, AUDIO_FORMAT_DTSX);
    ASSERT_TRUE(config.dtsx_neural_x);
    
    // Switch to custom surround
    spatial_audio_config_set_format(&config, AUDIO_FORMAT_CUSTOM_SURROUND);
    ASSERT_EQ(config.format, AUDIO_FORMAT_CUSTOM_SURROUND);
    ASSERT_FLOAT_EQ(config.vbap_spread, 15.0f);
    ASSERT_TRUE(config.distance_attenuation);
}

// Test: 5.1 layout preset
TEST(preset_51) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    spatial_audio_load_preset_51(&config);
    
    ASSERT_EQ(config.active_speaker_count, 6);
    ASSERT_TRUE(config.speakers[SPEAKER_FRONT_LEFT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_FRONT_RIGHT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_FRONT_CENTER].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_LFE].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_SURROUND_LEFT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_SURROUND_RIGHT].enabled);
    
    // LFE should have higher gain
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_LFE].gain, 1.5f);
}

// Test: 7.1 layout preset
TEST(preset_71) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    spatial_audio_load_preset_71(&config);
    
    ASSERT_EQ(config.active_speaker_count, 8);
    ASSERT_TRUE(config.speakers[SPEAKER_BACK_LEFT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_BACK_RIGHT].enabled);
}

// Test: 7.1.4 Atmos layout preset
TEST(preset_714) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    spatial_audio_load_preset_714(&config);
    
    ASSERT_EQ(config.active_speaker_count, 12);
    ASSERT_TRUE(config.has_height_channels);
    
    // Check height speakers
    ASSERT_TRUE(config.speakers[SPEAKER_HEIGHT_FRONT_LEFT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_HEIGHT_FRONT_RIGHT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_HEIGHT_BACK_LEFT].enabled);
    ASSERT_TRUE(config.speakers[SPEAKER_HEIGHT_BACK_RIGHT].enabled);
    
    // Check height positions (should have positive Y)
    ASSERT_TRUE(config.speakers[SPEAKER_HEIGHT_FRONT_LEFT].position[1] > 0);
}

// Test: Speaker configuration
TEST(speaker_configuration) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    float position[3] = {1.0f, 0.5f, 2.0f};
    float gain = 0.8f;
    
    spatial_audio_configure_speaker(&config, SPEAKER_FRONT_LEFT, position, gain);
    
    ASSERT_TRUE(config.speakers[SPEAKER_FRONT_LEFT].enabled);
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_FRONT_LEFT].position[0], 1.0f);
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_FRONT_LEFT].position[1], 0.5f);
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_FRONT_LEFT].position[2], 2.0f);
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_FRONT_LEFT].gain, 0.8f);
}

// Test: Binaural preset
TEST(preset_binaural) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    spatial_audio_load_preset_binaural(&config);
    
    ASSERT_TRUE(config.atmos_binaural_mode);
    ASSERT_EQ(config.active_speaker_count, 2);
}

// Test: Speaker positions are normalized
TEST(speaker_positions_normalized) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    spatial_audio_load_preset_51(&config);
    
    // Check that front left/right are symmetrical
    ASSERT_FLOAT_EQ(
        config.speakers[SPEAKER_FRONT_LEFT].position[0],
        -config.speakers[SPEAKER_FRONT_RIGHT].position[0]
    );
    
    // Check that center is at origin X
    ASSERT_FLOAT_EQ(config.speakers[SPEAKER_FRONT_CENTER].position[0], 0.0f);
}

int main() {
    printf("Running Spatial Audio Configuration Tests\n");
    printf("==========================================\n\n");
    
    RUN_TEST(config_init);
    RUN_TEST(format_switching);
    RUN_TEST(preset_51);
    RUN_TEST(preset_71);
    RUN_TEST(preset_714);
    RUN_TEST(speaker_configuration);
    RUN_TEST(preset_binaural);
    RUN_TEST(speaker_positions_normalized);
    
    printf("\n==========================================\n");
    printf("All tests passed!\n");
    
    return 0;
}
