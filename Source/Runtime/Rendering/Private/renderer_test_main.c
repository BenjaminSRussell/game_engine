#include "unified_logger.h"
#include "unified_memory.h"
#include "unified_renderer.h"
#include <stdio.h>

int main(void) {
  printf("Starting Renderer Verification...\\n");

  // Initialize logger
  if (!logger_init(NULL)) {
    printf("FAILED: Logger init\\n");
    return 1;
  }
  LOG_INFO(LOG_CAT_RENDERER, "Renderer test started");

  // Test 1: Renderer Context Creation
  RenderConfig config = {.backend =
                             RENDER_BACKEND_AUTO, // Will select Metal on macOS
                         .window_handle = NULL,   // Headless for now
                         .width = 1920,
                         .height = 1080,
                         .format = RENDER_FORMAT_B8G8R8A8_SRGB,
                         .present_mode = RENDER_PRESENT_MODE_FIFO,
                         .swapchain_image_count = 2,
                         .enable_validation = true,
                         .enable_profiling = true,
                         .enable_vsync = true};

  RenderContext *renderer = render_init(&config);
  if (!renderer) {
    printf("FAILED: Renderer initialization\\n");
    return 1;
  }
  printf("PASSED: Renderer initialized\\n");

  // Test 2: Check initialization state
  if (!render_is_initialized(renderer)) {
    printf("FAILED: Renderer not initialized\\n");
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Renderer is initialized\\n");

  // Test 3: Get device
  RenderDevice *device = render_get_device(renderer);
  if (!device) {
    printf("FAILED: Failed to get device\\n");
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Device retrieved\\n");

  // Test 4: Get stats
  RenderStats stats;
  render_get_stats(renderer, &stats);
  printf("PASSED: Stats retrieved (frame_count=%llu)\\n",
         (unsigned long long)stats.frame_count);

  // Test 5: VSync toggle
  render_set_vsync(renderer, false);
  render_set_vsync(renderer, true);
  printf("PASSED: VSync toggle\\n");

  // Test 6: Simulate frame (no window)
  render_begin_frame(renderer);
  // Would do rendering here
  render_end_frame(renderer);
  printf("PASSED: Frame simulation\\n");

  // Test 7: Check frame count incremented
  render_get_stats(renderer, &stats);
  if (stats.frame_count != 1) {
    printf("FAILED: Frame count not incremented (expected 1, got %llu)\\n",
           (unsigned long long)stats.frame_count);
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Frame count incremented\\n");

  // Test 8: Cleanup
  render_shutdown(renderer);
  printf("PASSED: Renderer shutdown\\n");

  logger_shutdown();

  printf("\\nRenderer Verification Successful!\\n");
  return 0;
}
