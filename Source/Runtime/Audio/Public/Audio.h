#ifndef ULTIMATE_ENGINE_AUDIO_H
#define ULTIMATE_ENGINE_AUDIO_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct AudioDevice AudioDevice;
typedef struct AudioMixer AudioMixer;
typedef struct AudioBus AudioBus;

typedef struct AudioSystem {
  AudioDevice *device;
  AudioMixer *mixer;
  AudioBus *master_bus;
  AudioBus *sfx_bus;
  AudioBus *music_bus;
} AudioSystem;

// Lifecycle
bool Audio_Init(void);
void Audio_Shutdown(void);
void Audio_Update(float delta_time);

// Subsystems
AudioDevice *AudioDevice_Create(void);
void AudioDevice_Destroy(AudioDevice *device);
void AudioDevice_Submit(AudioDevice *device, void *buffer);

AudioMixer *AudioMixer_Create(void);
void AudioMixer_Destroy(AudioMixer *mixer);
void AudioMixer_Mix(AudioMixer *mixer, float dt);

AudioBus *AudioBus_Create(const char *name);
void AudioBus_Destroy(AudioBus *bus);
void AudioBus_Connect(AudioBus *source, AudioBus *dest);

void SpatialAudio_Update(float dt);
void AudioCommandProcessor_Update(void);

#endif // ULTIMATE_ENGINE_AUDIO_H
