#include "Public/Audio.h"
#include "Public/AudioHelper.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

// Internal state
typedef struct AudioInternal {
  ALCdevice *alc_device;
  ALCcontext *alc_context;
  bool initialized;
} AudioInternal;

static AudioInternal g_audio;

// Struct Implementations
struct AudioDevice {
  int id;
};
struct AudioMixer {
  void *output_buffer;
};
struct AudioBus {
  char name[32];
};

// --- Audio Helper Implementation ---

int Audio_GenerateTestSound(int frequency, float duration) {
  if (!g_audio.initialized)
    return 0;

  int sample_rate = 44100;
  int samples = (int)(duration * sample_rate);
  short *buffer_data = (short *)malloc(samples * sizeof(short));

  for (int i = 0; i < samples; i++) {
    // Simple decay envelope
    float time = (float)i / sample_rate;
    float envelope = expf(-5.0f * time);

    // Sine wave + some noise for "Thud" texture
    float sine = sinf(2.0f * 3.14159f * frequency * time);
    float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.5f;

    float signal = (sine + noise) * envelope * 0.5f;

    // Clamp
    if (signal > 1.0f)
      signal = 1.0f;
    if (signal < -1.0f)
      signal = -1.0f;

    buffer_data[i] = (short)(signal * 32767);
  }

  ALuint buffer;
  alGenBuffers(1, &buffer);
  alBufferData(buffer, AL_FORMAT_MONO16, buffer_data, samples * sizeof(short),
               sample_rate);

  free(buffer_data);

  // Check error
  if (alGetError() != AL_NO_ERROR) {
    printf("Audio Error: Failed to generate test sound buffer\n");
    return 0;
  }

  return (int)buffer;
}

// --- Wrapper for ECS systems to play sounds ---
// (We will add a proper PlaySound function later, for now we expose
// alSourcePlay etc via components)

// Implementation Stubs / Wrappers
AudioDevice *AudioDevice_Create(void) { return malloc(sizeof(AudioDevice)); }
void AudioDevice_Destroy(AudioDevice *device) { free(device); }
void AudioDevice_Submit(AudioDevice *device, void *buffer) {
  (void)device;
  (void)buffer;
}

AudioMixer *AudioMixer_Create(void) { return malloc(sizeof(AudioMixer)); }
void AudioMixer_Destroy(AudioMixer *mixer) { free(mixer); }
void AudioMixer_Mix(AudioMixer *mixer, float dt) {
  (void)mixer;
  (void)dt;
}

AudioBus *AudioBus_Create(const char *name) {
  AudioBus *b = malloc(sizeof(AudioBus));
  strncpy(b->name, name, 31);
  return b;
}
void AudioBus_Destroy(AudioBus *bus) { free(bus); }
void AudioBus_Connect(AudioBus *source, AudioBus *dest) {
  (void)source;
  (void)dest;
}

void SpatialAudio_Update(float dt) { (void)dt; }
void AudioCommandProcessor_Update(void) {}

bool Audio_Init(void) {
  if (g_audio.initialized)
    return true;

  // Open default device
  g_audio.alc_device = alcOpenDevice(NULL);
  if (!g_audio.alc_device) {
    printf("Audio Error: Failed to open default device\n");
    return false;
  }

  // Create context
  g_audio.alc_context = alcCreateContext(g_audio.alc_device, NULL);
  if (!g_audio.alc_context || !alcMakeContextCurrent(g_audio.alc_context)) {
    printf("Audio Error: Failed to create/set OpenAL context\n");
    alcCloseDevice(g_audio.alc_device);
    return false;
  }

  printf("Audio System Initialized (OpenAL)\n");
  g_audio.initialized = true;

  // Setup listener default
  ALfloat pos[] = {0, 0, 0};
  ALfloat vel[] = {0, 0, 0};
  ALfloat ori[] = {0, 0, -1, 0, 1, 0};
  alListenerfv(AL_POSITION, pos);
  alListenerfv(AL_VELOCITY, vel);
  alListenerfv(AL_ORIENTATION, ori);

  return true;
}

void Audio_Update(float delta_time) {
  (void)delta_time;
  // OpenAL handles mixing internally
}

void Audio_Shutdown(void) {
  if (g_audio.initialized) {
    alcMakeContextCurrent(NULL);
    alcDestroyContext(g_audio.alc_context);
    alcCloseDevice(g_audio.alc_device);
    g_audio.initialized = false;
    printf("Audio System Shutdown\n");
  }
}
