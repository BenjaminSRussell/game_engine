#include "core/algorithms/random.h"
#include <stdlib.h>

static unsigned long long g_seed = 12345;

void random_seed(unsigned long long seed) {
    g_seed = seed;
}

unsigned int random_uint() {
    g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned int)(g_seed >> 32);
}

float random_float() {
    return (float)random_uint() / (float)0xFFFFFFFFU;
}

float random_range(float min, float max) {
    return min + random_float() * (max - min);
}

int random_int_range(int min, int max) {
    return min + (int)(random_uint() % (max - min + 1));
}
