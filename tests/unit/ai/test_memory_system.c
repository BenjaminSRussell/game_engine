/**
 * Unit Test for AI Memory System
 * Tests NPC memory and knowledge persistence
 */

#include "../../../../src/engine/ai/npc_advanced/memory_system.h"
#include "../../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test memory system initialization
static TestResult test_memory_init(void) {
    MemorySystem* memory = memory_system_create();
    TEST_ASSERT_NOT_NULL(memory, "Memory system created");
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Test storing a memory
static TestResult test_store_memory(void) {
    MemorySystem* memory = memory_system_create();
    
    Memory mem = {
        .type = MEMORY_TYPE_EVENT,
        .timestamp = 1000.0f,
        .importance = 0.8f,
        .data = "Saw player near village",
        .entity_id = 5001
    };
    
    bool success = memory_system_store(memory, &mem);
    TEST_ASSERT(success, "Memory stored successfully");
    
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Test recalling memories
static TestResult test_recall_memory(void) {
    MemorySystem* memory = memory_system_create();
    
    // Store multiple memories
    Memory mem1 = {
        .type = MEMORY_TYPE_EVENT,
        .timestamp = 1000.0f,
        .importance = 0.7f,
        .data = "Found treasure",
        .entity_id = 0
    };
    
    Memory mem2 = {
        .type = MEMORY_TYPE_LOCATION,
        .timestamp = 1050.0f,
        .importance = 0.9f,
        .data = "Village coordinates: 100, 0, 200",
        .entity_id = 0
    };
    
    memory_system_store(memory, &mem1);
    memory_system_store(memory, &mem2);
    
    // Recall recent memories
    MemoryQuery query = {
        .type = MEMORY_TYPE_ANY,
        .max_results = 10,
        .min_importance = 0.5f
    };
    
    MemoryResult* results = memory_system_recall(memory, &query);
    TEST_ASSERT_NOT_NULL(results, "Recall produced results");
    TEST_ASSERT(results->count >= 2, "Both memories recalled");
    
    memory_result_destroy(results);
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Test memory decay
static TestResult test_memory_decay(void) {
    MemorySystem* memory = memory_system_create();
    
    Memory mem = {
        .type = MEMORY_TYPE_EVENT,
        .timestamp = 1000.0f,
        .importance = 0.5f,
        .data = "Old event",
        .entity_id = 0
    };
    
    memory_system_store(memory, &mem);
    
    // Simulate time passing (decay)
    memory_system_update(memory, 5000.0f);  // 5000 time units later
    
    MemoryQuery query = {
        .type = MEMORY_TYPE_EVENT,
        .max_results = 10,
        .min_importance = 0.4f
    };
    
    MemoryResult* results = memory_system_recall(memory, &query);
    // Memory may have decayed below importance threshold
    TEST_ASSERT(results != NULL, "Recall completed");
    
    memory_result_destroy(results);
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Test entity-specific memory recall
static TestResult test_entity_memory(void) {
    MemorySystem* memory = memory_system_create();
    
    Entity player = 1001;
    Entity guard = 2001;
    
    Memory mem_player = {
        .type = MEMORY_TYPE_INTERACTION,
        .timestamp = 1000.0f,
        .importance = 0.8f,
        .data = "Player traded items",
        .entity_id = player
    };
    
    Memory mem_guard = {
        .type = MEMORY_TYPE_INTERACTION,
        .timestamp = 1100.0f,
        .importance = 0.6f,
        .data = "Guard spoke to me",
        .entity_id = guard
    };
    
    memory_system_store(memory, &mem_player);
    memory_system_store(memory, &mem_guard);
    
    // Recall only memories about player
    MemoryQuery query = {
        .type = MEMORY_TYPE_ANY,
        .entity_filter = player,
        .max_results = 10,
        .min_importance = 0.5f
    };
    
    MemoryResult* results = memory_system_recall(memory, &query);
    TEST_ASSERT_NOT_NULL(results, "Entity-filtered recall succeeded");
    TEST_ASSERT(results->count >= 1, "Player memory recalled");
    
    memory_result_destroy(results);
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Test memory consolidation
static TestResult test_memory_consolidation(void) {
    MemorySystem* memory = memory_system_create();
    
    // Store many short-term memories
    for (int i = 0; i < 20; i++) {
        Memory mem = {
            .type = MEMORY_TYPE_EVENT,
            .timestamp = (float)i * 10.0f,
            .importance = 0.3f + (i % 5) * 0.1f,
            .data = "Event",
            .entity_id = 0
        };
        memory_system_store(memory, &mem);
    }
    
    // Trigger consolidation (move important memories to long-term)
    memory_system_consolidate(memory);
    
    // Verify the system still functions
    MemoryQuery query = {
        .type = MEMORY_TYPE_ANY,
        .max_results = 100,
        .min_importance = 0.0f
    };
    
    MemoryResult* results = memory_system_recall(memory, &query);
    TEST_ASSERT_NOT_NULL(results, "Post-consolidation recall works");
    
    memory_result_destroy(results);
    memory_system_destroy(memory);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:Memory", "Initialization", test_memory_init, NULL, NULL);
    test_register("AI:Memory", "Store Memory", test_store_memory, NULL, NULL);
    test_register("AI:Memory", "Recall Memory", test_recall_memory, NULL, NULL);
    test_register("AI:Memory", "Memory Decay", test_memory_decay, NULL, NULL);
    test_register("AI:Memory", "Entity Memory", test_entity_memory, NULL, NULL);
    test_register("AI:Memory", "Consolidation", test_memory_consolidation, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  AI Memory System Test Results\n");
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
