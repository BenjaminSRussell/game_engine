#include "ai/npc_advanced/goap_enhanced.h"
#include <core/logger.h>
#include <core/memory.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                    ENHANCED GOAP SYSTEM - AGENT_AI_1 (Phase 5-8)
 * =================================================================================================
 *
 * PURPOSE: High-performance planning with bitfield-based world state.
 *
 * OPTIMIZATIONS:
 * - O(1) state operations using 64-bit bitfields
 * - O(1) state equality and difference checks
 * - Improved A* heuristic using Hamming distance
 * - Plan validation and incremental replanning
 * =================================================================================================
 */

// ===========================================================================================
// PHASE 5: OPTIMIZED WORLD STATE OPERATIONS
// ===========================================================================================

// Set a single atom (O(1))
void goap_state_set_atom(GoapWorldStateEnhanced *state, GoapAtomID atom,
                         bool value) {
  if (!state || atom >= GOAP_MAX_STATE_ATOMS)
    return;

  if (value) {
    state->bits.bits |= (1ULL << atom); // Set bit
  } else {
    state->bits.bits &= ~(1ULL << atom); // Clear bit
  }

  // Invalidate hash
  state->bits.state_hash = 0;
}

// Get a single atom (O(1))
bool goap_state_get_atom(const GoapWorldStateEnhanced *state, GoapAtomID atom) {
  if (!state || atom >= GOAP_MAX_STATE_ATOMS)
    return false;
  return (state->bits.bits & (1ULL << atom)) != 0;
}

// Clear all atoms
void goap_state_clear(GoapWorldStateEnhanced *state) {
  if (!state)
    return;
  memset(state, 0, sizeof(GoapWorldStateEnhanced));
}

// Copy state
void goap_state_copy(GoapWorldStateEnhanced *dest,
                     const GoapWorldStateEnhanced *src) {
  if (!dest || !src)
    return;
  memcpy(dest, src, sizeof(GoapWorldStateEnhanced));
}

// Set multiple atoms at once
void goap_state_set_multiple(GoapWorldStateEnhanced *state,
                             const GoapAtomID *atoms, const bool *values,
                             int count) {
  if (!state || !atoms || !values)
    return;

  for (int i = 0; i < count; i++) {
    goap_state_set_atom(state, atoms[i], values[i]);
  }
}

// Get raw bitfield
u64 goap_state_get_bits(const GoapWorldStateEnhanced *state) {
  return state ? state->bits.bits : 0;
}

// Set raw bitfield
void goap_state_set_bits(GoapWorldStateEnhanced *state, u64 bits) {
  if (!state)
    return;
  state->bits.bits = bits;
  state->bits.state_hash = 0; // Invalidate hash
}

