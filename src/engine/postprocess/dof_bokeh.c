#include "postprocess/depth_of_field/dof_bokeh.h"
#include <stdlib.h>

typedef struct DOFContext {
    float focus_distance;
    float aperture;
    int bokeh_samples;
    float bokeh_rotation;
} DOFContext;

static DOFContext g_dof_ctx = {0};

void dof_init() {
    g_dof_ctx.focus_distance = 10.0f;
    g_dof_ctx.aperture = 2.8f;
    g_dof_ctx.bokeh_samples = 32;
    g_dof_ctx.bokeh_rotation = 0.0f;
}

void dof_generate_coc(void *depth, void *coc_output) {
    // Calculate circle of confusion from depth
}

void dof_bokeh_blur(void *input, void *coc, void *output) {
    // Apply hexagonal/circular bokeh blur
}

void dof_set_focus(float distance, float aperture) {
    g_dof_ctx.focus_distance = distance;
    g_dof_ctx.aperture = aperture;
}
