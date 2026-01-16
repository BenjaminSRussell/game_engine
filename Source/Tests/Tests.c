#include "Tests.h"
#include "../Runtime/Core/Log/Public/Log.h"
#include "../Runtime/Core/Memory/Public/Memory.h"
#include "../Runtime/Core/Threading/sync/mutex.h"
#include "../Runtime/ECS/Public/ECS.h"
#include "../Runtime/ECS/components/Audio_Component.h"
#include "../Runtime/ECS/components/Physics_Component.h"
#include "../Runtime/ECS/components/Transform_Component.h"
#include "../Runtime/Threading/Public/Job_System.h"
#include <string.h>

// ============================================================================
// MEMORY TESTS
// ============================================================================
void Memory_Tests(int *passed, int *total) {
  printf("Running Suite: Memory_Tests\n");

  // TEST 1: Basic Allocation
  {
    printf("  Test: Memory_Allocate_Basic\n");
    void *ptr = Memory_Allocate(128, MEMORY_TAG_GAME);
    if (ptr) {
      memset(ptr, 0xAA, 128); // Write check
      Memory_Free(ptr, 128, MEMORY_TAG_GAME);
      printf("    PASSED\n");
      (*passed)++;
    } else {
      printf("    FAILED (Alloc returned NULL)\n");
    }
    (*total)++;
  }

  // TEST 2: Zero Allocation
  {
    printf("  Test: Memory_Allocate_Zero\n");
    void *ptr = Memory_Allocate(1024, MEMORY_TAG_GAME);
    if (ptr) {
      // Check if zeroed (default behavior of our allocator wrapper)
      bool is_zero = true;
      unsigned char *byte_ptr = (unsigned char *)ptr;
      for (int i = 0; i < 1024; i++) {
        if (byte_ptr[i] != 0) {
          is_zero = false;
          break;
        }
      }
      Memory_Free(ptr, 1024, MEMORY_TAG_GAME);

      if (is_zero) {
        printf("    PASSED\n");
        (*passed)++;
      } else {
        printf("    FAILED (Memory not zeroed)\n");
      }
    } else {
      printf("    FAILED (Alloc returned NULL)\n");
    }
    (*total)++;
  }

  // TEST 3: Arena Allocator (Stub verification since we don't have full arena
  // API exposed globally yet, but we can test the tag)
  {
    // TODO: Expose Arena Allocator tests when Arena API is fully modularized
  }
}

// ============================================================================
// THREADING TESTS
// ============================================================================
void Threading_Tests(int *passed, int *total) {
  printf("Running Suite: Threading_Tests\n");

  // TEST 1: Mutex Basic
  {
    printf("  Test: Mutex_LockUnlock\n");
    Mutex *mutex = mutex_create();
    if (mutex) {
      mutex_lock(mutex);
      mutex_unlock(mutex);
      mutex_destroy(mutex);
      printf("    PASSED\n");
      (*passed)++;
    } else {
      printf("    FAILED (Mutex create failed)\n");
    }
    (*total)++;
  }

  // TEST 2: Job System Init/Shutdown
  {
    printf("  Test: JobSystem_Lifecycle\n");
    if (JobSystem_Init()) {
      // Simple job
      // JobSystem_Schedule(NULL, NULL, JOB_PRIORITY_NORMAL); // Can't wait
      // easily yet
      JobSystem_Shutdown();
      printf("    PASSED\n");
      (*passed)++;
    } else {
      printf("    FAILED (JobSystem init failed)\n");
    }
    (*total)++;
  }
}

// ============================================================================
// ECS TESTS
// ============================================================================
// Forward declare component registration for tests
extern ComponentType COMPONENT_TRANSFORM;
extern ComponentType COMPONENT_PHYSICS;
extern ComponentType COMPONENT_AUDIO_SOURCE;
void Components_RegisterAll(void);

bool ECS_Lifecycle_Test(void) {
  TEST_ASSERT(ECS_Init());
  EntityID entity = ECS_CreateEntity();
  TEST_ASSERT(entity != 0 && ECS_IsEntityValid(entity));
  ECS_DestroyEntity(entity);
  TEST_ASSERT(!ECS_IsEntityValid(entity));
  ECS_Shutdown();
  return true;
}

bool ECS_Component_Test(void) {
  TEST_ASSERT(ECS_Init());
  Components_RegisterAll();

  EntityID entity = ECS_CreateEntity();
  TransformComponent tc = {0};
  tc.position.x = 10.0f;

  void *added = ECS_AddComponent(entity, COMPONENT_TRANSFORM, &tc);
  TEST_ASSERT(added != NULL);
  TEST_ASSERT(ECS_HasComponent(entity, COMPONENT_TRANSFORM));

  TransformComponent *retrieved =
      (TransformComponent *)ECS_GetComponent(entity, COMPONENT_TRANSFORM);
  TEST_ASSERT(retrieved != NULL);
  TEST_ASSERT(retrieved->position.x == 10.0f);

  ECS_Shutdown();
  return true;
}

