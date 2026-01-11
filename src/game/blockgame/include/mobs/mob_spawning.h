/**
 * Mob Spawning System - Biome-specific spawn rules and automatic despawn mechanics
 *
 * Purpose: Manage automatic mob spawning based on biome type, light levels, and spawn caps.
 * Also handles automatic despawning when mobs are too far from the player or have been idle too long.
 *
 * Integration Points:
 * - Uses BiomeType from biome_features.h
 * - Uses MobType and MobManager from mob_system.h
 * - Uses ChunkManager for chunk queries
 * - Uses WorldGenerator for biome lookups
 * - Uses PlayerSystem for player position
 */

#ifndef MOB_SPAWNING_H
#define MOB_SPAWNING_H

#include "../game_common.h"
#include "../mobs/mob_system.h"
#include "../world/biome_features.h"
#include "../chunk/chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Spawn Configuration Constants
// ============================================================================

// Spawn caps (prevent lag from too many mobs)
#define MAX_MOBS_PER_CHUNK 10
#define MAX_HOSTILE_PER_CHUNK 5
#define MAX_PASSIVE_PER_CHUNK 5

// Despawn thresholds
#define DESPAWN_DISTANCE 128.0f      // Blocks - mobs despawn if >128 blocks from player
#define DESPAWN_TIMEOUT 300.0f       // Seconds - mobs despawn if idle for 5 minutes

// Light level thresholds (BlockGame standard)
#define LIGHT_LEVEL_DARK_THRESHOLD 7        // Light <= 7 = dark (hostile spawn)
#define LIGHT_LEVEL_BRIGHT_THRESHOLD 9      // Light >= 9 = bright (passive spawn)

// Spawn attempt frequency
#define SPAWN_ATTEMPT_INTERVAL 1.0f  // Attempt spawns once per second

// ============================================================================
// Biome Spawn Configuration
// ============================================================================

/**
 * Single spawn entry for a mob type in a biome
 * Defines the mob type, spawn probability weight, and light requirements
 */
typedef struct {
    MobType mob_type;           // Which mob to spawn
    f32 spawn_weight;           // Relative probability (higher = more likely)
    u8 min_light;               // Minimum light level required (0-15)
    u8 max_light;               // Maximum light level required (0-15)
    u8 min_group_size;          // Minimum mobs to spawn together
    u8 max_group_size;          // Maximum mobs to spawn together
} BiomeSpawnEntry;

/**
 * Complete spawn table for one biome type
 * Contains all possible mobs that can spawn in this biome
 */
typedef struct {
    BiomeType biome;            // Which biome this table applies to
    BiomeSpawnEntry *entries;   // Array of possible spawns
    u32 entry_count;            // Number of entries
    f32 spawn_rate_modifier;    // Biome-specific spawn rate (e.g., 0.8 = 20% slower)
} BiomeSpawnTable;

// ============================================================================
// Mob Spawner System
// ============================================================================

/**
 * Main spawn system manager
 * Manages all aspects of mob spawning and despawning
 */
typedef struct {
    // References to game systems
    MobManager *mob_manager;            // Where to spawn/despawn mobs
    struct ChunkManager *chunk_manager; // For chunk queries
    struct WorldGenerator *world_gen;   // For biome lookups
    struct PlayerSystem *player_system; // For player position

    // Spawn tables (one per biome)
    BiomeSpawnTable *spawn_tables;
    u32 spawn_table_count;

    // Spawn timing
    f32 spawn_timer;           // Time since last spawn attempt
    f32 spawn_interval;        // Seconds between spawn attempts (usually 1.0)

    // System state
    bool enabled;              // Whether spawning is active
} MobSpawner;

// ============================================================================
// Initialization and Cleanup
// ============================================================================

/**
 * Initialize the mob spawner system
 * Sets up spawn tables and connects to game systems
 */
void mob_spawner_init(MobSpawner *spawner,
                     MobManager *mobs,
                     struct ChunkManager *chunks,
                     struct WorldGenerator *gen,
                     struct PlayerSystem *player);

/**
 * Free all resources used by the spawner
 */
void mob_spawner_free(MobSpawner *spawner);

// ============================================================================
// Main Update Function
// ============================================================================

/**
 * Update the spawn system each frame
 * Called from main game loop with delta_time
 * Internally throttles spawn attempts to SPAWN_ATTEMPT_INTERVAL
 */
void mob_spawner_update(MobSpawner *spawner, f32 delta_time);

// ============================================================================
// Spawn System Functions
// ============================================================================

/**
 * Attempt to spawn mobs in chunks near the player
 * Checks spawn caps, light levels, and biome spawn tables
 * Called once per second by mob_spawner_update()
 */
void mob_spawner_attempt_spawn(MobSpawner *spawner);

