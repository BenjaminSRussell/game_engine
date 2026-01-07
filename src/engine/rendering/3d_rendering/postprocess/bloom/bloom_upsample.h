#ifndef BLOOM_UPSAMPLE_H
#define BLOOM_UPSAMPLE_H

#include "bloom_types.h"

void bloom_upsample_init(bloom_system_t* bloom, id<MTLDevice> device, id<MTLLibrary> library);
void bloom_upsample_dispatch(bloom_system_t* bloom, id<MTLCommandBuffer> cmd);

#endif // BLOOM_UPSAMPLE_H
