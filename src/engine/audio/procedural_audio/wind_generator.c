#include "audio/procedural_audio/wind_generator.h"
#include <math.h>

void wind_generator_init() {}

void wind_generate(float speed, float *output, int sample_count) {
    for (int i = 0; i < sample_count; i++) {
        output[i] = ((float)rand() / RAND_MAX - 0.5f) * speed;
    }
}
