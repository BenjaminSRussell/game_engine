#include "engine/cinematics/sequencer/sequencer_core.h"
#include <stddef.h>

// SEQUENCER TIME MANAGEMENT

float sequencer_get_current_time(SequencerSystem *system) {
  if (system == NULL || system->active_sequence == NULL) {
    return 0.0f;
  }
  return system->active_sequence->current_time;
}

float sequencer_get_duration(SequencerSystem *system) {
  if (system == NULL || system->active_sequence == NULL) {
    return 0.0f;
  }
  return system->active_sequence->duration;
}