// Check exact equality (O(1))
bool goap_state_equals(const GoapWorldStateEnhanced *a,
                       const GoapWorldStateEnhanced *b) {
  if (!a || !b)
    return false;

  // Fast path: compare bitfields
  if (a->bits.bits != b->bits.bits)
    return false;

  // Extended data comparison (rare case)
  if (a->extended_count != b->extended_count)
    return false;

  for (int i = 0; i < a->extended_count; i++) {
    bool found = false;
    for (int j = 0; j < b->extended_count; j++) {
      if (a->extended_data[i].atom_id == b->extended_data[j].atom_id) {
        if (a->extended_data[i].type != b->extended_data[j].type)
          return false;

        switch (a->extended_data[i].type) {
        case GOAP_STATE_INT:
          if (a->extended_data[i].value.int_val !=
              b->extended_data[j].value.int_val)
            return false;
          break;
        case GOAP_STATE_FLOAT:
          if (fabsf(a->extended_data[i].value.float_val -
                    b->extended_data[j].value.float_val) > 0.001f)
            return false;
          break;
        default:
          break;
        }
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  return true;
}

// Count bit differences (Hamming distance) - O(1)
int goap_state_count_differences(const GoapWorldStateEnhanced *a,
                                 const GoapWorldStateEnhanced *b) {
  if (!a || !b)
    return 64;

  u64 xor_bits = a->bits.bits ^ b->bits.bits;

  // Count set bits (population count)
  int count = 0;
  while (xor_bits) {
    count += xor_bits & 1;
    xor_bits >>= 1;
  }

  return count;
}

// Compute detailed difference
void goap_state_diff(const GoapWorldStateEnhanced *from,
                     const GoapWorldStateEnhanced *to, GoapStateDiff *diff) {
  if (!from || !to || !diff)
    return;

  memset(diff, 0, sizeof(GoapStateDiff));

  diff->added_bits =
      to->bits.bits & ~from->bits.bits; // Bits in 'to' but not 'from'
  diff->removed_bits =
      from->bits.bits & ~to->bits.bits; // Bits in 'from' but not 'to'
  diff->changed_bits = diff->added_bits | diff->removed_bits;

  // Count changes
  u64 changes = diff->changed_bits;
  while (changes) {
    diff->change_count += changes & 1;
    changes >>= 1;
  }
}

// Check if state satisfies requirements (ALL atoms must match)
bool goap_state_satisfies(const GoapWorldStateEnhanced *state,
                          const GoapWorldStateEnhanced *requirements) {
  if (!state || !requirements)
    return false;

  // Check if all required bits are set
  u64 required_bits = requirements->bits.bits;
  return (state->bits.bits & required_bits) == required_bits;
}

// Check if state satisfies ANY requirements
bool goap_state_satisfies_any(const GoapWorldStateEnhanced *state,
                              const GoapWorldStateEnhanced *requirements) {
  if (!state || !requirements)
    return false;

  // Check if ANY required bit is set
  return (state->bits.bits & requirements->bits.bits) != 0;
}

// Compute hash for state
u32 goap_state_compute_hash(const GoapWorldStateEnhanced *state) {
  if (!state)
    return 0;

  // Simple hash: XOR folding of 64-bit value
  u32 hash = (u32)(state->bits.bits ^ (state->bits.bits >> 32));

  // Mix in extended data
  for (int i = 0; i < state->extended_count; i++) {
    hash ^= state->extended_data[i].atom_id;
    hash ^= state->extended_data[i].value.int_val; // Works for int/float
  }

  return hash;
}

void goap_state_update_hash(GoapWorldStateEnhanced *state) {
  if (!state)
    return;
  state->bits.state_hash = goap_state_compute_hash(state);
}

// ===========================================================================================
// EXTENDED DATA (Non-boolean atoms)
// ===========================================================================================

static int find_extended_data(const GoapWorldStateEnhanced *state,
                              GoapAtomID atom) {
  for (int i = 0; i < state->extended_count; i++) {
    if (state->extended_data[i].atom_id == atom) {
      return i;
    }
  }
  return -1;
}

void goap_state_set_int(GoapWorldStateEnhanced *state, GoapAtomID atom,
                        int value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add int for atom %d", atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_INT;
  state->extended_data[idx].value.int_val = value;
  state->bits.state_hash = 0; // Invalidate hash
}

void goap_state_set_float(GoapWorldStateEnhanced *state, GoapAtomID atom,
                          float value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add float for atom %d", atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_FLOAT;
  state->extended_data[idx].value.float_val = value;
  state->bits.state_hash = 0;
}

void goap_state_set_vector(GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 value) {
  if (!state)
    return;

  int idx = find_extended_data(state, atom);
  if (idx < 0) {
    if (state->extended_count >= 16) {
      LOG_ERROR("GOAP: Extended data full, cannot add vector for atom %d",
                atom);
      return;
    }
    idx = state->extended_count++;
    state->extended_data[idx].atom_id = atom;
  }

  state->extended_data[idx].type = GOAP_STATE_VECTOR3;
  state->extended_data[idx].value.vector_val = value;
  state->bits.state_hash = 0;
}

int goap_state_get_int(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                       int default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_INT) {
    return state->extended_data[idx].value.int_val;
  }
  return default_val;
}

float goap_state_get_float(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           float default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_FLOAT) {
    return state->extended_data[idx].value.float_val;
  }
  return default_val;
}

Vec3 goap_state_get_vector(const GoapWorldStateEnhanced *state, GoapAtomID atom,
                           Vec3 default_val) {
  if (!state)
    return default_val;

  int idx = find_extended_data(state, atom);
  if (idx >= 0 && state->extended_data[idx].type == GOAP_STATE_VECTOR3) {
    return state->extended_data[idx].value.vector_val;
  }
  return default_val;
}

// ===========================================================================================
// DEBUG
// ===========================================================================================

const char *goap_atom_name(GoapAtomID atom) {
  switch (atom) {
  case GOAP_ATOM_HAS_WEAPON:
    return "HAS_WEAPON";
  case GOAP_ATOM_HAS_AMMO:
    return "HAS_AMMO";
  case GOAP_ATOM_WEAPON_LOADED:
    return "WEAPON_LOADED";
  case GOAP_ATOM_IN_COMBAT:
    return "IN_COMBAT";
  case GOAP_ATOM_TARGET_VISIBLE:
    return "TARGET_VISIBLE";
  case GOAP_ATOM_TARGET_IN_RANGE:
    return "TARGET_IN_RANGE";
  case GOAP_ATOM_UNDER_FIRE:
    return "UNDER_FIRE";
  case GOAP_ATOM_HEALTHY:
    return "HEALTHY";
  case GOAP_ATOM_WOUNDED:
    return "WOUNDED";
  case GOAP_ATOM_HAS_MEDKIT:
    return "HAS_MEDKIT";
  case GOAP_ATOM_HAS_ARMOR:
    return "HAS_ARMOR";
  case GOAP_ATOM_AT_COVER:
    return "AT_COVER";
  case GOAP_ATOM_AT_OBJECTIVE:
    return "AT_OBJECTIVE";
  case GOAP_ATOM_PATH_CLEAR:
    return "PATH_CLEAR";
  case GOAP_ATOM_DOOR_OPEN:
    return "DOOR_OPEN";
  case GOAP_ATOM_ENEMY_HEARD:
    return "ENEMY_HEARD";
  case GOAP_ATOM_ENEMY_SEEN:
    return "ENEMY_SEEN";
  case GOAP_ATOM_ALERT:
    return "ALERT";
  case GOAP_ATOM_INVESTIGATING:
    return "INVESTIGATING";
  case GOAP_ATOM_OBJECTIVE_COMPLETE:
    return "OBJECTIVE_COMPLETE";
  case GOAP_ATOM_ITEM_COLLECTED:
    return "ITEM_COLLECTED";
  case GOAP_ATOM_AREA_SECURED:
    return "AREA_SECURED";
  case GOAP_ATOM_IN_FORMATION:
    return "IN_FORMATION";
  case GOAP_ATOM_FOLLOWING_LEADER:
    return "FOLLOWING_LEADER";
  case GOAP_ATOM_SQUAD_READY:
    return "SQUAD_READY";
  default:
    return "UNKNOWN";
  }
}

void goap_state_print(const GoapWorldStateEnhanced *state) {
  if (!state)
    return;

  LOG_INFO("=== GOAP World State ===");
  LOG_INFO("Bits: 0x%016llX", state->bits.bits);
  LOG_INFO("Active atoms:");

  for (int i = 0; i < GOAP_MAX_STATE_ATOMS; i++) {
    if (goap_state_get_atom(state, i)) {
      LOG_INFO("  [%d] %s", i, goap_atom_name(i));
    }
  }

  if (state->extended_count > 0) {
    LOG_INFO("Extended data:");
    for (int i = 0; i < state->extended_count; i++) {
      GoapAtomID atom = state->extended_data[i].atom_id;
      switch (state->extended_data[i].type) {
      case GOAP_STATE_INT:
        LOG_INFO("  %s = %d", goap_atom_name(atom),
                 state->extended_data[i].value.int_val);
        break;
      case GOAP_STATE_FLOAT:
        LOG_INFO("  %s = %.2f", goap_atom_name(atom),
                 state->extended_data[i].value.float_val);
        break;
      default:
        break;
      }
    }
  }
}
// ===========================================================================================
// PHASE 6: ENHANCED ACTION DEFINITIONS  
// ===========================================================================================

GoapActionEnhanced* goap_action_create_enhanced(const char* name, float cost) {
    GoapActionEnhanced* action = (GoapActionEnhanced*)MALLOC_TAGGED(sizeof(GoapActionEnhanced), MEMORY_TAG_AI);
    if (!action) {
        LOG_ERROR("Failed to allocate GOAP action");
        return NULL;
    }
    
    memset(action, 0, sizeof(GoapActionEnhanced));
    strncpy(action->name, name, sizeof(action->name) - 1);
    action->cost = cost;
    action->duration = 1.0f;
    
    return action;
}

void goap_action_destroy_enhanced(GoapActionEnhanced* action) {
    if (!action) return;
    FREE(0);
}

void goap_action_add_precondition_bit(GoapActionEnhanced* action, GoapAtomID atom, bool value) {
    if (!action) return;
    goap_state_set_atom(&action->preconditions, atom, value);
}

void goap_action_add_effect_bit(GoapActionEnhanced* action, GoapAtomID atom, bool value) {
    if (!action) return;
    goap_state_set_atom(&action->effects, atom, value);
}

bool goap_action_can_run(const GoapActionEnhanced* action, const GoapWorldStateEnhanced* state) {
    if (!action || !state) return false;
    
    if (!goap_state_satisfies(state, &action->preconditions)) {
        return false;
    }
    
    if (action->check_precondition) {
        return action->check_precondition(NULL, state);
    }
    
    return true;
}

void goap_action_apply(const GoapActionEnhanced* action, GoapWorldStateEnhanced* state) {
    if (!action || !state) return;
    
    state->bits.bits |= action->effects.bits.bits;
    
    if (action->apply_effects) {
        action->apply_effects(NULL, state);
    }
    
    state->bits.state_hash = 0;
}
