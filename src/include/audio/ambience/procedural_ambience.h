#ifndef PROCEDURAL_AMBIENCE_H
#define PROCEDURAL_AMBIENCE_H

#ifdef __cplusplus
extern "C" {
#endif

void procedural_ambience_init(void);
void procedural_ambience_generate(void* biome, void* output_audio);
void procedural_ambience_update(void* location, float time_of_day);

#ifdef __cplusplus
}
#endif

#endif // PROCEDURAL_AMBIENCE_H
