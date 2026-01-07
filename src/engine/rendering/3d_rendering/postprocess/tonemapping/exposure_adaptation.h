#ifndef EXPOSURE_ADAPTATION_H
#define EXPOSURE_ADAPTATION_H

#include "tonemapping_types.h"

void exposure_adaptation_init(tonemapper_t* tm, id<MTLDevice> device, id<MTLLibrary> library);
void exposure_adaptation_dispatch(tonemapper_t* tm, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_src, float dt);

#endif // EXPOSURE_ADAPTATION_H
