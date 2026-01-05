/**
 * Unit Test for AI Content Manager
 * Tests AI-driven content generation functionality
 */

#include "../../../src/engine/ai/content/content_manager.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test initialization
static TestResult test_content_manager_init(void) {
    ContentManager* mgr = content_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Content manager created successfully");
    content_manager_destroy(mgr);
    return TEST_PASS;
}

// Test generating item content
static TestResult test_generate_item(void) {
    ContentManager* mgr = content_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Content manager created");
    
    ContentRequest req = {
        .type = CONTENT_TYPE_ITEM,
        .prompt = "A magical sword with fire enchantment",
        .max_tokens = 100
    };
    
    ContentResult* result = content_manager_generate(mgr, &req);
    TEST_ASSERT_NOT_NULL(result, "Generated content result");
    TEST_ASSERT(result->success, "Generation was successful");
    TEST_ASSERT_NOT_NULL(result->data, "Result data is not null");
    
    content_result_destroy(result);
    content_manager_destroy(mgr);
    return TEST_PASS;
}

// Test generating quest content
static TestResult test_generate_quest(void) {
    ContentManager* mgr = content_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Content manager created");
    
    ContentRequest req = {
        .type = CONTENT_TYPE_QUEST,
        .prompt = "A rescue mission in a dark dungeon",
        .max_tokens = 200
    };
    
    ContentResult* result = content_manager_generate(mgr, &req);
    TEST_ASSERT_NOT_NULL(result, "Generated quest content");
    
    content_result_destroy(result);
    content_manager_destroy(mgr);
    return TEST_PASS;
}

// Test generating dialogue
static TestResult test_generate_dialogue(void) {
    ContentManager* mgr = content_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Content manager created");
    
    ContentRequest req = {
        .type = CONTENT_TYPE_DIALOGUE,
        .prompt = "A friendly merchant greeting the player",
        .max_tokens = 50
    };
    
    ContentResult* result = content_manager_generate(mgr, &req);
    TEST_ASSERT_NOT_NULL(result, "Generated dialogue content");
    
    content_result_destroy(result);
    content_manager_destroy(mgr);
    return TEST_PASS;
}

// Test invalid request
static TestResult test_invalid_request(void) {
    ContentManager* mgr = content_manager_create();
    TEST_ASSERT_NOT_NULL(mgr, "Content manager created");
    
    // Empty prompt
    ContentRequest req = {
        .type = CONTENT_TYPE_ITEM,
        .prompt = "",
        .max_tokens = 100
    };
    
    ContentResult* result = content_manager_generate(mgr, &req);
    if (result) {
        TEST_ASSERT(!result->success, "Empty prompt should fail");
        content_result_destroy(result);
    }
    
    content_manager_destroy(mgr);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:ContentManager", "Initialization", test_content_manager_init, NULL, NULL);
    test_register("AI:ContentManager", "Generate Item", test_generate_item, NULL, NULL);
    test_register("AI:ContentManager", "Generate Quest", test_generate_quest, NULL, NULL);
    test_register("AI:ContentManager", "Generate Dialogue", test_generate_dialogue, NULL, NULL);
    test_register("AI:ContentManager", "Invalid Request", test_invalid_request, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  AI Content Manager Test Results\n");
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
