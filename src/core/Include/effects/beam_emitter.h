// VFX/Beam Emitter
#ifndef BEAM_EMITTER_H
#define BEAM_EMITTER_H

#include <common.h>
#include "math/vec3.h"
#import <Metal/Metal.h>

typedef struct {
  Vec3 source;
  Vec3 target;
  f32 width;
  Vec4 color;
  f32 noise_amplitude;
  f32 scroll_speed;
  u32 tessellation_segments;
  id<MTLTexture> beam_texture;
  id<MTLBuffer> vertex_buffer;
} BeamEmitter;

#ifdef __cplusplus
extern "C" {
#endif

BeamEmitter *beam_create(id<MTLDevice> device);
void beam_set_endpoints(BeamEmitter *beam, Vec3 source, Vec3 target);
void beam_update(BeamEmitter *beam, f32 delta_time);
void beam_render(BeamEmitter *beam, id<MTLRenderCommandEncoder> encoder);

#ifdef __cplusplus
}
#endif

#endif
