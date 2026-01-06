#include "dlss_wrapper.h"
#include <stddef.h>
#include <string.h>

// Mock Vendor Headers (would usually include <nvsdk_ngx.h>, <ffx_fsr2.h>, etc)
// Assuming definitions for compilation.

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
) {
    if (!impl) return;

    switch (type) {
        case UPSCALER_DLSS:
            // dlss_evaluate(impl, ...);
            break;
        case UPSCALER_FSR:
            // fsr_dispatch(impl, ...);
            break;
        case UPSCALER_XESS:
            // xess_execute(impl, ...);
            break;
        default:
            break;
    }
}

upscaler_t* postprocessing_upscale_create(upscaler_type_t type, int width, int height) {
    upscaler_t* up = calloc(1, sizeof(upscaler_t));
    up->type = type;
    // up->impl = vendor_create(...)
    return up;
}

void postprocessing_upscale_destroy(upscaler_t* up) {
    if (!up) return;
    // vendor_destroy(up->impl)
    free(up);
}
