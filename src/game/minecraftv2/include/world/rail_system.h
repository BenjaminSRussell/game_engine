// include/world/rail_system.h
//
// Purpose: Defines the public API and data structures for implementing a comprehensive
// rail system within the game world. This header provides the necessary components
// for creating, managing, and interacting with various types of railway tracks,
// facilitating the movement of minecarts and trains. It includes definitions for
// different rail types, individual rail segments, and a network structure to link them.
//
// Public APIs:
// - `RailType`: Enumeration defining various configurations for rail segments
//   (e.g., `RAIL_STRAIGHT_NS`, `RAIL_CURVE_NE`, `RAIL_ASCENDING_N`).
// - `RailSegment`: Structure representing a single piece of track, including its
//   world coordinates (`x`, `y`, `z`), `type`, and `powered` status.
// - `RailNetwork`: The main structure managing a collection of `RailSegment`s,
//   tracking their count and capacity.
// - `rail_system_init`: Initializes the rail network with a specified maximum number of segments.
// - `rail_system_free`: Frees all resources held by the rail network.
// - `rail_place_segment`: Places a rail segment of a specified type at given world coordinates.
// - `rail_generate_network`: Automatically generates a railway track path between two points in the world.
// - `rail_find_segment`: Locates a `RailSegment` at a specific world position.
// - `rail_get_connected`: Retrieves all rail segments directly connected to a given segment.
//
// Ownership: The `RailNetwork` owns the collection of `RailSegment`s it manages.
//
// Invariants:
// - A `RailNetwork` must be initialized with `rail_system_init` before use.
// - `ChunkManager` (from `chunk.h`) is assumed to be available for modifying the world's blocks.
// - Coordinates (`x`, `y`, `z`) must be valid world coordinates.
// - `RailType` values are crucial for determining track geometry and connections.
//
#ifndef RAIL_SYSTEM_H
#define RAIL_SYSTEM_H


#include "../game_common.h"
#include "../chunk/chunk.h"

// Rail types (must be defined first)
typedef enum {
    RAIL_STRAIGHT_NS = 0,
    RAIL_STRAIGHT_EW = 1,
    RAIL_CURVE_NE = 2,
    RAIL_CURVE_NW = 3,
    RAIL_CURVE_SE = 4,
    RAIL_CURVE_SW = 5,
    RAIL_ASCENDING_N = 6,
    RAIL_ASCENDING_S = 7,
    RAIL_ASCENDING_E = 8,
    RAIL_ASCENDING_W = 9,
    RAIL_COUNT = 10
} RailType;

// Rail segment
typedef struct RailSegment {
    i32 x, y, z;
    RailType type;
    bool powered;
} RailSegment;

// Rail network
typedef struct RailNetwork {
    RailSegment *segments;
    u32 segment_count;
    u32 segment_capacity;
} RailNetwork;

// Initialize rail system
void rail_system_init(RailNetwork *network, u32 max_segments);

// Place rail segment
bool rail_place_segment(ChunkManager *chunks, i32 x, i32 y, i32 z, RailType type, bool powered);

// Generate rail network between two points
bool rail_generate_network(ChunkManager *chunks, i32 start_x, i32 start_y, i32 start_z,
                          i32 end_x, i32 end_y, i32 end_z);

// Find rail segment at position
RailSegment *rail_find_segment(RailNetwork *network, i32 x, i32 y, i32 z);

// Get connected rails
u32 rail_get_connected(RailNetwork *network, i32 x, i32 y, i32 z, RailSegment **out_segments, u32 max_count);

// Free rail system
void rail_system_free(RailNetwork *network);

#endif // RAIL_SYSTEM_H
