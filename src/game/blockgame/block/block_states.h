/**
 * @file block_states.h
 * @brief Block state machine for the block game.
 *
 * A block cell occupies one of a small number of states. State changes are
 * driven by discrete events (place, break, flood, plant, harvest) and are
 * only allowed along a fixed set of valid transitions; anything else leaves
 * the state unchanged.
 */

#ifndef BLOCKGAME_BLOCK_BLOCK_STATES_H
#define BLOCKGAME_BLOCK_BLOCK_STATES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief States a block cell can occupy.
 */
typedef enum BlockState {
    BLOCK_STATE_EMPTY = 0, /**< No block present. */
    BLOCK_STATE_SOLID,     /**< Opaque, player-placed block. */
    BLOCK_STATE_LIQUID,    /**< Flowing liquid (water). */
    BLOCK_STATE_PLANT,     /**< Growing plant; harvestable. */
    BLOCK_STATE_COUNT
} BlockState;

/**
 * @brief Events that may cause a block state change.
 */
typedef enum BlockEvent {
    BLOCK_EVENT_PLACE = 0, /**< Player places a solid block. */
    BLOCK_EVENT_BREAK,     /**< Player breaks the block. */
    BLOCK_EVENT_FLOOD,     /**< Liquid flows into the cell. */
    BLOCK_EVENT_PLANT,     /**< A plant is planted in the cell. */
    BLOCK_EVENT_HARVEST,   /**< A plant is harvested. */
    BLOCK_EVENT_COUNT
} BlockEvent;

/**
 * @brief Check whether an event is a valid transition from `from`.
 *
 * @param from    Current state of the cell.
 * @param event   Event to apply.
 * @param out_to  If non-NULL, receives the resulting state when the
 *                transition is valid.
 * @return true if the transition is valid, false otherwise.
 */
bool block_state_can_transition(BlockState from, BlockEvent event,
                                BlockState *out_to);

/**
 * @brief Apply an event to a state.
 *
 * Valid transitions move to the new state; invalid transitions leave the
 * state unchanged.
 *
 * @param from   Current state of the cell.
 * @param event  Event to apply.
 * @return The state after the event (unchanged if the event is invalid).
 */
BlockState block_state_apply(BlockState from, BlockEvent event);

/**
 * @brief Human-readable name of a state, for logging and tests.
 *
 * @param state State to name (out-of-range values yield "UNKNOWN").
 * @return Non-NULL string.
 */
const char *block_state_name(BlockState state);

/**
 * @brief Human-readable name of an event, for logging and tests.
 *
 * @param event Event to name (out-of-range values yield "UNKNOWN").
 * @return Non-NULL string.
 */
const char *block_event_name(BlockEvent event);

#ifdef __cplusplus
}
#endif

#endif /* BLOCKGAME_BLOCK_BLOCK_STATES_H */
