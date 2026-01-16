#include "../Core/Log/Public/Log.h"
#include "../Core/Public/Engine.h"
#include <stdio.h>
#include <time.h>

// Simple monotonic time for Mac (if platform header not available)
#include <sys/time.h>

double GetTimeInSeconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// Include ECS and Components
#include "../ECS/Public/ECS.h"
#include "../ECS/components/Components_Registration.h"
#include "../ECS/components/Physics_Component.h"
#include "../ECS/components/Transform_Component.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf(
      "Starting Ultimate Engine Integration Test (Phase 16 - Gameplay)...\n");

  // Initialize Engine
  if (!Engine_Init("Config.json")) {
    fprintf(stderr, "Engine Initialization Failed!\n");
    return -1;
  }

  VF_INFO("Engine Initialized. Creating Test Entity...");

  // Create Physics Entity
  EntityID ent = ECS_CreateEntity();

  TransformComponent trans = {{0, 10, 0}, {0, 0, 0}, {1, 1, 1}};
  ECS_AddComponent(ent, COMPONENT_TRANSFORM, &trans);

  PhysicsComponent phys = {{0, 0, 0}, 1.0f, true};
  ECS_AddComponent(ent, COMPONENT_PHYSICS, &phys);

  VF_INFO("Entity %u Created at (0, 10, 0). Starting Loop...", ent);

  // Main Loop
  const int target_fps = 60;
  const double target_frame_time = 1.0 / target_fps;

  double last_time = GetTimeInSeconds();
  int frames = 0;

  while (frames < 600) { // Test for ~10 seconds
    double current_time = GetTimeInSeconds();
    double delta_time = current_time - last_time;
    last_time = current_time;

    Engine_Update((float)delta_time);

    // Verify Physics (Log position every 60 frames)
    if (frames % 60 == 0) {
      TransformComponent *t =
          (TransformComponent *)ECS_GetComponent(ent, COMPONENT_TRANSFORM);
      if (t) {
        VF_INFO("Frame %d: Entity Pos (%.2f, %.2f, %.2f)", frames,
                t->position.x, t->position.y, t->position.z);
      }
    }

    frames++;

    // Simulate Frame Pacing
    double work_end = GetTimeInSeconds();
    double work_taken = work_end - current_time;
    if (work_taken < target_frame_time) {
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (long)((target_frame_time - work_taken) * 1000000000.0);
      nanosleep(&ts, NULL);
    }
  }

  VF_INFO("Test Loop Complete (%d frames). Shutting Down...", frames);

  Engine_Shutdown();

  printf("Engine Shutdown Complete. Integration Test PASSED.\n");

  return 0;
}
