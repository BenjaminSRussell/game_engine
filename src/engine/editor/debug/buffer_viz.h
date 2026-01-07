#ifndef BUFFER_VIZ_H
#define BUFFER_VIZ_H

#include <Metal/Metal.h>

void debug_draw_texture_fullscreen(id<MTLRenderCommandEncoder> encoder, id<MTLTexture> texture, float scale);

#endif // BUFFER_VIZ_H
