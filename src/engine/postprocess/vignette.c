#include "renderer/post_processing/vignette.h"
#include <include/math/math.h>

typedef struct VignetteContext {
    float intensity;
    float smoothness;
    float roundness;
} VignetteContext;

static VignetteContext g_vignette_ctx = {0};

void vignette_init() {
    g_vignette_ctx.intensity = 0.4f;
    g_vignette_ctx.smoothness = 0.5f;
    g_vignette_ctx.roundness = 1.0f;
}

void vignette_apply(void *input, void *output, int width, int height) {
    // Darken edges based on distance from center
}

void vignette_set_params(float intensity, float smoothness) {
    g_vignette_ctx.intensity = intensity;
    g_vignette_ctx.smoothness = smoothness;
}
