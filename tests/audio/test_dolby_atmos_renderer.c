/**
 * =================================================================================================
 *                          DOLBY ATMOS RENDERER TESTS
 * =================================================================================================
 */

#include "../spatial_audio_config.h"
#include "../dolby_atmos_renderer.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

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

// Test: Renderer initialization
TEST(renderer_init) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    ASSERT_NOT_NULL(renderer.objects);
    ASSERT_EQ(renderer.max_objects, 128);
    ASSERT_EQ(renderer.object_count, 0);
    
    // Check speaker buffers allocated
    for (int i = 0; i < SPEAKER_MAX; i++) {
        ASSERT_NOT_NULL(renderer.speaker_buffers[i]);
    }
    
    ASSERT_NOT_NULL(renderer.binaural_left);
    ASSERT_NOT_NULL(renderer.binaural_right);
    
    atmos_renderer_destroy(&renderer);
}

// Test: Add audio object
TEST(add_object) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    float position[3] = {1.0f, 0.0f, 2.0f};
    int obj_id = atmos_add_object(&renderer, position);
    
    ASSERT_EQ(obj_id, 0);
    ASSERT_EQ(renderer.object_count, 1);
    
    AtmosAudioObject *obj = &renderer.objects[0];
    ASSERT_FLOAT_EQ(obj->position[0], 1.0f);
    ASSERT_FLOAT_EQ(obj->position[1], 0.0f);
    ASSERT_FLOAT_EQ(obj->position[2], 2.0f);
    ASSERT_FLOAT_EQ(obj->gain, 1.0f);
    
    atmos_renderer_destroy(&renderer);
}

// Test: Update object position
TEST(update_object_position) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    float initial_pos[3] = {0.0f, 0.0f, 0.0f};
    int obj_id = atmos_add_object(&renderer, initial_pos);
    
    float new_pos[3] = {2.0f, 1.5f, 3.0f};
    atmos_update_object_position(&renderer, obj_id, new_pos);
    
    AtmosAudioObject *obj = &renderer.objects[obj_id];
    ASSERT_FLOAT_EQ(obj->position[0], 2.0f);
    ASSERT_FLOAT_EQ(obj->position[1], 1.5f);
    ASSERT_FLOAT_EQ(obj->position[2], 3.0f);
    
    atmos_renderer_destroy(&renderer);
}

// Test: Set object audio
TEST(set_object_audio) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    float position[3] = {0.0f, 0.0f, 1.0f};
    int obj_id = atmos_add_object(&renderer, position);
    
    // Create test audio buffer
    int samples = 512;
    float *test_audio = (float*)malloc(samples * sizeof(float));
    for (int i = 0; i < samples; i++) {
        test_audio[i] = sinf(2.0f * 3.14159f * 440.0f * i / 48000.0f);
    }
    
    atmos_set_object_audio(&renderer, obj_id, test_audio, samples);
    
    // Verify audio was copied
    AtmosAudioObject *obj = &renderer.objects[obj_id];
    ASSERT_NOT_NULL(obj->audio_buffer);
    
    free(test_audio);
    atmos_renderer_destroy(&renderer);
}

// Test: VBAP panning (object at center should distribute evenly)
TEST(vbap_center_object) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    spatial_audio_load_preset_51(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    // Object at center front
    float position[3] = {0.0f, 0.0f, 2.0f};
    int obj_id = atmos_add_object(&renderer, position);
    
    // Create sine wave
    int samples = 512;
    float *test_audio = (float*)malloc(samples * sizeof(float));
    for (int i = 0; i < samples; i++) {
        test_audio[i] = 0.5f;  // Constant 0.5 for easy testing
    }
    atmos_set_object_audio(&renderer, obj_id, test_audio, samples);
    
    // Render frame
    atmos_render_frame(&renderer);
    
    // Center speaker should have most energy
    float *center_output = (float*)malloc(samples * sizeof(float));
    atmos_get_speaker_output(&renderer, SPEAKER_FRONT_CENTER, center_output, samples);
    
    // Check that center has non-zero output
    float total = 0.0f;
    for (int i = 0; i < samples; i++) {
        total += fabsf(center_output[i]);
    }
    ASSERT_TRUE(total > 0.0f);
    
    free(test_audio);
    free(center_output);
    atmos_renderer_destroy(&renderer);
}

// Test: Binaural rendering
TEST(binaural_rendering) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    config.atmos_binaural_mode = true;
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    // Object on left side
    float position[3] = {-2.0f, 0.0f, 1.0f};
    int obj_id = atmos_add_object(&renderer, position);
    
    int samples = 512;
    float *test_audio = (float*)malloc(samples * sizeof(float));
    for (int i = 0; i < samples; i++) {
        test_audio[i] = 0.5f;
    }
    atmos_set_object_audio(&renderer, obj_id, test_audio, samples);
    
    float *left_out = (float*)malloc(samples * sizeof(float));
    float *right_out = (float*)malloc(samples * sizeof(float));
    
    atmos_render_binaural(&renderer, left_out, right_out, samples);
    
    // Left output should be stronger than right
    float left_energy = 0.0f, right_energy = 0.0f;
    for (int i = 0; i < samples; i++) {
        left_energy += fabsf(left_out[i]);
        right_energy += fabsf(right_out[i]);
    }
    
    ASSERT_TRUE(left_energy > right_energy);
    
    free(test_audio);
    free(left_out);
    free(right_out);
    atmos_renderer_destroy(&renderer);
}

// Test: Multiple objects rendering
TEST(multiple_objects) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    spatial_audio_load_preset_51(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    // Add 3 objects at different positions
    float pos1[3] = {-1.0f, 0.0f, 2.0f};
    float pos2[3] = {1.0f, 0.0f, 2.0f};
    float pos3[3] = {0.0f, 0.0f, 1.0f};
    
    int obj1 = atmos_add_object(&renderer, pos1);
    int obj2 = atmos_add_object(&renderer, pos2);
    int obj3 = atmos_add_object(&renderer, pos3);
    
    ASSERT_EQ(renderer.object_count, 3);
    ASSERT_EQ(obj1, 0);
    ASSERT_EQ(obj2, 1);
    ASSERT_EQ(obj3, 2);
    
    atmos_renderer_destroy(&renderer);
}

// Test: Object limit
TEST(object_limit) {
    SpatialAudioConfig config;
    spatial_audio_config_init(&config);
    
    DolbyAtmosRenderer renderer;
    atmos_renderer_init(&renderer, &config);
    
    // Try to add more than max_objects
    float position[3] = {0.0f, 0.0f, 1.0f};
    for (int i = 0; i < renderer.max_objects + 10; i++) {
        int obj_id = atmos_add_object(&renderer, position);
        if (i < renderer.max_objects) {
            ASSERT_TRUE(obj_id >= 0);
        } else {
            ASSERT_EQ(obj_id, -1);  // Should fail
        }
    }
    
    ASSERT_EQ(renderer.object_count, renderer.max_objects);
    
    atmos_renderer_destroy(&renderer);
}

int main() {
    printf("Running Dolby Atmos Renderer Tests\n");
    printf("===================================\n\n");
    
    RUN_TEST(renderer_init);
    RUN_TEST(add_object);
    RUN_TEST(update_object_position);
    RUN_TEST(set_object_audio);
    RUN_TEST(vbap_center_object);
    RUN_TEST(binaural_rendering);
    RUN_TEST(multiple_objects);
    RUN_TEST(object_limit);
    
    printf("\n===================================\n");
    printf("All tests passed!\n");
    
    return 0;
}
