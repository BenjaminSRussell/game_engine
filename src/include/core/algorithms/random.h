#ifndef CORE_ALGORITHMS_RANDOM_H
#define CORE_ALGORITHMS_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

void random_seed(unsigned long long seed);
unsigned int random_uint();
float random_float();
float random_range(float min, float max);
int random_int_range(int min, int max);

#ifdef __cplusplus
}
#endif

#endif // CORE_ALGORITHMS_RANDOM_H
