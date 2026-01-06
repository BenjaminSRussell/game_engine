#include "renderer/post_processing/chromatic_aberration.h"

typedef struct ChromaticAberrationContext {
    float intensity;
    float samples;
} ChromaticAberrationContext;

static ChromaticAberrationContext g_chroma_ctx = {0};

void chromatic_aberration_init() {
    g_chroma_ctx.intensity = 0.5f;
    g_chroma_ctx.samples = 3.0f;
}

void chromatic_aberration_apply(void *input, void *output) {
    // Offset R, G, B channels radially from center
}

void chromatic_aberration_set_intensity(float intensity) {
    g_chroma_ctx.intensity = intensity;
}
