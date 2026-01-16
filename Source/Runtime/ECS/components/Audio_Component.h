#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Audio Source Component (Attach to entity to play sound)
typedef struct AudioSourceComponent {
  int buffer_id; // OpenAL Buffer ID
  int source_id; // OpenAL Source ID (Runtime)
  float pitch;
  float gain;
  bool loop;
  bool is_3d; // true = spatial, false = ui/music
  bool play_on_start;
  bool is_playing;
} AudioSourceComponent;

// Audio Listener Component (Attach to camera)
typedef struct AudioListenerComponent {
  bool active;
} AudioListenerComponent;

#endif // AUDIO_COMPONENT_H
