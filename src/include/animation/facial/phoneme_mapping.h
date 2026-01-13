/**
 * @file phoneme_mapping.h
 * @brief Facial animation phoneme mapping for lipsync.
 */
#ifndef ANIMATION_FACIAL_PHONEME_MAPPING_H
#define ANIMATION_FACIAL_PHONEME_MAPPING_H

#ifdef __cplusplus
extern "C" {
#endif

void phoneme_mapping_init(void);
void phoneme_mapping_load(const char *mapping_file);
void phoneme_mapping_get_blend_shape(const char *phoneme, void *blend_shapes);
void phoneme_mapping_animate_from_audio(void *audio, void *output_animation);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_FACIAL_PHONEME_MAPPING_H
