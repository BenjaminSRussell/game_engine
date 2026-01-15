#include "unified_input.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include "unified_platform.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  printf("Starting Platform Verification...\\n");

  // Memory system not strictly needed for this test
  // (UNIFIED_ALLOC/FREE will use malloc/free directly if not initialized)
  // Initialize memory system (no config needed)
  // if (!memory_init()) {
  //   printf("FAILED: Memory system init\\n");
  //   return 1;
  // }

  // Initialize logger with NULL config (uses defaults)
  if (!logger_init(NULL)) {
    printf("FAILED: Logger init\\n");
    return 1;
  }
  LOG_INFO(LOG_CAT_SYSTEM, "Platform test started");

  // Test 1: Platform Context Init
  PlatformContext *platform =
      platform_init(PLATFORM_SUBSYSTEM_WINDOW | PLATFORM_SUBSYSTEM_INPUT);
  if (!platform) {
    printf("FAILED: Platform context creation\\n");
    return 1;
  }
  printf("PASSED: Platform context created\\n");

  // Test 2: Window Creation
  WindowConfig window_config = {.title = "Platform Test",
                                .width = 800,
                                .height = 600,
                                .fullscreen = false,
                                .vsync = true,
                                .resizable = true,
                                .native_handle = NULL};

  PlatformWindow *window = platform_window_create(platform, &window_config);
  if (!window) {
    printf("FAILED: Window creation\\n");
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Window created\\n");

  // Test 3: Window Size Query
  u32 width, height;
  platform_window_get_size(window, &width, &height);
  if (width != 800 || height != 600) {
    printf("FAILED: Window size query (expected 800x600, got %ux%u)\\n", width,
           height);
    platform_window_destroy(window);
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Window size query\\n");

  // Test 4: Input System Init
  InputContext *input = input_init();
  if (!input) {
    printf("FAILED: Input context creation\\n");
    platform_window_destroy(window);
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Input context created\\n");

  // Test 5: Input Binding
  input_bind_key(input, INPUT_ACTION_JUMP, 32); // Space key
  printf("PASSED: Input key binding\\n");

  // Test 6: Action Name Lookup
  const char *action_name = input_action_get_name(INPUT_ACTION_JUMP);
  if (!action_name || strcmp(action_name, "JUMP") != 0) {
    printf("FAILED: Action name lookup\\n");
    input_shutdown(input);
    platform_window_destroy(window);
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Action name lookup\\n");

  // Test 7: Input Event Injection
  input_inject_key(input, 32, true); // Press space
  bool is_down = input_is_key_down(input, 32);
  if (!is_down) {
    printf("FAILED: Key injection\\n");
    input_shutdown(input);
    platform_window_destroy(window);
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Input event injection\\n");

  // Test 8: Mouse Position
  input_inject_mouse_move(input, 100.0f, 200.0f);
  f32 mx, my;
  input_get_mouse_position(input, &mx, &my);
  if (mx != 100.0f || my != 200.0f) {
    printf("FAILED: Mouse position (expected 100,200, got %f,%f)\\n", mx, my);
    input_shutdown(input);
    platform_window_destroy(window);
    platform_shutdown(platform);
    return 1;
  }
  printf("PASSED: Mouse position tracking\\n");

  // Test 9: Input Update
  input_update(input, 0.016f);
  printf("PASSED: Input update\\n");

  // Test 10: Cleanup
  input_shutdown(input);
  platform_window_destroy(window);
  platform_shutdown(platform);
  printf("PASSED: Cleanup complete\\n");

  logger_shutdown();
  // memory_shutdown();

  printf("\\nPlatform Verification Successful!\\n");
  return 0;
}
