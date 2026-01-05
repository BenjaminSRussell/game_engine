#include <core/logger.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdlib.h>

// Minimal type definitions to satisfy linker without full headers
typedef struct PhysicsWorld PhysicsWorld;
typedef struct Collider Collider;
typedef struct CombatSystem CombatSystem;
typedef struct NPCSystem NPCSystem;
typedef struct World World;
typedef struct Entity {
  unsigned int id;
} Entity;
// Vec3 defined in math/math.h via input.h
typedef struct BlockState {
  int id;
} BlockState;
typedef struct ChunkManager ChunkManager;
typedef struct GameConfig GameConfig;

// Round 3 Stubs



bool combat_ranged_attack(CombatSystem *system, struct World *ecs,
                          Entity caster, Vec3 target_pos) {
  (void)system;
  (void)ecs;
  (void)caster;
  (void)target_pos;
  LOG_INFO("combat_ranged_attack stub called");
  return true;
}

void combat_system_bind_world(CombatSystem *combat_system, World *world) {
  (void)combat_system;
  (void)world;
}

void combat_system_init(CombatSystem *system) {
  (void)system;
  LOG_INFO("combat_system_init stub called");
}

void combat_system_update(CombatSystem *system, float delta_time) {
  (void)system;
  (void)delta_time;
}

static bool stub_input_init(InputSystem *sys, const InputConfig *cfg) {
  LOG_INFO("Input System Stub Initialized");
  return true;
}
static void stub_input_shutdown(InputSystem *sys) {}
static void stub_input_update(InputSystem *sys, f32 dt) {}

// create_glfw_input_system implemented in input_factory.c

void dialogue_manager_update(NPCSystem *system, float delta_time) {
  (void)system;
  (void)delta_time;
}

// Round 4 Stubs

void *asset_importer_load_audio(const char *path) {
  (void)path;
  LOG_INFO("asset_importer_load_audio stub called");
  return NULL;
}

void brewing_stand_update(void *state, void *inventory, float delta_time) {
  (void)state;
  (void)inventory;
  (void)delta_time;
}

BlockState chunk_manager_get_block(ChunkManager *manager, int x, int y, int z) {
  (void)manager;
  (void)x;
  (void)y;
  (void)z;
  // LOG_INFO("chunk_manager_get_block stub called"); // Too spammy
  return (BlockState){0};
}

const GameConfig *engine_get_config(void) {
  static int dummy = 0;
  return (const GameConfig *)&dummy;
}
