# Audio System API Documentation

## Overview

The audio system provides comprehensive 3D audio capabilities including spatial audio, DSP effects, streaming, and advanced surround sound support.

## Core Components

### Audio Core System (`audio_core.h`)

Basic audio functionality for simple use cases:

```c
// Initialize audio core system
void audio_core_init(void);

// Volume control
void audio_core_set_master_volume(float volume);
void audio_core_set_sfx_volume(float volume);
void audio_core_set_music_volume(float volume);

// Playback
void audio_core_play_sfx(const char* sound_name, float volume);
void audio_core_play_music(const char* music_name, float volume);
```

### Advanced Audio System (`advanced_audio.h`)

Full-featured 3D audio system:

```c
// World management
AudioWorld* audio_world_create(uint32_t maxSources, uint32_t maxBuffers, uint32_t maxEffects);
void audio_world_update(AudioWorld* world, float deltaTime);

// 3D listener
void audio_listener_set_position(AudioWorld* world, float x, float y, float z);
void audio_listener_set_orientation(AudioWorld* world, float forwardX, float forwardY, float forwardZ, 
                                   float upX, float upY, float upZ);

// 3D sources
uint32_t audio_source_create(AudioWorld* world);
void audio_source_set_position(AudioWorld* world, uint32_t sourceId, float x, float y, float z);
void audio_source_set_3d_settings(AudioWorld* world, uint32_t sourceId, const AudioSource3D* settings);
```

### DSP Effects

#### Compressor (`dsp/compressor.h`)
Dynamic range compression with full parameter control:

```c
typedef struct {
    float threshold_db;    // Compression threshold (-60 to 0 dB)
    float ratio;          // Compression ratio (1:1 to 20:1)
    float attack_ms;      // Attack time (0.1 to 100 ms)
    float release_ms;     // Release time (10 to 2000 ms)
    float makeup_gain_db; // Makeup gain (-20 to +20 dB)
} Compressor;

void compressor_init(Compressor *comp, int sample_rate);
float compressor_process(Compressor *comp, float input, float sidechain_input);
```

### Spatial Audio (`spatial/spatial_audio_config.h`)

Advanced surround sound and object-based audio:

```c
typedef enum {
    AUDIO_FORMAT_DOLBY_ATMOS,
    AUDIO_FORMAT_DTSX,
    AUDIO_FORMAT_CUSTOM_SURROUND
} SpatialAudioFormat;

// Speaker configuration
void spatial_audio_load_preset_51(SpatialAudioConfig *config);
void spatial_audio_load_preset_71(SpatialAudioConfig *config);
void spatial_audio_load_preset_714(SpatialAudioConfig *config); // 7.1.4 Atmos
```

## Usage Examples

### Basic 3D Audio Setup

```c
#include "audio/advanced_audio.h"

// Create audio world
AudioWorld* world = audio_world_create(32, 64, 8);

// Set up listener at origin
audio_listener_set_position(world, 0.0f, 0.0f, 0.0f);

// Load sound buffer
uint32_t bufferId = audio_buffer_load_from_file(world, "sounds/footstep.wav");

// Create 3D source
uint32_t sourceId = audio_source_create(world);
audio_source_set_buffer(world, sourceId, bufferId);
audio_source_set_position(world, sourceId, 5.0f, 0.0f, 0.0f);

// Play sound
audio_source_play(world, sourceId);

// Update in game loop
while (game_running) {
    audio_world_update(world, deltaTime);
}
```

### DSP Effect Chain

```c
#include "audio/dsp/compressor.h"
#include "audio/dsp/reverb.h"

// Initialize compressor
Compressor comp;
compressor_init(&comp, 48000);
compressor_set_threshold(&comp, -12.0f);
compressor_set_ratio(&comp, 4.0f);

// Process audio samples
for (int i = 0; i < sample_count; i++) {
    float processed = compressor_process(&comp, input_samples[i], 0.0f);
    output_samples[i] = processed;
}
```

## Data Structures

### AudioFormat
- `AUDIO_FORMAT_MONO8` - 8-bit mono
- `AUDIO_FORMAT_MONO16` - 16-bit mono
- `AUDIO_FORMAT_STEREO8` - 8-bit stereo
- `AUDIO_FORMAT_STEREO16` - 16-bit stereo
- `AUDIO_FORMAT_5POINT1_16` - 5.1 surround 16-bit
- `AUDIO_FORMAT_7POINT1_16` - 7.1 surround 16-bit

### AudioSampleRate
- `AUDIO_SAMPLE_RATE_44KHZ` - CD quality (44100 Hz)
- `AUDIO_SAMPLE_RATE_48KHZ` - Standard digital audio (48000 Hz)
- `AUDIO_SAMPLE_RATE_96KHZ` - High resolution (96000 Hz)

## Performance Considerations

- Use audio source pooling to avoid allocation overhead
- Implement voice management for limited simultaneous sounds
- Consider audio streaming for large files
- Use distance-based culling for distant sounds
- Implement LOD for audio quality based on distance

## Thread Safety

- Audio world updates should be called from main thread
- Source/buffer creation/destruction must be synchronized
- DSP processing is thread-safe for separate instances
