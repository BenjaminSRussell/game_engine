#ifndef DLSS_WRAPPER_H
#define DLSS_WRAPPER_H

#include "assets/resources/resource_management/resource_handle.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum upscaler_type {
    UPSCALER_DLSS,
    UPSCALER_FSR,
    UPSCALER_XESS
} upscaler_type_t;

typedef struct upscaler {
    upscaler_type_t type;
    void* impl;
} upscaler_t;

upscaler_t* postprocessing_upscale_create(upscaler_type_t type, int width, int height);
void postprocessing_upscale_destroy(upscaler_t* up);
void postprocessing_upscale_dispatch(
    upscaler_type_t type,
    void* impl,
    texture_handle_t input,
    texture_handle_t output,
    texture_handle_t depth,
    texture_handle_t velocity,
    texture_handle_t exposure,
    float sharpness,
    float jitter_x,
    float jitter_y,
    float delta_time,
    bool reset
);

#endif
