#ifndef AUDIO_LOADER_H
#define AUDIO_LOADER_H

#include <../../include/vendor/miniaudio.h>
#include <common.h>

// Forward declarations
struct AssetManager;

// Audio resource management
ma_sound *audio_loader_load_sound(const char *filepath, ma_engine *engine,
                                  u32 flags);
void audio_loader_unload_sound(ma_sound *sound);

// Stream management (for music)
ma_sound *audio_loader_stream_sound(const char *filepath, ma_engine *engine);

#endif // AUDIO_LOADER_H
