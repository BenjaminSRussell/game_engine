#ifndef DIRECTOR_AI_H
#define DIRECTOR_AI_H

#ifdef __cplusplus
extern "C" {
#endif

void director_ai_init(void);
void director_update(void);
void director_set_intensity(float intensity);

#ifdef __cplusplus
}
#endif

#endif // DIRECTOR_AI_H
