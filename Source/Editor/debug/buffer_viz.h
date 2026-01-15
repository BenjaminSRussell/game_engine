#ifndef BUFFER_VIZ_H
#define BUFFER_VIZ_H

#ifdef __OBJC__
#include <Metal/Metal.h>
#else
typedef void* id;
typedef struct {} MTLRenderCommandEncoder;
typedef struct {} MTLTexture;
#endif

void debug_draw_texture_fullscreen(id<MTLRenderCommandEncoder> encoder, id<MTLTexture> texture, float scale);

#endif // BUFFER_VIZ_H
