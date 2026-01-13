/*
 * Audio Core System Header
 */

#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize audio core system
void audio_core_init(void);

// Shutdown audio core system
void audio_core_shutdown(void);

// Set master volume (0.0 to 1.0)
void audio_core_set_master_volume(float volume);

// Get master volume
float audio_core_get_master_volume(void);

// Set SFX volume (0.0 to 1.0)
void audio_core_set_sfx_volume(float volume);

// Get SFX volume
float audio_core_get_sfx_volume(void);

// Set music volume (0.0 to 1.0)
void audio_core_set_music_volume(float volume);

// Get music volume
float audio_core_get_music_volume(void);

// Check if audio is initialized
int audio_core_is_initialized(void);

// Play sound effect
void audio_core_play_sfx(const char* sound_name, float volume);

// Play music
void audio_core_play_music(const char* music_name, float volume);

// Stop music
void audio_core_stop_music(void);

// Pause all audio
void audio_core_pause_all(void);

// Resume all audio
void audio_core_resume_all(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_CORE_H
