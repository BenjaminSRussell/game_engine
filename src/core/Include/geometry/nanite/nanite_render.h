#ifndef NANITE_RENDER_H
#define NANITE_RENDER_H

#include "geometry/nanite/nanite_cluster.h"
#include "math/math_all.h"
#include <Metal/Metal.h>

typedef struct NaniteRenderer NaniteRenderer;

NaniteRenderer *nanite_renderer_create(id<MTLDevice> device,
                                       id<MTLLibrary> shader_lib);

void nanite_render_mesh(NaniteRenderer *renderer,
                        id<MTLRenderCommandEncoder> encoder,
                        id<MTLCommandBuffer> cmd, NaniteMesh *mesh,
                        const Mat4 *view_proj, const Vec3 *camera_pos);

void nanite_renderer_destroy(NaniteRenderer *renderer);

#endif // NANITE_RENDER_H
