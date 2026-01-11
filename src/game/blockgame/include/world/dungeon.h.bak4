// include/world/dungeon.h
//
// Purpose: Defines the public API and data structures for a comprehensive dungeon
// generation system. This header provides the necessary components to procedurally
// create complex underground structures, including various `DungeonType`s and
// individual `DungeonRoom`s. It supports generating entire dungeons, placing them
// within the game world (chunks), and populating them with features like enemies,
// treasure, and traps.
//
// Public APIs:
// - `DungeonType`: Enumeration defining different types of dungeons (e.g., Cave, Mineshaft, Fortress).
// - `RoomType`: Enumeration defining various types of rooms within a dungeon (e.g., Entrance, Corridor, Treasure, Boss).
// - `DungeonRoom`: Structure representing a single room within a dungeon, including its `type`,
//   `position`, `size`, generation status, and flags for content (enemies, treasure, traps).
// - `Dungeon`: The main structure encapsulating an entire dungeon, detailing its `type`,
//   `position`, `size`, collection of `DungeonRoom`s, generation status, and seed.
// - `DungeonGenerator`: Structure managing multiple dungeons, including a seed for generation.
// - `dungeon_generator_init`: Initializes the dungeon generator with a global seed.
// - `dungeon_generator_free`: Frees resources held by the dungeon generator.
// - `dungeon_generate`: Generates a new `Dungeon` instance of a specified type at a given position.
// - `dungeon_place_in_chunk`: Integrates a generated dungeon's structure into a game `Chunk`.
// - `dungeon_generate_room`: Generates specific features and blocks within a `DungeonRoom`.
// - `dungeon_generate_corridor`: Generates a connecting corridor between two points.
// - Dungeon features: `dungeon_add_chest`, `dungeon_add_spawner`, `dungeon_add_trap` for populating dungeons.
//
// Ownership: The `DungeonGenerator` owns the collection of `Dungeon` instances it creates.
// Each `Dungeon` in turn owns its `DungeonRoom` array.
//
// Invariants:
// - A `DungeonGenerator` must be initialized before generating dungeons.
// - `Vec3` and `Chunk` structures (from `vec3.h` and `chunk.h`) are assumed to be correctly defined.
// - Dungeon generation relies on the provided seed for reproducibility.
// - Coordinates (`position`, `size`) are typically in world-space or relative to the dungeon's origin.
//
#ifndef DUNGEON_H
#define DUNGEON_H


#include "../game_common.h"
#include "../chunk/chunk.h"
#include <math/vec3.h>

// Dungeon types
typedef enum {
    DUNGEON_TYPE_CAVE,
    DUNGEON_TYPE_MINESHAFT,
    DUNGEON_TYPE_RUINS,
    DUNGEON_TYPE_FORTRESS,
    DUNGEON_TYPE_TEMPLE,
    DUNGEON_TYPE_CRYPT,
    DUNGEON_TYPE_COUNT
} DungeonType;

// Room types
typedef enum {
    ROOM_TYPE_ENTRANCE,
    ROOM_TYPE_CORRIDOR,
    ROOM_TYPE_CHAMBER,
    ROOM_TYPE_TREASURE,
    ROOM_TYPE_BOSS,
    ROOM_TYPE_TRAP,
    ROOM_TYPE_COUNT
} RoomType;

// Dungeon room
typedef struct {
    RoomType type;
    Vec3 position;
    Vec3 size;
    bool generated;
    bool has_enemies;
    bool has_treasure;
    bool has_traps;
    u32 enemy_count;
} DungeonRoom;

// Dungeon structure
typedef struct {
    DungeonType type;
    Vec3 position;
    Vec3 size;
    DungeonRoom *rooms;
    u32 room_count;
    u32 room_capacity;
    bool generated;
    u32 seed;
} Dungeon;

// Dungeon generator
typedef struct {
    u32 seed;
    Dungeon *dungeons;
    u32 count;
    u32 capacity;
} DungeonGenerator;

// Initialize dungeon generator
void dungeon_generator_init(DungeonGenerator *gen, u32 seed);
void dungeon_generator_free(DungeonGenerator *gen);

// Generate dungeon
Dungeon *dungeon_generate(DungeonGenerator *gen, DungeonType type, Vec3 position, u32 seed);

// Place dungeon in world
void dungeon_place_in_chunk(Dungeon *dungeon, Chunk *chunk);

// Dungeon room generation
void dungeon_generate_room(Dungeon *dungeon, DungeonRoom *room, Chunk *chunk);
void dungeon_generate_corridor(Dungeon *dungeon, Vec3 start, Vec3 end, Chunk *chunk);

// Dungeon features
void dungeon_add_chest(Dungeon *dungeon, Vec3 position, Chunk *chunk);
void dungeon_add_spawner(Dungeon *dungeon, Vec3 position, Chunk *chunk);
void dungeon_add_trap(Dungeon *dungeon, Vec3 position, Chunk *chunk);

#endif // DUNGEON_H