/**
 * Try to spawn a specific mob at a position
 * Checks light level against spawn table requirements
 *
 * @param spawner Spawn system
 * @param position World position to spawn at
 * @param biome Biome type at this position
 * @return true if spawn was attempted, false if rejected (caps or light level)
 */
bool mob_spawner_try_spawn_at(MobSpawner *spawner, Vec3 position, BiomeType biome);

// ============================================================================
// Despawn System Functions
// ============================================================================

/**
 * Despawn mobs that are too far from the player
 * Called once per second by mob_spawner_update()
 * Mobs are despawned if distance > DESPAWN_DISTANCE (128 blocks)
 */
void mob_spawner_despawn_distant(MobSpawner *spawner);

/**
 * Despawn mobs that have been idle for too long
 * Called once per second by mob_spawner_update()
 * Only affects mobs in IDLE or WANDERING states
 * Mobs are despawned if idle time > DESPAWN_TIMEOUT (300 seconds)
 */
void mob_spawner_despawn_old(MobSpawner *spawner);

// ============================================================================
// Spawn Table Query Functions
// ============================================================================

/**
 * Get the spawn table for a specific biome
 *
 * @param spawner Spawn system
 * @param biome Biome to query
 * @return Spawn table for biome, or NULL if biome not configured
 */
BiomeSpawnTable *mob_spawner_get_table_for_biome(MobSpawner *spawner, BiomeType biome);

/**
 * Select a random mob type from biome spawn table based on light level
 * Uses weighted random selection based on spawn_weight
 *
 * @param spawner Spawn system
 * @param biome Biome type
 * @param light_level Current light level (0-15)
 * @return Selected mob type (MOB_TYPE_ZOMBIE if no valid options)
 */
MobType mob_spawner_select_mob_for_biome(MobSpawner *spawner, BiomeType biome, u8 light_level);

// ============================================================================
// Spawn Cap Functions
// ============================================================================

/**
 * Check if we can spawn more mobs in a chunk
 * Considers MAX_MOBS_PER_CHUNK, MAX_HOSTILE_PER_CHUNK, MAX_PASSIVE_PER_CHUNK
 *
 * @param spawner Spawn system
 * @param chunk Chunk to check
 * @return true if we can spawn at least one more mob in this chunk
 */
bool mob_spawner_can_spawn_in_chunk(MobSpawner *spawner, Chunk *chunk);

/**
 * Count how many mobs are in a specific chunk
 * Counts all mobs within chunk boundaries
 *
 * @param spawner Spawn system
 * @param chunk Chunk to count
 * @return Number of mobs in chunk
 */
u32 mob_spawner_count_mobs_in_chunk(MobSpawner *spawner, Chunk *chunk);

/**
 * Count how many hostile mobs are in a chunk
 *
 * @param spawner Spawn system
 * @param chunk Chunk to count
 * @return Number of hostile mobs in chunk
 */
u32 mob_spawner_count_hostile_in_chunk(MobSpawner *spawner, Chunk *chunk);

/**
 * Count how many passive mobs are in a chunk
 *
 * @param spawner Spawn system
 * @param chunk Chunk to count
 * @return Number of passive mobs in chunk
 */
u32 mob_spawner_count_passive_in_chunk(MobSpawner *spawner, Chunk *chunk);

// ============================================================================
// Light Level Check Functions
// ============================================================================

/**
 * Check if a position has sufficient light for passive mob spawning
 * Light level >= LIGHT_LEVEL_BRIGHT_THRESHOLD (9)
 *
 * @param chunk Chunk containing position
 * @param x Block X coordinate within chunk
 * @param y Block Y coordinate
 * @param z Block Z coordinate within chunk
 * @return true if light level is sufficient for passive mobs
 */
bool mob_spawner_has_sufficient_light(Chunk *chunk, i32 x, i32 y, i32 z);

/**
 * Check if a position is dark enough for hostile mob spawning
 * Light level <= LIGHT_LEVEL_DARK_THRESHOLD (7)
 *
 * @param chunk Chunk containing position
 * @param x Block X coordinate within chunk
 * @param y Block Y coordinate
 * @param z Block Z coordinate within chunk
 * @return true if light level is dark enough for hostile mobs
 */
bool mob_spawner_is_dark_enough(Chunk *chunk, i32 x, i32 y, i32 z);

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * Enable or disable the spawn system
 * When disabled, no mobs will spawn
 */
void mob_spawner_set_enabled(MobSpawner *spawner, bool enabled);

/**
 * Get whether the spawn system is currently enabled
 */
bool mob_spawner_is_enabled(MobSpawner *spawner);

#ifdef __cplusplus
}
#endif

#endif // MOB_SPAWNING_H
