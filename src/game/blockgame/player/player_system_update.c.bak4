// Per-frame player update and interactions.
// TODO: Implement player update optimization system.
// TODO: Add player update validation system.
// TODO: Implement player update statistics tracking.
// TODO: Add player update performance monitoring.
// TODO: Implement player update debugging tools.
// TODO: Add player update unit testing framework.
// TODO: Implement player update documentation system.
// TODO: Add player update profiling system.
// TODO: Implement player update error recovery.
// TODO: Add player update thread-safety improvements.
#include <player/player.h>

// Main player system update function
void player_system_update(PlayerSystem *system, f32 delta_time,
                          ChunkManager *chunk_manager,
                          PhysicsWorld *physics_world,
                          BlockRegistry *block_registry) {
  if (!system) {
    return;
  }

  if (chunk_manager) {
    system->chunk_manager = chunk_manager;
  }
  if (physics_world) {
    system->physics_world = physics_world;
  }
  if (block_registry) {
    system->block_registry = block_registry;
  }

  player_update(system, delta_time);
}
