/*
 * unified_logger_example.c
 * 
 * Example usage of the consolidated unified logger system
 * This demonstrates how all the different logging backends have been unified
 */

#include "engine/include/core/logger.h"
#include <unistd.h>

int main() {
    // Initialize the unified logger with consolidated configuration
    LoggerConfig config = {
        .min_level = LOG_LEVEL_DEBUG,
        .enabled_channels = LOG_CHANNEL_CONSOLE | LOG_CHANNEL_FILE,
        .use_colors = true,
        .show_timestamp = true,
        .show_file_line = true,
        .show_function = true,
        .use_buffering = true,
        .enable_rotation = true,
        .enable_compression = true,
        .enable_remote = false,
        .log_file_path = "unified_log.txt",
        .max_file_size = 1024 * 1024, // 1MB
        .max_backup_files = 5
    };

    // Enable all categories
    for (int i = 0; i < LOG_CAT_COUNT; i++) {
        config.enabled_categories[i] = true;
    }

    // Initialize the unified logger (consolidates all backends)
    if (!unified_logger_init(&config)) {
        printf("Failed to initialize unified logger\n");
        return 1;
    }

    printf("=== Unified Logger Demo - All Backends Consolidated ===\n");

    // Demonstrate different log levels and categories
    LOG_TRACE_CAT(LOG_CAT_GENERAL, "This is a trace message from consolidated logger");
    LOG_DEBUG_CAT(LOG_CAT_PHYSICS, "Physics system initialized - consolidated backend");
    LOG_INFO_CAT(LOG_CAT_RENDERER, "Renderer started - unified logging active");
    LOG_WARN_CAT(LOG_CAT_AUDIO, "Audio device not found - using fallback");
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Failed to connect to server");

    // Demonstrate structured logging (from structured_logger.h backend)
    LOG_INFO_STRUCT(LOG_CAT_GAME, "Player joined game", 
                  "{\"player_id\": 1234, \"username\": \"test_user\", \"ip\": \"192.168.1.1\"}");

    // Demonstrate legacy compatibility (from original logger.h backend)
    LOGI("Legacy compatibility message - works with unified logger");
    LOGE("Legacy error message - routed through consolidated system");

    // Demonstrate different categories
    LOG_DEBUG_CAT(LOG_CAT_AI, "AI state machine updated");
    LOG_DEBUG_CAT(LOG_CAT_MEMORY, "Memory allocation: 1024 bytes");
    LOG_DEBUG_CAT(LOG_CAT_IO, "File loaded: assets/texture.png");
    LOG_DEBUG_CAT(LOG_CAT_SCRIPT, "Script executed: player_controller.lua");

    // Show logger statistics
    unified_logger_benchmark();

    // Test log rotation
    printf("\nTesting log rotation...\n");
    unified_logger_rotate();

    // Test search functionality
    printf("\nTesting search functionality...\n");
    LogQueryResult result = unified_logger_search_by_level(LOG_LEVEL_INFO);
    printf("Found %u info level entries\n", result.count);
    unified_logger_free_query_result(&result);

    // Flush and shutdown
    unified_logger_flush();
    unified_logger_shutdown();

    printf("\n=== Unified Logger Demo Complete ===\n");
    printf("All logging backends have been successfully consolidated!\n");

    return 0;
}
