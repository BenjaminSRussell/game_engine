#include "audio/audio_engine.h"
#include <stdlib.h>

void audio_init() {}

void audio_shutdown() {}

void *audio_load_sound(const char *path) {
    return NULL;
}

void audio_play_sound(void *sound, float volume) {}

void audio_play_sound_3d(void *sound, float position[3], float volume) {}

void audio_set_listener_position(float position[3]) {}

void audio_set_listener_orientation(float forward[3], float up[3]) {}

void audio_update() {}
