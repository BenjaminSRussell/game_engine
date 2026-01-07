#ifndef ACES_TONEMAPPER_H
#define ACES_TONEMAPPER_H

#include "tonemapping_types.h"

void aces_tonemapper_init(tonemapper_t* tm, id<MTLDevice> device, id<MTLLibrary> library);
void aces_tonemapper_render(tonemapper_t* tm, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_input, id<MTLTexture> bloom_input, id<MTLTexture> ldr_output);

#endif // ACES_TONEMAPPER_H
