/**
 * Post-Processing System Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(expr) do { \
    tests_run++; \
    if (expr) { \
        tests_passed++; \
        printf("  ✓ %s\n", #expr); \
    } else { \
        printf("  ✗ FAILED: %s (line %d)\n", #expr, __LINE__); \
    } \
} while(0)

// Mock Structures
typedef struct {
    char name[64];
    int enabled;
    float intensity;
} PostEffect;

typedef struct {
    PostEffect* effects[16];
    int effect_count;
} PostPipeline;

// Mock API
PostPipeline* post_pipeline_create() {
    return calloc(1, sizeof(PostPipeline));
}

void post_pipeline_destroy(PostPipeline* pipeline) {
    if (!pipeline) return;
    for (int i = 0; i < pipeline->effect_count; i++) free(pipeline->effects[i]);
    free(pipeline);
}

PostEffect* post_pipeline_add_effect(PostPipeline* pipeline, const char* name) {
    if (pipeline->effect_count >= 16) return NULL;
    PostEffect* effect = calloc(1, sizeof(PostEffect));
    strncpy(effect->name, name, 63);
    effect->enabled = 1; // Default on
    effect->intensity = 1.0f;
    pipeline->effects[pipeline->effect_count++] = effect;
    return effect;
}

void post_effect_set_intensity(PostEffect* effect, float intensity) {
    if (effect) effect->intensity = intensity;
}

void post_effect_set_enabled(PostEffect* effect, int enabled) {
    if (effect) effect->enabled = enabled;
}

// Tests
void test_pipeline_creation() {
    printf("Testing pipeline creation...\n");
    PostPipeline* pipeline = post_pipeline_create();
    TEST_ASSERT(pipeline != NULL);
    TEST_ASSERT(pipeline->effect_count == 0);
    post_pipeline_destroy(pipeline);
}

void test_add_effects() {
    printf("Testing adding effects...\n");
    PostPipeline* pipeline = post_pipeline_create();
    
    PostEffect* bloom = post_pipeline_add_effect(pipeline, "Bloom");
    PostEffect* ssao = post_pipeline_add_effect(pipeline, "SSAO");
    
    TEST_ASSERT(bloom != NULL);
    TEST_ASSERT(ssao != NULL);
    TEST_ASSERT(pipeline->effect_count == 2);
    TEST_ASSERT(strcmp(bloom->name, "Bloom") == 0);
    
    post_pipeline_destroy(pipeline);
}

void test_effect_properties() {
    printf("Testing effect properties...\n");
    PostPipeline* pipeline = post_pipeline_create();
    PostEffect* blur = post_pipeline_add_effect(pipeline, "Blur");
    
    post_effect_set_intensity(blur, 0.5f);
    post_effect_set_enabled(blur, 0);
    
    TEST_ASSERT(blur->intensity == 0.5f);
    TEST_ASSERT(blur->enabled == 0);
    
    post_pipeline_destroy(pipeline);
}

int main() {
    printf("=== Post-Processing Tests ===\n\n");
    
    test_pipeline_creation();
    test_add_effects();
    test_effect_properties();
    
    printf("\n=== Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_run == tests_passed) ? 0 : 1;
}
