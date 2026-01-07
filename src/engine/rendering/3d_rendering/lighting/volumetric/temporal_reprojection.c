/*
 * temporal_reprojection.c
 * Volumetric temporal accumulation
 */

#include "temporal_reprojection.h"
#include <stdlib.h>
#include <math.h>

// Halton sequence helper
static float halton(uint32_t index, uint32_t base) {
    float result = 0;
    float f = 1.0f / (float)base;
    uint32_t i = index;
    while (i > 0) {
        result += f * (i % base);
        i /= base;
        f /= (float)base;
    }
    return result;
}

void temporal_reprojection_init(temporal_reprojection_t* reproj, metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d) {
    metal_texture_desc_t desc = {0};
    desc.type = METAL_TEXTURE_TYPE_3D;
    desc.format = METAL_PIXEL_FORMAT_RGBA16_FLOAT;
    desc.width = w;
    desc.height = h;
    desc.depth = d;
    desc.usage = METAL_TEXTURE_USAGE_SHADER_READ | METAL_TEXTURE_USAGE_SHADER_WRITE;
    
    reproj->history_texture = metal_texture_create(dev, &desc);
    reproj->feedback_factor = 0.95f;
    reproj->frame_index = 0;
    reproj->jitter_x = 0;
    reproj->jitter_y = 0;
}

void temporal_reprojection_cleanup(temporal_reprojection_t* reproj) {
    if (reproj->history_texture) {
        metal_texture_destroy(reproj->history_texture);
        reproj->history_texture = NULL;
    }
}

void temporal_reprojection_next_frame(temporal_reprojection_t* reproj) {
    reproj->frame_index++;
    
    // 8-sample Halton sequence
    uint32_t index = (reproj->frame_index % 8) + 1;
    reproj->jitter_x = halton(index, 2) - 0.5f;
    reproj->jitter_y = halton(index, 3) - 0.5f;
}
