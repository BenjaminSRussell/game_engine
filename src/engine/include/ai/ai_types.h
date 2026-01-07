// include/ai/ai_types.h
// Purpose: Core AI types and structures for behavior systems

#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "include/common.h"
#include "include/math/vec3.h"

// AI Entity ID
typedef u32 AIEntityID;
#define AI_NULL_ENTITY 0

// World State for GOAP
// GOAP State (Bitmask)
typedef u64 GOAPState;
// WorldState is deprecated/alias
typedef GOAPState WorldState;

// Note: GoapAction is defined in goap_planner.h

// Cover Point structure
typedef struct {
    Vec3 position;
    Vec3 normal;
    enum {
        COVER_LOW,
        COVER_HIGH
    } type;
    bool is_occupied;
    AIEntityID occupant;
} CoverPoint;

// EQS Query context
typedef struct {
    AIEntityID querier;
    AIEntityID target;
    Vec3 position;
} EQSContext;

// EQS Candidate point
typedef struct {
    Vec3 position;
    f32 score;
    bool is_valid;
} EQSCandidate;

#endif // AI_TYPES_H
