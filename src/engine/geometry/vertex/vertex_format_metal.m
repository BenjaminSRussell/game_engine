#include "geometry/vertex/vertex_format.h"

#ifdef __OBJC__
#import <Metal/Metal.h>

MTLVertexDescriptor *vertex_format_to_metal(vertex_format_t *fmt) {
  if (!fmt)
    return nil;

  MTLVertexDescriptor *desc = [[MTLVertexDescriptor alloc] init];

  for (int i = 0; i < 8; i++) {
    if (fmt->attribute_mask & (1 << i)) {
      desc.attributes[i].format = (MTLVertexFormat)fmt->formats[i];
      desc.attributes[i].offset = fmt->offsets[i];
      desc.attributes[i].bufferIndex = 0;
    }
  }

  desc.layouts[0].stride = fmt->stride;
  desc.layouts[0].stepRate = 1;
  desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

  return desc;
}
#endif
