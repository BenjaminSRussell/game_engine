// include/ai/ai_types.h
// Purpose: Core AI types and structures for behavior systems

#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "../../common.h"
#include "../../math/vec3.h"

// AI Entity ID
typedef u32 AIEntityID;
#define AI_NULL_ENTITY 0

// World State for GOAP
typedef struct {
    u32 state_mask;     // Bitmask for atomic state
    u32 hash_value;     // Quick comparison hash
} WorldState;

// GOAP Action structure
typedef struct {
    const char* name;
    WorldState preconditions;
    WorldState effects;
    f32 cost;
    bool (*proc_check)(AIEntityID entity);
    void (*execute)(AIEntityID entity);
} GoapAction;

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
