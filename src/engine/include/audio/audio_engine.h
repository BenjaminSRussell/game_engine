#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

// Minimal stub header for audio_engine.c
void audio_engine_init(void);
void audio_engine_shutdown(void);
void audio_engine_update(float delta_time);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_ENGINE_H
