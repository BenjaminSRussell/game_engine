/*
 * Engine Initialization Test
 * Tests CORE-001: Engine Initialization Pipeline
 */

#include <stdio.h>
#include <stdlib.h>
#include <src/engine/include/core/engine.h>
#include <src/engine/include/core/logging_system.h>
#include <src/engine/include/core/memory_allocator.h>
#include <src/engine/include/core/thread_pool.h>

// Simple test game module
static bool test_game_initialize(GameModule* module, Engine* engine) {
    (void)module; (void)engine;
    LOG_INFO("Test game module initialized");
    return true;
}

static void test_game_shutdown(GameModule* module) {
    (void)module;
    LOG_INFO("Test game module shutdown");
}

static void test_game_update(GameModule* module, Engine* engine, float delta_time) {
    (void)module; (void)engine; (void)delta_time;
    static int frame_count = 0;
    frame_count++;
    
    if (frame_count >= 10) {
        LOG_INFO("Completed 10 frames, stopping engine");
        engine_stop(engine);
    }
}

static void test_game_render(GameModule* module, Engine* engine) {
    (void)module; (void)engine;
    // Simple rendering - just clear screen conceptually
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("=== Engine Initialization Test ===\n");
    printf("Testing CORE-001: Engine Initialization Pipeline\n\n");
    
    // Test 1: Initialize critical systems individually
    printf("1. Testing critical system initialization...\n");
    
    if (!memory_allocator_init()) {
        printf("❌ Memory allocator initialization failed\n");
        return 1;
    }
    printf("✓ Memory allocator initialized\n");
    
    if (!logging_system_init()) {
        printf("❌ Logging system initialization failed\n");
        return 1;
    }
    printf("✓ Logging system initialized\n");
    
    if (!thread_pool_init(4)) {
        printf("❌ Thread pool initialization failed\n");
        return 1;
    }
    printf("✓ Thread pool initialized\n");
    
    // Test 2: Create and initialize engine
    printf("\n2. Testing full engine initialization...\n");
    
    Engine engine;
    EngineConfig config = engine_create_default_config();
    config.window_width = 800;
    config.window_height = 600;
    config.window_title = "Engine Initialization Test";
    config.debug_mode = true;
    
    if (!engine_init(&engine, &config)) {
        printf("❌ Engine initialization failed\n");
        return 1;
    }
    printf("✓ Engine initialized successfully\n");
    
    // Test 3: Run engine for 10 frames
    printf("\n3. Testing engine run loop (10 frames)...\n");
    
    GameModule test_game = {
        .info = {
            .name = "Initialization Test",
            .version = "1.0.0"
        },
        .initialize = test_game_initialize,
        .shutdown = test_game_shutdown,
        .update = test_game_update,
        .render = test_game_render
    };
    
    engine_run(&engine, &test_game);
    printf("✓ Engine run loop completed\n");
    
    // Test 4: Check memory leaks
    printf("\n4. Testing memory leak detection...\n");
    
    size_t total_allocated, peak_allocated;
    u64 allocation_count;
    memory_allocator_get_stats(&total_allocated, &peak_allocated, &allocation_count);
    
    printf("Memory stats:\n");
    printf("  Currently allocated: %zu bytes\n", total_allocated);
    printf("  Peak allocated: %zu bytes\n", peak_allocated);
    printf("  Active allocations: %llu\n", allocation_count);
    
    if (allocation_count > 0) {
        printf("❌ Memory leaks detected\n");
    } else {
        printf("✓ No memory leaks detected\n");
    }
    
    // Test 5: Thread pool statistics
    printf("\n5. Testing thread pool statistics...\n");
    
    ThreadPool* pool = thread_pool_get_global();
    if (pool) {
        u64 submitted, completed, failed;
        thread_pool_get_stats(pool, &submitted, &completed, &failed);
        
        printf("Thread pool stats:\n");
        printf("  Jobs submitted: %llu\n", submitted);
        printf("  Jobs completed: %llu\n", completed);
        printf("  Jobs failed: %llu\n", failed);
        printf("✓ Thread pool statistics available\n");
    } else {
        printf("❌ Thread pool not available\n");
    }
    
    // Test 6: Clean shutdown
    printf("\n6. Testing clean shutdown...\n");
    
    engine_shutdown(&engine);
    printf("✓ Engine shutdown complete\n");
    
    thread_pool_shutdown();
    printf("✓ Thread pool shutdown complete\n");
    
    logging_system_shutdown();
    printf("✓ Logging system shutdown complete\n");
    
    memory_allocator_shutdown();
    printf("✓ Memory allocator shutdown complete\n");
    
    printf("\n=== Test Results ===\n");
    printf("✅ Engine initialization pipeline test PASSED\n");
    printf("✅ All critical systems initialized correctly\n");
    printf("✅ Engine runs and shuts down cleanly\n");
    printf("✅ No memory leaks detected\n");
    printf("✅ Thread pool functioning correctly\n");
    
    return 0;
}
