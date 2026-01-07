#ifndef BLOOM_DOWNSAMPLE_H
#define BLOOM_DOWNSAMPLE_H

#include "bloom_types.h"

void bloom_downsample_init(bloom_system_t* bloom, id<MTLDevice> device, id<MTLLibrary> library);
void bloom_downsample_dispatch(bloom_system_t* bloom, id<MTLCommandBuffer> cmd, id<MTLTexture> hdr_input);

#endif // BLOOM_DOWNSAMPLE_H
