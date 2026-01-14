/**
 * @file core_init.c
 * @brief Core subsystem initialization
 */

#include "Core/Public/core.h"

VF_Result vf_core_init(void) {
  VF_Result result;

  // Initialize memory first (other systems depend on it)
  result = vf_memory_init();
  if (result != VF_SUCCESS) {
    return result;
  }

  // Initialize logging
  result = vf_logger_init();
  if (result != VF_SUCCESS) {
    vf_memory_shutdown();
    return result;
  }

  LOG_INFO("VoxelForge Core initialized successfully");
  LOG_INFO("  Platform: %s",
#if defined(VF_PLATFORM_WINDOWS)
           "Windows"
#elif defined(VF_PLATFORM_MACOS)
           "macOS"
#elif defined(VF_PLATFORM_LINUX)
           "Linux"
#else
           "Unknown"
#endif
  );

  LOG_INFO("  Hardware threads: %u", vf_thread_hardware_concurrency());

  return VF_SUCCESS;
}

void vf_core_shutdown(void) {
  LOG_INFO("VoxelForge Core shutting down...");

  // Log final memory stats
  VF_AllocationStats stats;
  vf_memory_get_stats(&stats);
  LOG_INFO("Memory stats: %llu allocated, %llu freed, %llu peak",
           stats.total_allocated, stats.total_freed, stats.peak_usage);

  // Shutdown in reverse order
  vf_logger_shutdown();
  vf_memory_shutdown();
}
