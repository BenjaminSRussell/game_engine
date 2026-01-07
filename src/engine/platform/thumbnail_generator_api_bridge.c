// Thumbnail Generator API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/thumbnail_generator_api_bridge.h"
#include <stdlib.h>
#include <string.h>

// Mock implementation data
typedef struct {
  uint64_t id;
  char path[256];
  uint32_t width;
  uint32_t height;
  bool ready;
  void *data;
} ThumbnailRequest;

static uint64_t g_next_request_id = 1;

uint64_t thumbnail_request(const char *asset_path, uint32_t width,
                           uint32_t height) {
  if (!asset_path)
    return 0;

  uint64_t id = g_next_request_id++;
  LOG_INFO("Requesting thumbnail %llu for %s (%ux%u)", id, asset_path, width,
           height);

  // In real implementation, this would queue a job to the render thread
  // For now we simulate immediate success for simple cases or mock async
  return id;
}

bool thumbnail_is_ready(uint64_t request_id) {
  // Mock: always ready after request (simulating instant load or cached)
  return true;
}

bool thumbnail_get_data(uint64_t request_id, void *buffer,
                        uint32_t buffer_size) {
  if (!buffer || buffer_size == 0)
    return false;

  // Mock: Fill with random color/noise
  uint8_t *pixels = (uint8_t *)buffer;
  for (uint32_t i = 0; i < buffer_size; i++) {
    pixels[i] = rand() % 255;
  }

  return true;
}

void thumbnail_cancel(uint64_t request_id) {
  LOG_INFO("Canceling thumbnail request %llu", request_id);
}

void thumbnail_clear_cache(void) { LOG_INFO("Clearing thumbnail cache"); }

uint32_t thumbnail_get_pending_count(void) { return 0; }
