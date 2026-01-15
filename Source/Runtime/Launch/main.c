#include "../../Core/Initialization/init_pipeline.h"
#include "../../Core/Platform/platform_bootstrap.h"
#include "../../Engine/engine_loop.h"
#include <stdio.h>

int main(int argc, char **argv) {
  // Initialize platform
  // Window creation is handled inside game_init -> init_window -> platform_init

  // Run initialization pipeline
  InitResult result = game_init();
  if (!result.success) {
    fprintf(stderr, "Game initialization failed: %s (Error code: %d)\n",
            result.message, result.error);
    return 1;
  }

  // Enter main loop
  engine_run();

  return 0;
}
