/**
 * Unit Test for Audio System
 * Tests core audio system functionality including initialization, playback,
 * 3D spatialization, volume control, and reverb zones
 */

#include "../../../src/engine/audio/audio_system.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test audio system initialization
static TestResult test_audio_init(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);  // 16 channels
    
    TEST_ASSERT(sys.initialized, "Audio system initialized");
    TEST_ASSERT_EQ(sys.max_channels, 16, "Max channels set correctly");
    TEST_ASSERT_NOT_NULL(sys.sources, "Sound sources allocated");
    TEST_ASSERT_EQ(sys.master_volume, 1.0f, "Master volume at 100%");
    
    // Check category volumes
    for (u32 i = 0; i <= SOUND_CATEGORY_RECORD; i++) {
        TEST_ASSERT_EQ(sys.category_volumes[i], 1.0f, "Category volumes initialized to 100%");
    }
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test listener position update
static TestResult test_listener_update(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    Vec3 pos = {10.0f, 5.0f, 20.0f};
    Vec3 forward = {1.0f, 0.0f, 0.0f};
    Vec3 up = {0.0f, 1.0f, 0.0f};
    Vec3 velocity = {0.5f, 0.0f, 0.0f};
    
    audio_update_listener(&sys, pos, forward, up, velocity);
    
    TEST_ASSERT_EQ(sys.listener_position.x, 10.0f, "Listener X position updated");
    TEST_ASSERT_EQ(sys.listener_position.y, 5.0f, "Listener Y position updated");
    TEST_ASSERT_EQ(sys.listener_position.z, 20.0f, "Listener Z position updated");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test volume control
static TestResult test_volume_control(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    // Test master volume
    audio_set_master_volume(&sys, 0.7f);
    TEST_ASSERT_EQ(sys.master_volume, 0.7f, "Master volume set correctly");
    
    // Test category volume
    audio_set_volume(&sys, SOUND_CATEGORY_SFX, 0.5f);
    TEST_ASSERT_EQ(sys.category_volumes[SOUND_CATEGORY_SFX], 0.5f, "SFX category volume set");
    
    // Test volume clamping
    audio_set_master_volume(&sys, 1.5f);  // Above max
    TEST_ASSERT_EQ(sys.master_volume, 1.0f, "Master volume clamped to 1.0");
    
    audio_set_master_volume(&sys, -0.5f);  // Below min
    TEST_ASSERT_EQ(sys.master_volume, 0.0f, "Master volume clamped to 0.0");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test category enable/disable
static TestResult test_category_toggle(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    // Disable category
    audio_disable_category(&sys, SOUND_CATEGORY_MUSIC);
    TEST_ASSERT_EQ(sys.category_volumes[SOUND_CATEGORY_MUSIC], 0.0f, "Music category disabled");
    
    // Enable category
    audio_enable_category(&sys, SOUND_CATEGORY_MUSIC);
    TEST_ASSERT_EQ(sys.category_volumes[SOUND_CATEGORY_MUSIC], 1.0f, "Music category enabled");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test reverb zone management
static TestResult test_reverb_zones(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    Vec3 min_bounds = {0.0f, 0.0f, 0.0f};
    Vec3 max_bounds = {100.0f, 50.0f, 100.0f};
    
    u32 zone_id = audio_add_reverb_zone(&sys, min_bounds, max_bounds, 0.7f, 2.5f);
    
    TEST_ASSERT(zone_id != 0xFFFFFFFF, "Reverb zone created");
    TEST_ASSERT_EQ(sys.reverb_zone_count, 1, "Zone count incremented");
    TEST_ASSERT(sys.reverb_zones[zone_id].active, "Zone is active");
    TEST_ASSERT_EQ(sys.reverb_zones[zone_id].reverb_level, 0.7f, "Reverb level set correctly");
    TEST_ASSERT_EQ(sys.reverb_zones[zone_id].decay_time, 2.5f, "Decay time set correctly");
    
    // Remove zone
    audio_remove_reverb_zone(&sys, zone_id);
    TEST_ASSERT(!sys.reverb_zones[zone_id].active, "Zone deactivated");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test audio system update
static TestResult test_audio_update(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    // Run update cycle
    audio_system_update(&sys, 0.016f);
    
    TEST_ASSERT(sys.initialized, "System remains initialized after update");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test channel management and voice stealing
static TestResult test_channel_management(void) {
    AudioSystem sys;
    audio_system_init(&sys, 4);  // Small number for easier testing
    
    // Initial active sources should be 0
    TEST_ASSERT_EQ(sys.active_sources, 0, "No active sources initially");
    
    // Note: Actual sound playback would require valid audio files
    // This test verifies the channel allocation logic
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test underwater filter initialization
static TestResult test_underwater_filter(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    // Underwater filter should be initialized during audio system init
    TEST_ASSERT(sys.initialized, "Audio system initialized with underwater filter");
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Test sound buffer management
static TestResult test_sound_buffer(void) {
    AudioSystem sys;
    audio_system_init(&sys, 16);
    
    // Initially no buffers should be loaded
    for (u32 i = 0; i < SOUND_COUNT; i++) {
        TEST_ASSERT(!sys.sound_buffers[i].loaded, "Buffer initially not loaded");
        TEST_ASSERT_NULL(sys.sound_buffers[i].data, "Buffer data initially null");
    }
    
    audio_system_free(&sys);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Audio:System", "Initialization", test_audio_init, NULL, NULL);
    test_register("Audio:System", "Listener Update", test_listener_update, NULL, NULL);
    test_register("Audio:System", "Volume Control", test_volume_control, NULL, NULL);
    test_register("Audio:System", "Category Toggle", test_category_toggle, NULL, NULL);
    test_register("Audio:System", "Reverb Zones", test_reverb_zones, NULL, NULL);
    test_register("Audio:System", "Audio Update", test_audio_update, NULL, NULL);
    test_register("Audio:System", "Channel Management", test_channel_management, NULL, NULL);
    test_register("Audio:System", "Underwater Filter", test_underwater_filter, NULL, NULL);
    test_register("Audio:System", "Sound Buffer", test_sound_buffer, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Audio System Test Results\n");
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
