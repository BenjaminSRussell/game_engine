/**
 * Unit Test for AI Perception System
 * Tests NPC perception capabilities (vision, hearing, etc.)
 */

#include "../../../src/engine/ai/npc/perception_system.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test perception system initialization
static TestResult test_perception_init(void) {
    PerceptionSystem* psys = perception_system_create();
    TEST_ASSERT_NOT_NULL(psys, "Perception system created");
    perception_system_destroy(psys);
    return TEST_PASS;
}

// Test adding a perceiver (NPC)
static TestResult test_add_perceiver(void) {
    PerceptionSystem* psys = perception_system_create();
    
    PerceptionConfig config = {
        .vision_range = 50.0f,
        .hearing_range = 30.0f,
        .vision_angle = 120.0f,
        .update_frequency = 0.1f
    };
    
    Entity npc_id = 1001;
    bool success = perception_system_add_perceiver(psys, npc_id, &config);
    TEST_ASSERT(success, "Perceiver added successfully");
    
    perception_system_destroy(psys);
    return TEST_PASS;
}

// Test vision detection
static TestResult test_vision_detection(void) {
    PerceptionSystem* psys = perception_system_create();
    
    PerceptionConfig config = {
        .vision_range = 50.0f,
        .vision_angle = 120.0f
    };
    
    Entity npc_id = 2001;
    perception_system_add_perceiver(psys, npc_id, &config);
    
    // Set NPC position and direction
    Vec3 npc_pos = {0.0f, 0.0f, 0.0f};
    Vec3 npc_forward = {1.0f, 0.0f, 0.0f};
    perception_system_update_perceiver_transform(psys, npc_id, npc_pos, npc_forward);
    
    // Add a stimuli (target) within vision range and angle
    Vec3 target_pos = {30.0f, 0.0f, 0.0f};  // 30 units ahead
    Entity target_id = 3001;
    perception_system_add_stimuli(psys, target_id, STIMULI_TYPE_VISUAL, target_pos);
    
    // Update and check if target is perceived
    perception_system_update(psys, 0.016f);
    
    PerceivedEntity* perceived = perception_system_get_perceived(psys, npc_id);
    TEST_ASSERT_NOT_NULL(perceived, "Target should be perceived");
    
    perception_system_destroy(psys);
    return TEST_PASS;
}

// Test hearing detection
static TestResult test_hearing_detection(void) {
    PerceptionSystem* psys = perception_system_create();
    
    PerceptionConfig config = {
        .hearing_range = 40.0f
    };
    
    Entity npc_id = 4001;
    perception_system_add_perceiver(psys, npc_id, &config);
    
    Vec3 npc_pos = {0.0f, 0.0f, 0.0f};
    Vec3 npc_forward = {1.0f, 0.0f, 0.0f};
    perception_system_update_perceiver_transform(psys, npc_id, npc_pos, npc_forward);
    
    // Add audio stimuli
    Vec3 sound_pos = {0.0f, 0.0f, 35.0f};  // 35 units away (within hearing range)
    Entity sound_id = 5001;
    perception_system_add_stimuli(psys, sound_id, STIMULI_TYPE_AUDIO, sound_pos);
    
    perception_system_update(psys, 0.016f);
    
    PerceivedEntity* perceived = perception_system_get_perceived(psys, npc_id);
    TEST_ASSERT_NOT_NULL(perceived, "Sound should be heard");
    
    perception_system_destroy(psys);
    return TEST_PASS;
}

// Test out of range detection
static TestResult test_out_of_range(void) {
    PerceptionSystem* psys = perception_system_create();
    
    PerceptionConfig config = {
        .vision_range = 50.0f,
        .hearing_range = 30.0f
    };
    
    Entity npc_id = 6001;
    perception_system_add_perceiver(psys, npc_id, &config);
    
    Vec3 npc_pos = {0.0f, 0.0f, 0.0f};
    Vec3 npc_forward = {1.0f, 0.0f, 0.0f};
    perception_system_update_perceiver_transform(psys, npc_id, npc_pos, npc_forward);
    
    // Add target far away (out of range)
    Vec3 target_pos = {100.0f, 0.0f, 0.0f};
    Entity target_id = 7001;
    perception_system_add_stimuli(psys, target_id, STIMULI_TYPE_VISUAL, target_pos);
    
    perception_system_update(psys, 0.016f);
    
    PerceivedEntity* perceived = perception_system_get_perceived(psys, npc_id);
    TEST_ASSERT(perceived == NULL || perceived->count == 0, "Out of range target should not be perceived");
    
    perception_system_destroy(psys);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:Perception", "Initialization", test_perception_init, NULL, NULL);
    test_register("AI:Perception", "Add Perceiver", test_add_perceiver, NULL, NULL);
    test_register("AI:Perception", "Vision Detection", test_vision_detection, NULL, NULL);
    test_register("AI:Perception", "Hearing Detection", test_hearing_detection, NULL, NULL);
    test_register("AI:Perception", "Out of Range", test_out_of_range, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  AI Perception System Test Results\n");
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