void ECS_Tests(int *passed, int *total) {
  printf("Running Suite: ECS_Tests\n");
  TEST_CASE(ECS_Lifecycle_Test);
  TEST_CASE(ECS_Component_Test);
}

// ============================================================================
// STUBS FOR OTHER SUITES
// ============================================================================
void Core_Engine_Tests(int *passed, int *total) {
  printf("Running Suite: Core_Engine_Tests\n");

  // TEST 1: Log Initialization
  {
    printf("  Test: Log_Init\n");
    if (Log_Init()) {
      VF_INFO("Test Log Message"); // Check if this prints
      Log_Shutdown();
      printf("    PASSED\n");
      (*passed)++;
    } else {
      printf("    FAILED (Log_Init returned false)\n");
    }
    (*total)++;
  }
}
void Render_Tests(int *passed, int *total) {
  (void)passed;
  (void)total;
}
void PhysicsSystem_Update(float dt);

bool Physics_Gravity_Test(void) {
  TEST_ASSERT(ECS_Init());
  Components_RegisterAll();

  EntityID entity = ECS_CreateEntity();
  TransformComponent tc = {0};
  tc.position.y = 10.0f;
  ECS_AddComponent(entity, COMPONENT_TRANSFORM, &tc);

  PhysicsComponent pc = {0};
  pc.active = true;
  pc.mass = 1.0f;
  ECS_AddComponent(entity, COMPONENT_PHYSICS, &pc);

  // Step physics for 1 second (60 frames)
  for (int i = 0; i < 60; i++) {
    PhysicsSystem_Update(0.016f);
  }

  TransformComponent *t =
      (TransformComponent *)ECS_GetComponent(entity, COMPONENT_TRANSFORM);
  // Expected distance: 0.5 * g * t^2 = 0.5 * 9.81 * 1.0^2 = 4.905
  // New height: 10 - 4.905 = 5.095
  TEST_ASSERT(t->position.y < 9.9f); // Minimal check that it moved down

  ECS_Shutdown();
  return true;
}

void Physics_Tests(int *passed, int *total) {
  printf("Running Suite: Physics_Tests\n");
  TEST_CASE(Physics_Gravity_Test);
}

#include "../../Source/Runtime/Audio/Public/Audio.h"
#include "../../Source/Runtime/Audio/Public/AudioHelper.h"

bool Audio_Lifecycle_Test(void) {
  TEST_ASSERT(Audio_Init());
  Audio_Shutdown();
  return true;
}

bool Audio_Synthesis_Test(void) {
  TEST_ASSERT(Audio_Init());
  int buffer = Audio_GenerateTestSound(440, 0.1f);
  TEST_ASSERT(buffer != 0);
  Audio_Shutdown();
  return true;
}

void Audio_Tests(int *passed, int *total) {
  printf("Running Suite: Audio_Tests\n");
  TEST_CASE(Audio_Lifecycle_Test);
  TEST_CASE(Audio_Synthesis_Test);
}
void AI_Tests(int *passed, int *total) {
  (void)passed;
  (void)total;
}
void Animation_Tests(int *passed, int *total) {
  (void)passed;
  (void)total;
}
void Network_Tests(int *passed, int *total) {
  (void)passed;
  (void)total;
}
void UI_Tests(int *passed, int *total) {
  (void)passed;
  (void)total;
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  printf("Ultimate Game Engine Test Suite\n");
  printf("===============================\n\n");

  // Initialize Core Systems needed for tests
  // We manually init Memory here because tests might rely on it before
  // Engine_Init
  Memory_Init();

  int passed = 0;
  int total = 0;

  Core_Engine_Tests(&passed, &total);
  Memory_Tests(&passed, &total);
  Threading_Tests(&passed, &total);
  ECS_Tests(&passed, &total);
  Render_Tests(&passed, &total);
  Physics_Tests(&passed, &total);
  AI_Tests(&passed, &total);
  Audio_Tests(&passed, &total);
  Animation_Tests(&passed, &total);
  Network_Tests(&passed, &total);
  UI_Tests(&passed, &total);

  printf("\n===========================\n");
  printf("Tests Passed: %d/%d\n", passed, total);

  Memory_Shutdown();

  if (total > 0)
    printf("Success Rate: %.1f%%\n", (float)passed / total * 100.0f);
  else
    printf("No tests run.\n");

  return (passed == total && total > 0) ? 0 : -1;
}
