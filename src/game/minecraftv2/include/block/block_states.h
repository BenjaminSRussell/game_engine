// include/block/block_states.h
#ifndef BLOCK_STATES_H
#define BLOCK_STATES_H

#include "../chunk/chunk.h"
#include "../game_common.h"

// Forward declarations
typedef struct ChunkManager ChunkManager;
typedef struct PhysicsWorld PhysicsWorld;
typedef struct BlockRegistry BlockRegistry;
struct World;

// Block state functions
void block_update_water_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z);
void block_update_lava_flow(ChunkManager *chunk_manager, i32 x, i32 y, i32 z);
void block_update_falling(ChunkManager *chunk_manager,
                          BlockRegistry *block_registry, struct World *world,
                          PhysicsWorld *physics_world, i32 x, i32 y, i32 z);

// Manager functions
void block_state_manager_init(BlockStateManager *manager, u32 capacity);
void block_state_manager_free(BlockStateManager *manager);
void block_state_manager_update(BlockStateManager *manager,
                                ChunkManager *chunk_manager, f32 delta_time);
void block_state_manager_add(BlockStateManager *manager, i32 x, i32 y, i32 z,
                             BlockID block_id);

#endif // BLOCK_STATES_H
