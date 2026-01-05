#include "renderer/gi/light_probes.h"

void light_probe_init() {}

void light_probe_bake(float position[3], void *output) {
    // Capture irradiance in all directions
}

void light_probe_sample(float position[3], float normal[3], float *color) {
    // Interpolate between probes
}
