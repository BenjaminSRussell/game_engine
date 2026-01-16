#include "Public/Engine.h"
#include "Public/core_types.h"

// Core Systems
#include "../../Memory/Public/Memory.h"
#include "../../Platform/Public/Platform.h"
#include "../../Threading/Public/Job_System.h"
#include "Log/Public/Log.h"

// Engine Systems
#include "../../AI/Public/AI.h"
#include "../../Asset/Public/Asset.h"
#include "../../Audio/Public/Audio.h"
#include "../../ECS/Public/ECS.h"
#include "../../Input/Public/Input.h"
#include "../../Networking/Public/Networking.h"
#include "../../Physics/Public/Physics.h"
#include "../../Render/Public/Render.h"
#include "../../UI/Public/UI.h"

// Gameplay Systems Registration
#include "../../ECS/components/Components_Registration.h"
#include "../../ECS/systems/Physics_System.h"

// Threading Wrapper (since we named it JobSystem)
bool Threading_Init(void) { return JobSystem_Init(); }
void Threading_Shutdown(void) { JobSystem_Shutdown(); }

typedef struct {
  bool initialized;
  bool running;
  float delta_time;
  double total_time;
  u64 frame_count;
} EngineState;

static EngineState g_engine = {0};

bool Engine_Init(const char *config_path) {
  // Phase 1: Initialize core systems
  if (!Memory_Init())
    return false;
  if (!Log_Init())
    return false;
  if (!Threading_Init())
    return false;

  // Phase 2: Initialize engine systems
  if (!Platform_Init())
    return false;
  if (!ECS_Init())
    return false;

  // Gameplay Registration
  Components_RegisterAll();
  PhysicsSystem_Register();

  // Phase 3: Initialize game systems
  if (!Input_Init())
    return false;
  if (!Render_Init())
    return false;
  if (!Physics_Init())
    return false;
  if (!Audio_Init())
    return false;
  if (!AI_Init())
    return false;
  if (!Networking_Init())
    return false;
  if (!UI_Init())
    return false;
  if (!Asset_Init())
    return false;

  g_engine.initialized = true;
  g_engine.running = true;
  return true;
}

void Engine_Update(float delta_time) {
  g_engine.delta_time = delta_time;
  g_engine.total_time += delta_time;
  g_engine.frame_count++;

  // Update core systems
  Platform_Update(delta_time);
  Input_Update(delta_time);

  // Update game logic
  Networking_Update(delta_time);
  Physics_Update(delta_time); // Legacy/Low-level physics update (steps solver)
  AI_Update(delta_time);
  ECS_Update(delta_time); // Runs ECS Systems (PhysicsSystem)
  UI_Update(delta_time);

  // Render frame
  Render_BeginFrame();
  ECS_Render();
  UI_Render();
  Render_EndFrame();
}

void Engine_Shutdown(void) {
  // Shutdown in reverse order
  Asset_Shutdown();
  UI_Shutdown();
  Networking_Shutdown();
  AI_Shutdown();
  Audio_Shutdown();
  Render_Shutdown();
  Physics_Shutdown();
  Input_Shutdown();
  ECS_Shutdown();
  Platform_Shutdown();
  Threading_Shutdown();
  Log_Shutdown();
  Memory_Shutdown();

  g_engine.initialized = false;
}
